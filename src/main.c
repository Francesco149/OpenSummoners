/*
 * OpenSummoners — drop-in replacement for sotes.exe (skeleton).
 *
 * Phase-0 skeleton: register a window class, create a 640x480 window
 * (placeholder — retail's actual surface size will be confirmed once we
 * see DirectDrawCreateEx + SetCooperativeLevel), run a PeekMessage/Dispatch
 * loop with a millisecond-resolution Sleep gate, exit cleanly on WM_QUIT.
 * Real renderer / audio / input wiring lands in subsequent slices.
 *
 * Drop-in defaults:
 *   - Auto-cd into OPENSUMMONERS_GAME_DIR and SetDllDirectory to the same
 *     so any later LoadLibrary calls resolve to the game-dir DLLs
 *     (sotesd.dll / sotesp.dll / sotesw.dll) rather than picking up
 *     anything from system32.  This was a footgun the OpenMare port hit
 *     when its renderer DLL load picked the OS ddraw.dll instead of
 *     the game's wrapper — same risk class here.
 *   - MessageBox→stderr hook installed unconditionally (override with
 *     --show-msgbox).  A blocked modal on Wndclass / DDraw init would
 *     otherwise stall the harness invisibly.
 *   - Single-instance mutex so a stray .exe from a previous SIGKILL'd
 *     run can't silently shadow the current build.  Skip with --allow-multi.
 *   - --hide-window: skip ShowWindow for harness runs.
 *   - --frames N: voluntary exit after N main-loop iterations so stderr
 *     flushes cleanly for the smoke-test pipeline.
 *   - --skip-ddraw: skip DDraw init entirely (window-only boot).
 *   - --no-present: skip the per-frame zdd_present dispatch
 *     (DDraw init still runs).
 *   - --no-title-scene: skip the title-scene drive (FUN_0056aea0 caller)
 *     and run the legacy minimal present loop instead — a known-good
 *     black/uninitialised window, useful for isolating DDraw-init issues
 *     from the scene runner.
 *   - --input-trace <file.jsonl>: replay a sparse {frame,ids} input trace
 *     (the harness format, tools/frida_capture.py) into the drive's input
 *     ring, keyed on the present/Flip count — drives the title scene
 *     deterministically (skip-splash / menu nav).  No-op without the drive.
 *   - --held-trace <file.jsonl>: replay a sparse {frame,keys} HELD-AXIS trace
 *     (held_trace.h) — rebuilds the drive's input_mgr.axis_held[] each frame so
 *     held WALKING replays deterministically (the level counterpart of the ring
 *     presses; the freeroam mover / title auto-repeat read it).  Composes with
 *     --input-trace (ring Z-advance + held walk together).  No-op without the drive.
 *
 * Build: `make -C src` from inside `nix develop`.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "dev_hooks.h"
#include "zdd.h"
#include "cs_dispatch.h"
#include "call_trace.h"
#include "title_drive.h"
#include "title_sink.h"       /* title_sink_menu_trace — --menu-trace toggle */
#include "input_trace.h"
#include "held_trace.h"
#include "input_live.h"        /* the live keyboard producer (0x46a880; interactive) */
#include "asset_register.h"   /* ar_sprite_decode / ar_sprite_decode_hook */
#include "bitmap_session.h"   /* bs_convert_* for the 8d format switch adapter */
#include "pixel_drawer.h"     /* pd_boot_init_slots — the alpha-ramp descriptors */
#include "rng.h"              /* engine LCG — pinned seed for determinism */
#include "title_particles.h"  /* phase-7 sparkle-particle pool + spawn */
#include "app_flow.h"         /* post-title dispatch (FUN_00562ea0 tail switch) */
#include "menu_list.h"        /* menu_ctrl / menu_node — glyph render test */
#include "glyph_text.h"       /* glyph_cell_layout — glyph render test */
#include "glyph_render.h"     /* glyph_grid_render + glyph_gdi_ops_win32 */
#include "newgame_drive.h"    /* the new-game config scene drive (FUN_00565d10) */
#include "newgame_box.h"      /* the 9-slice box panel render (FUN_0048cf80) */
#include "newgame_cursor.h"   /* the menu selection cursor / gold vine (FUN_0048d940) */
#include "glyph_wrap.h"       /* the tooltip text-node word-wrap (FUN_0040e5e0)  */
#include "prologue_drive.h"   /* the Elemental-Stone intro cutscene (FUN_0056cd20) */
#include "game_drive.h"       /* the in-game map run loop (0x59f2c0 seam)     */
#include "town_render.h"      /* the in-game backdrop scene (decode→walk→present) */
#include "game_world.h"       /* the room registry: key -> {scene, parallax params} */
#include "color_grade.h"      /* the in-game palette color-grade LUT (0x417c40) */
#include "camera_follow.h"    /* the in-game camera easer (0x43d1d0) + setters (0x439690) */
#include "letterbox.h"        /* the establishing-shot cinematic letterbox (0x48c150 slice) */
#include "scene_fade.h"       /* the establishing REVEAL fade-grid (0x48e920 / 0x439690 arm) */
#include "actor_spawn.h"      /* the town CHARACTER-band spawn (0x58d460 -> 0x431e30) */
#include "actor_render.h"     /* actor_render_static (the 0x491ae0 default arm) */
#include "character.h"        /* the freeroam mover (character_step, bit-exact)  */
#include "particle.h"         /* the fountain spray (0x13e0 band / 0x46e510 / 0x493480) */
#include "butterfly.h"        /* the town butterflies' per-tick LCG (0x47b990 0xe29a) */
#include "ambient.h"          /* the town's irregular ambient/event RNG timers      */
#include "banner.h"           /* the area-title banner ("Town of Tonkiness", 0x494a60) */
#include "dialogue.h"         /* the in-game dialogue box (0x439690 widget / 0x48c820) */
#include "cutscene.h"         /* the town-arrival dialogue sequence (0x4d7d80 case 0x334be) */
#include "exe_strings.h"      /* story text read from the user's sotes.exe by VA */
#include "osr_recon.h"        /* --osr-replay: reconstruct frames from a .osr (M4) */
#include "osr_emit.h"         /* --osr-emit: the port-side .osr draw stream (M5) */

#define OPENSUMMONERS_CLASS  "OpenSummonersMain"
#define OPENSUMMONERS_TITLE  "Fortune Summoners"

/* Placeholder default surface.  Retail's actual window size needs a Ghidra
 * pass on CreateWindowEx / DirectDrawCreateEx call sites; until then we
 * boot a 640x480 surface (Fortune Summoners' on-disk readme suggests it
 * runs in 4:3 windowed by default — confirmable later). */
#define DEFAULT_WIDTH   640
#define DEFAULT_HEIGHT  480

/* Same-era games typically target 60 FPS; the exact frame period is a
 * Ghidra answer.  Until then, 16ms is a safe initial gate that won't peg
 * a single CPU and won't visibly lag. */
#define FRAME_PERIOD_MS  16

#define SINGLETON_MUTEX_NAME  "OpenSummoners-SingleInstance"

/* Default launcher mode = 2 (Windowed).  Retail picks this from the
 * launcher dialog's radio (stored in user/config.dat at +0x04).  Until
 * we port FUN_005a4770 (the 45 KB config.dat parser) the drop-in
 * hardcodes Windowed — the lowest-risk first wiring pass, doesn't
 * require display-mode changes that may fail under WSL/RDP/headless.
 * Override per-run via --launcher-mode=N (0=Full / 1=Safe / 2=Wind /
 * 3=DB / 4=Zoom).  Zoom-target defaults mirror retail's
 * `*(int*)(in_ECX + 0x14/0x18)` = 1280×960. */
#define DEFAULT_LAUNCHER_MODE   2
#define DEFAULT_ZOOM_TARGET_W   1280
#define DEFAULT_ZOOM_TARGET_H   960

static HINSTANCE g_hInstance;
static HWND      g_hwnd;
static int       g_hide_window;
static int       g_no_frame_limit;   /* --no-frame-limit: uncap the 60 FPS gate */
static int       g_show_msgbox;
static int       g_allow_multi;
static int       g_skip_cd;
static int       g_skip_ddraw;
static int       g_no_present;
static int       g_no_title_scene;
static unsigned  g_max_frames;
static int       g_launcher_mode = DEFAULT_LAUNCHER_MODE;
static int       g_shutdown;
static HANDLE    g_singleton_mutex;
static zdd      *g_zdd;

/* sotesd.dll — the bulk-data resource DLL that holds every title sprite
 * (logo 0x49f, backgrounds 0x91b/0x91c, menu cursor, …).  Retail's launcher
 * (FUN_005a4770 @ 0x5af5fc) LoadLibraryA's it and stashes the HMODULE in
 * DAT_008a6e74, which the boot driver then passes to every sprite/font/sound
 * registrar as the `settings` (PE-resource source) argument.  We mirror that:
 * load it once after the game-dir is anchored, register the title banks, and
 * free it at shutdown.  NB despite the `sotesp_module` parameter name on the
 * registrar, retail sources slot 0 (id 0x90b) from this same handle. */
static HMODULE   g_sotesd;

/* Alpha-ramp pointer tables — retail's DAT_008a92b8 (ramp_a) / DAT_008a9308
 * (ramp_b) are 20-entry tables of pointers to blend descriptors.  The
 * descriptors themselves are built by pd_boot_init_slots into the static
 * g_pd_boot_group_a/_b arrays; these tables expose them as the
 * `const zdd_blend_desc *const *` the render wrappers index (PdBlend aliases
 * the retail blend-descriptor layout that zdd_alpha_blit reads — mode/state at
 * +0, the three channels at +0x04/0x18/0x2c).  Filled once by
 * init_alpha_ramps(); they back the menu cursor, the sprite-level fades, and
 * (once their arms are wired) the logo/sparkle alpha draws. */
static const zdd_blend_desc *g_ramp_a[PD_BOOT_GROUP_A_COUNT];
static const zdd_blend_desc *g_ramp_b[PD_BOOT_GROUP_B_COUNT];
static int       g_ramps_built;

/* The title-scene drive (the caller side of FUN_0056aea0).  Active once
 * init_title_drive succeeds; one main_loop_body iteration runs one
 * title_scene_step against it.  --no-title-scene falls back to the legacy
 * minimal present loop (a black/uninitialised window, zero DDERR/frame). */
static title_drive g_drive;
static int         g_drive_active;

/* The new-game config scene drive (the caller side of FUN_00565d10 / 0x564780
 * case 0x24).  Entered when the title menu commits Start (app_flow NEW_GAME);
 * one main_loop_body iteration runs one newgame_drive_step.  On BACK it tears
 * down + re-displays the title; on START it hands off to the prologue cutscene. */
static newgame_drive g_newgame_drive;
static int           g_newgame_active;

/* The Elemental-Stone intro cutscene drive (FUN_0056cd20).  Entered when the
 * new-game scene commits Start Game; one main_loop_body iteration runs one
 * prologue_drive_step.  On DONE the game proper (0x59ec30) is unported, so it
 * re-displays the title for now; on ABORT (id 0x22) it re-displays the title. */
static prologue_drive g_prologue_drive;
static int            g_prologue_active;

/* The in-game map drive (0x59f2c0 seam).  Entered when the prologue cutscene
 * completes (3rd beat → PROLOGUE_DONE → enter_game).  One main_loop_body
 * iteration runs one game_drive_step.  The in-game engine is unported, so the
 * drive renders the faithful black map-load frame (the engine + render dispatch
 * 0x5a00c0 are the next port); see game_drive.h / docs/findings/in-game-intro.md. */
static game_drive     g_game_drive;
static int            g_game_active;

/* The in-game town BACKDROP scene (the decode → grid → walk → present pipeline,
 * composed in town_render.{c,h}).  Loaded in enter_game from the map's DATA
 * resource; game_render walks it each frame through the live-verified first-frame
 * camera MAP_RENDER_CAM_TOWN_3F2.  The map-data resource (DATA 1022 for the
 * opening town) lives in the ORIGINAL sotes.exe's .rsrc — distinct from
 * sotesd.dll (the sprite banks) — so it loads from a separate datafile handle
 * (g_sotes_exe), the engine-time module DAT_008a6e7c the ckpt-56 finding pinned. */
static town_render    g_town;
static int            g_town_loaded;
static uint32_t       g_loaded_room_key;  /* the room key whose backdrop is loaded */
static game_world     g_world;       /* the room registry (lazily built)         */
static int            g_world_built;
static HMODULE        g_sotes_exe;   /* original sotes.exe as a datafile (.rsrc) */

/* The room's CHARACTER-band actors (0x58d460 -> 0x431e30 spawn, ported
 * in actor_spawn.c).  Populated in enter_game from g_town.map; walked each frame
 * by game_actor_walk (emitting mode-0 keyed nodes into the town draw_pool between
 * the tile walk and the present).  For DATA 1022: 32 actors, of which 5 draw (the
 * villagers, bank 0x16c) — the other 27 are invisible volumes (quirk #80). */
static actor_spawn_pool g_actors;
static int              g_actors_loaded;

/* The room's STRUCTURE-band scenery (0x58d460 -> 0x438a60 spawn, ported in
 * actor_spawn.c).  Fully map-driven (engine-quirk #84): the foreground TREE
 * (0xec55), bg decorations (0xec6a), fg hedges (0xec60) — 39 static single-cel
 * objects for DATA 1022.  Rendered by the same actor_render_static (0x493230's
 * static blit is bit-identical), at the structure layers 8 (behind the cast) /
 * 15 (in front), which the layer-ordered present interleaves correctly. */
static actor_spawn_pool g_structs;
static int              g_structs_loaded;

/* The EFFECT band (0x41f200 spawn / 0x493ba0 render) — the standing townsfolk in
 * the square.  Map-driven (world = (map - dst) * 100), rendered by the same
 * actor_render_static (the 0x493ba0 static arm reduces to describe + emit for a
 * plain townsperson — one mode-0 keyed cel each), at layer 13.  Frozen on the
 * idle clip's frame 0 for now (Phase 1b animates).  engine-quirk #84. */
static actor_spawn_pool g_effects;
static int              g_effects_loaded;

/* Phase 2b: the per-room CAST (the house/errands family — Arche + her parents).
 * Spawned by actor_spawn_room_cast on the room swap, rendered + animated in the
 * NON-town rooms (the town cast band g_effects is suppressed there).  Reuses the
 * town cast's persisted banks (0x8b/0xe3/0xb5) + flip table.  These are the
 * persistent entities retail carries across the light room swap (quirk #103);
 * PORT-DEBT(cutscene-party-chars) — captured positions/clips for now. */
static actor_spawn_pool g_room_cast;
static int              g_room_cast_loaded;

/* Phase 2b: the FREEROAM hand-off — controllable Arche in the errands room
 * (0x334dc).  When the cutscene chain completes the port hands control to the
 * bit-exact mover (character_step, src/character.c): g_freeroam_char is her mover
 * state, driven by the live axis_held (g_game_drive.input — the held-trace replay
 * or the live keyboard producer); g_freeroam_actor/_rs render her body bank 0x8b
 * with the walk/idle clip selected from the mover state.  Her errands spawn is
 * world (19200,52000) facing right (runs/freeroam-walk + control-path-gt; projects
 * to screen (162,336) at the errands cam (0,16000), == retail).  The control path
 * is the +0x200==0 char-AI hand-off (quirk #103); this REPLACES the removed ckpt-120
 * CHAR_CONTROL_ARM_FRAMES MVP — retiring PORT-DEBT(char-control-trigger). */
static character          g_freeroam_char;
static actor              g_freeroam_actor;
static actor_render_state g_freeroam_rs;
static int                g_freeroam_active;
#define FREEROAM_ARCHE_SPAWN_WX  19200   /* errands spawn world_x (freeroam-walk)  */
#define FREEROAM_ARCHE_SPAWN_WY  52000   /* errands spawn world_y = ground_y       */

/* THEME 2: the town-intro "Arche runs to the house" run-off (cutscene-party-chars).
 * g_arche_slot is Arche's index in g_effects (the cast member on body bank 0x8b);
 * g_arche_runoff drives her clip + world_x during the L7->L8 beat.  See
 * actor_spawn.h (arche_runoff_*) for the RE + the faithful/stand-in boundary. */
static int          g_arche_slot = -1;
static arche_runoff g_arche_runoff;

/* USER studio notes #3-5: the house Arche TURN — 1 while her one-shot turn clip
 * plays (set by the CS_ACT_ACTOR_TURN drain on the house L5→L6 advance, cleared
 * when the clip finishes and she settles to the post-turn standing idle).  The
 * room-cast Arche is HOUSE_CAST[0] (bank 0x8b). */
static int          g_arche_house_turning;

/* The 4 town BUTTERFLIES' per-sim-tick LCG behaviour (engine-quirk #95) — the
 * EFFECT band's ONLY per-tick RNG consumer (0x47b990's 0xe29a case).  Registered
 * by actor_spawn_effect_from_map (their 0xc874 move-freq, in map order); stepped
 * once per sim-tick in game_actor_update BEFORE the particle emitters so the
 * shared LCG stream stays aligned with retail (the fountain/sky positions read it
 * downstream). */
static butterfly_pool   g_butterflies;

/* The town's IRREGULAR per-tick RNG timers (engine-quirk #95) — the two 0x5531b0
 * ambient SOUND emitters (0x1136f/0x11370), the wagon 0x1872d's idle-wander, and
 * the 0x467380 (0xe2a5) event timer.  Each fires once in the settled-town window
 * (ticks 189/33/134/183), consuming the LCG so the fountain/sky stay aligned past
 * the REVEAL.  Stepped in band order around the emitters in game_actor_update. */
static ambient_pool     g_ambient;

/* The particle band (0x13e0 DEVICE pool / 0x493480 render) — the FOUNTAIN SPRAY.
 * The fountain prop 0x112e5 (a CHARACTER in g_actors) emits one 0x18708 water
 * droplet per sim-tick; particle_pool_step applies gravity/fade.  engine-quirk
 * #87; findings "The FOUNTAIN SPRAY".  g_fountain_cx/cy is the emitter's
 * anchor-center; g_fountain_counter is the +0x5c %3 velocity-cycle state. */
static particle_pool    g_fountain_pp;
static int              g_fountain_loaded;
static int32_t          g_fountain_cx, g_fountain_cy;
static int              g_fountain_counter;
/* The 0x557370 mode-1 (param_7==1) anchor CENTERS the spawn on the emitter
 * prop's BOX (decompile 0x557370:56-60):
 *     final_x = world_x + (+0xc)/2  + jitter_x
 *     final_y = world_y + (+0x10)/2 + jitter_y
 * GROUND TRUTH (runs/r7-anchor-retail, the new 0x557370 field-spec; R7): the
 * fountain prop (0x18708 emitter) reads world=(176000,41600), box +0xc/+0x10 =
 * (6400,6400) — and the PORT's prop world is byte-identical (176000,41600), so
 * the only free variable is the offset.
 *
 * EMPIRICAL anchor = +1600 on BOTH axes (NOT the decompile's +0xc/2 = 3200).
 * Proof: a port|retail water-droplet blit comparison at stamp-equal tick 30
 * (runs/trace-studio/intro-1 port call-trace vs runs/r7-anchor-retail; both
 * render exactly 27 droplets, same cel sheet res 1032 → identical cel offset,
 * same camera since R6 background is differ_px==0).  At offset +3200 the port
 * spray centroid sat +14.6px right / +16.2px BELOW retail's; backing that out
 * (centroidΔ × 100, jitters cancel) puts retail's offset at ~1740/1580 ≈ 1600,
 * i.e. +0xc/4.  So the decompile reads /2 but the rendered spawn matches at /4.
 * PORT-DEBT(fountain-anchor) — OPEN 2× discrepancy: the missing factor is either
 * a second halving in the 0x557550/0x426620 water-config path (not in the cases
 * read so far) or a doubled +0xc unit; pin it before claiming the formula.  The
 * +1600 value retires the old curve-fit X=+1245 / missing-Y (R7 proved those
 * wrong: the spray sat too high-left) and matches the spray centroid. */
#define FOUNTAIN_EMIT_X_OFF 1600   /* empirical (= +0xc/4); see the 2× note above */
#define FOUNTAIN_EMIT_Y_OFF 1600   /* empirical (= +0x10/4); R7 tick-30 blit match */
/* The 0x112e5 emitter's +0x5c velocity-cycle counter.  particle_fountain_emit
 * mirrors retail byte-for-byte (54f980:218-285): increment FIRST
 * (counter=(counter+1)%30) THEN switch(counter%3) — 1 = right (0x453960
 * 20000,10000,-80000,-10000), 2 = weak-left (-10000,..), 0 = left-strong
 * (inline, no 0x453960).  So init 0 already yields cycle (k+1)%3 at emit #k,
 * matching the retail 0x453960 arg trace (runs/r7-vel-retail: abs_tick%3==0 →
 * right, ==1 → weak-left, ==2 → the inline case-0 with no scatter call).  Init 0
 * is faithful; the R7 "fake X-mirror" was the emit-then-step ORDER (now fixed in
 * game_actor_update), not this phase. */
#define FOUNTAIN_CYCLE_INIT 0

/* The 0x18704 SKY-AMBIENT emitters (0x112e2 / 0x54f980:150) — up to a handful of
 * CHARACTER props (census: 2 in the town); each spawns one 0x18704 particle every
 * 6th sim-tick into the SHARED particle band (g_fountain_pp = the whole +0x13e0
 * pool).  The 0x112e2 prop is an INVISIBLE trigger (no display cel), so its
 * render-state +0xc == 0 -> the 0x557370 mode-1 anchor is ZERO: the faithful
 * placement is the prop's exact world position (trace-confirmed: 0x18704 fresh
 * particles cluster at the prop +/- the jitter, no constant offset).  Cached
 * centers + per-emitter +0x5c counters.  findings "The SKY-AMBIENT particles". */
#define SKY_EMIT_MAX   8
static int32_t g_sky_cx[SKY_EMIT_MAX], g_sky_cy[SKY_EMIT_MAX];
static int     g_sky_counter[SKY_EMIT_MAX];
static int     g_sky_emit_count;

/* The actor mirror/flip table — the port stand-in for retail's global
 * DAT_008a8440 (a bank-indexed array of sprite-group frames-per-direction that
 * FUN_0044d160 adds to the frame on the facing==3 mirror arm).  Filled from the
 * EFFECT defs in enter_game (only the town villager banks; all others 0 = no
 * mirror) and passed to game_actor_walk's actor_render_static calls.  Sized to
 * AR_SPRITE_SLOT_COUNT so any actor bank indexes in-bounds. */
static int16_t g_actor_flip_table[AR_SPRITE_SLOT_COUNT];

/* The LIVE in-game camera (the room-state's +0x104c view object).  enter_game
 * spawn-snaps it to the hold origin (camera_apply_snap → cur=tgt=128000/12800);
 * game_render steps it each frame with camera_follow_step (FUN_0043d1d0) and
 * projects the backdrop through its current scroll — replacing the static
 * MAP_RENDER_CAM_TOWN_3F2.  At hold-end a synthetic timer issues the scripted
 * leftward pan (camera_apply_pan → tgt=12800/12800, speed 300; the 439690 +0x4c
 * command the unported cutscene script fires in retail).
 *
 * PORT-DEBT(ingame-camera-pan): the pan TRIGGER timing (the hold-frame count)
 * and the easer step CADENCE (retail steps per sim-tick; flips advance ~1 per 2
 * ticks, so the per-flip pan rate differs) are synthetic stand-ins for the
 * cutscene-script engine + the tick↔flip correlation — the easer FORMULA and
 * the target-setters themselves are bit-exact RE'd. */
static camera_view    g_game_camera;
static int            g_game_camera_armed;   /* live camera in use this scene   */
static uint32_t       g_game_camera_hold;    /* frames since the spawn snap     */
static mr_camera      g_game_camera_mr;       /* derived per-frame projection cam */

/* Easer-call count since boot — the port mirror of the retail capture's
 * sim_tick axis (the Frida agent counts 0x43d1d0 onEnter; 0 until the in-game
 * scene pumps).  Stamped into --capture-all BMP names (_t suffix) so the trace
 * studio can pair/attribute on the deterministic tick axis, not the Flip axis. */
static unsigned       g_sim_tick_count;

/* The establishing-shot cinematic letterbox bar heights (0x48c150's
 * in_ECX+0x44 / +0x48).  Set to LETTERBOX_INTRO_BAR (64) for the opening-town
 * intro = the quirk-#74 letterbox; 0 = no bars.
 * PORT-DEBT(ingame-letterbox): retail's heights are written by the unported
 * 0x5a00c0 cutscene script onto the scene-controller object; the port drives a
 * constant during the establishing shot (parallel to the camera-pan trigger
 * stand-in).  The grid-fill geometry itself is bit-exact RE'd (letterbox.c). */
static int            g_letterbox_top;
static int            g_letterbox_bottom;

/* The area-title banner ("Town of Tonkiness", banner.{c,h}; engine-quirk #96).
 * Declared here (before init_sprite_banks, which builds the GDI font) — the
 * render + arm wiring lives further down by the scene_fade sinks. */
static area_banner    g_banner;
static void          *g_banner_font;  /* HFONT — Courier New h20 w10 (DAT_008a9274[6]) */
static int            g_banner_armed; /* one-shot arm latch, reset per enter_game */
#define BANNER_SCROLL_SLOT  53        /* res 0x449 / the 0x8a7714 bank */
/* The scene-fade sinks (defined with their wiring further down, but the
 * grade skip-list above needs the slot ids early): retail binds BOTH via
 * the plain getter FUN_004184a0(0) at 0x48e920:37/66 — UNGRADED (the
 * quirk-#96 family; graded masks read one 5-bit step weak → the R6-B
 * frontier-band residual). */
#define SCENE_FADE_ALPHA_SLOT 40   /* 0x8a76e0 — res 0x458, frame = alpha level */
#define LETTERBOX_BANK_SLOT   41   /* 0x8a76e4 — res 0x583, the opaque black cel */
#define FIRE_BANK_SLOT       406   /* 0x8a7c98 — res 0x40a, the errands fireplace fire
                                    * (additive EFFECT sheet, bound via the plain getter
                                    * 0x418470(0) like the UI sheets => NOT colour-graded;
                                    * the port's global 8bpp grade over-darkens it). */
/* The banner's first alpha step lands at sim tick 42 on retail — TICK-AXIS
 * calibrated on trace-studio intro-1 per-present luminance: the alpha VALUE
 * sequence is bit-exact both sides; at +78 the port's first step landed t40
 * (2 ticks early), at +80 t41 (1 early — both fade-in AND fade-out edges lead
 * by exactly 1, the dt-probe plateaus from the 2.5-tick alpha-ramp
 * quantization hid the second tick); +82 puts arm+first-step on tick 42.
 * (The old flip-axis calibration "enable 0->1 between flips 1511/1515"
 * absorbed retail's present-coalescing.)  PORT-DEBT(banner-trigger): the real
 * source is the scene script (the same unported source as the camera-pan +
 * letterbox triggers). */
#define BANNER_ARM_FRAMES   82u

/* The in-game DIALOGUE BOX (dialogue.{c,h}; the town-intro line 1).  Banks:
 * the bubble 9-slice + corner/tail cels = res 0x456 (pool slot 50, the
 * DAT_008a7708 bank), the name tab = res 0x44a (slot 52, DAT_008a7710), the
 * portrait bust = res 0x7ef (slot 663, 24bpp magenta-keyed), and the advance
 * arrow = res 0x3e8 sliced 32x32 (the widget manager's own bank at god+0xb8c,
 * not a pool slot — registered standalone below). */
static dialogue_box   g_dialogue;        /* the box widget (rendered by game_render_dialogue) */
static cutscene       g_cutscene;        /* the town-arrival line SEQUENCER, drives g_dialogue */
static void          *g_dialogue_font;   /* HFONT — Courier New h18 w7 (textout probe) */
static int            g_cutscene_armed;  /* one-shot arm latch, reset per enter_game */
static int            g_errands_dlg_pending; /* set at chain-complete; arms the errands  *
                                            * opening dialogue once the entry reveal      *
                                            * recedes (deferred so it plays AFTER the     *
                                            * fade-from-black, like retail). reset/enter   */
#define DIALOGUE_BOX_BANK_SLOT      50   /* res 0x456 (DAT_008a7708) */
#define DIALOGUE_TAB_BANK_SLOT      52   /* res 0x44a (DAT_008a7710) */
#define DIALOGUE_PORTRAIT_BANK_SLOT 663  /* res 0x7ef (Father bust)  */
/* The bubble's first visible change lands at sim tick 645 on retail —
 * TICK-AXIS calibrated on trace-studio intro-1: the pop-in/portrait-fade
 * change sequence is pixel-identical both sides but at the old +1298 (arm
 * tick 650, first change t653) the port ran a constant 8 ticks late; arm
 * tick 642 (+1282) puts the port's first change at t645 = retail's.  (The
 * old flip-axis "+1300/+1304" readings absorbed retail's present-coalescing
 * drift.)  PORT-DEBT(dialogue-trigger): the real source is the town script's
 * beat sequence (0x4d7d80 case 0x334be -> 0x439690), the same unported
 * driver as the banner/camera-pan triggers. */
#define DIALOGUE_ARM_FRAMES 1282u

/* Flips the camera holds at the spawn origin before the scripted pan begins.
 * TICK-AXIS calibrated on trace-studio intro-1: retail's camera first moves at
 * sim tick 93 vs the port's 94 at the old 184, and at tick-equal frames the
 * whole pan matched retail tick T-1 (differ ~= the particle residual only) —
 * the pan command belongs on tick 92, hold 182.  (The original flip-axis
 * reading "Flip 1617 = PAN, game_enter@1433 → 184" had a 1-tick error from
 * retail's present-coalescing.)  Even (so the trigger Flip is also a sim
 * tick — see game_camera_step).  Synthetic stand-in for the cutscene-script
 * trigger source; see PORT-DEBT(ingame-camera-pan) above. */
#define GAME_CAMERA_HOLD_FRAMES 182u

/* The in-game palette color-grade LUT (src/color_grade.{c,h}).  Retail's in-game
 * render paths (FUN_00417c40 parallax + FUN_00490f30 tilemap) remap every sprite
 * palette channel through this tone curve (darker + more saturated); the title /
 * new-game / prologue paths do not.  g_color_grade_on is armed only when
 * enter_game runs, AFTER the title sheets are already converted — so those stay
 * identity (bit-exact) and only the lazily-decoded in-game banks are graded. */
static uint8_t        g_color_lut[256];
static int            g_color_grade_on;

/* The GDI HFONT slot the new-game config menu renders with: Courier New 7×18 =
 * ar_register_fonts slot 5 ({w=7,h=0x12,family=2}); the captured golden's
 * TextOutA stream selects exactly that LOGFONTA (face "Courier New", h 18,
 * w 7).  Fallbacks cover a partial font registration. */
#define NEWGAME_FONT_SLOT 5

/* The phase-7 sparkle-particle pool (FUN_0056c070 spawns into it; the sink's
 * FRAME_END composes it via title_compositor_draw == FUN_0056c180).  Owned
 * here so the spawn hook (which carries no context arg) can reach it, and so
 * its storage outlives the drive.  Reset at each drive init. */
static title_particle_pool g_particles;

static void emit_anchor(const char *name);   /* TAS alignment seam (see below) */

/* The spawn_sparkle hook (title_scene_hooks): phase-7's per-tick
 * FUN_0056c070 call with the title constants baked in. */
static void drive_spawn_sparkle(int32_t intensity)
{
    /* The first phase-7 spawn is the TAS `subtitle_anim_start` anchor — the
     * same point the retail agent stamps at its first FUN_0056c070 call.  This
     * is tick 0 of the sparkle/subtitle animation, the intro's alignment seam.
     * Emit BEFORE the spawn so the RNG state matches retail's pre-spawn read. */
    static int spawned_once = 0;
    if (!spawned_once) {
        spawned_once = 1;
        emit_anchor("subtitle_anim_start");
    }
    title_particle_spawn_title(&g_particles, intensity);
}

/* Per-frame particle update (0x56ba69) — rise/age/cull every update tick. */
static void drive_update_particles(void)
{
    title_particle_pool_update(&g_particles);
}

/* The post-update side effect (0x56c930): ramp the spawned menu node's input
 * gate (node->field_54 → 1000) so the menu becomes navigable a few frames after
 * it appears.  Without this the latch stays gated closed and injected nav input
 * (--input-trace) never moves the cursor (quirk #34/#59). */
static void drive_post_update(void)
{
    menu_owner_transition_step(&g_drive.owner);
}

/* Borrowed by the drive each frame (must outlive it).  spawn_sparkle +
 * update_particles drive the phase-7 twinkles; post_update opens the menu-input
 * gate; the remaining outer-loop side effects stay deferred (see HANDOFF). */
static const title_scene_hooks g_title_hooks = {
    .spawn_sparkle    = drive_spawn_sparkle,
    .update_particles = drive_update_particles,
    .post_update      = drive_post_update,
};

/* --input-trace <file.jsonl> — port-side deterministic input replay (the
 * harness's {frame,ids} format).  Injected into the drive's input ring keyed
 * on the present (Flip) count g_present_frame, which drive_present bumps. */
static char              g_input_trace_path_buf[1024];
static const char       *g_input_trace_path;
static struct input_trace g_input_trace;
static int               g_input_trace_active;
static uint32_t          g_present_frame;

/* --held-trace <file.jsonl> — port-side deterministic HELD-AXIS replay (the
 * harness's {frame,keys} level format; held_trace.h).  Rebuilds the drive's
 * input_mgr.axis_held[] every frame so held WALKING replays deterministically,
 * the level counterpart of --input-trace's discrete ring presses. */
static char              g_held_trace_path_buf[1024];
static const char       *g_held_trace_path;
static struct held_trace g_held_trace;
static int               g_held_trace_active;

/* The LIVE keyboard producer (0x46a880; src/input_live.{c,h}).  When NO replay
 * is active and the window is focused, real keys fill the active drive's input
 * manager each frame — the INTERACTIVE path (the replay is the deterministic
 * parity path; the two are mutually exclusive, see feed_input). */
static struct input_live g_input_live;
/* Set by WM_ACTIVATEAPP (defined in app_pump.c, the pump module) — the live
 * producer's focus gate, matching retail's input pause on deactivate. */
extern uint32_t g_app_active_flag;

static uint32_t  g_frame_counter;
static uint32_t  g_base_time_ms;
static uint32_t  g_total_ms;

/* --call-trace <path> [--call-trace-frames i,j,k] — port-side counterpart
 * of the Frida agent's call_trace emitter (see src/call_trace.h).  The
 * path is copied into a static buffer because parse_cmdline's token
 * pointers alias a stack buffer. */
#define CALL_TRACE_FRAMES_CAP 256
static char      g_call_trace_path_buf[1024];
static const char *g_call_trace_path;
static unsigned  g_call_trace_frames[CALL_TRACE_FRAMES_CAP];
static size_t    g_n_call_trace_frames;

/* --capture-frames "60,200,…" [--capture-dir <path>] — port-side frame
 * capture (the counterpart of the Frida harness's --capture-frames).  At
 * each requested Flip (present) frame, GetDC the composed primary surface,
 * BitBlt it into a 24bpp DIB, and write <dir>/port_frame_NNNNN.bmp.  Lets us
 * diff the port's actual pixels against the retail goldens in runs/. */
#define CAPTURE_FRAMES_CAP 4096
static unsigned  g_capture_frames[CAPTURE_FRAMES_CAP];
static size_t    g_n_capture_frames;
/* --capture-all: dump EVERY present frame (a dense video capture; avoids a huge
 * --capture-frames whitelist).  Optional "--capture-all-from N" lower bound. */
static int       g_capture_all;
static unsigned  g_capture_all_from;
static char      g_capture_dir_buf[1024] = ".";
static const char *g_capture_dir = g_capture_dir_buf;

/* --render-glyph-test [--glyph-test-font N] — the pixel-diff gate for the GDI
 * text renderer (glyph_render.c).  When set, after the fonts are registered we
 * build a standalone menu_ctrl/menu_node, lay the title-menu labels into its
 * cells via glyph_cell_layout, render the grid into an offscreen DIB-section
 * DC with glyph_gdi_ops_win32 (TRANSPARENT bk), and save it as a BMP — then
 * exit without entering the title scene.  The retail side renders the same
 * labels with the same font (CreateFontIndirectA → identical LOGFONTA, so the
 * GDI rasterization is bit-identical) for a differ_px diff.  Default font slot
 * 2 (ar_register_fonts: w=7,h=0x10); override with --glyph-test-font. */
static int       g_render_glyph_test;
static int       g_glyph_test_font = 2;

/* --osr-replay <file.osr> [--osr-out <dir>] [--osr-replay-frames i,j] — Trace
 * Studio v2 M4: reconstruct frames from a captured .osr draw stream.  After the
 * DDraw boot we stream the .osr (osr_recon.c) and replay each frame's blits +
 * GDI text onto the offscreen primary surface, snapshotting the wanted frames to
 * <dir>/recon_<flip>_t<tick>.bmp, then exit.  An empty frame whitelist renders
 * every frame.  Forces a headless windowed boot (no title scene). */
#define OSR_REPLAY_FRAMES_CAP 4096
static char       g_osr_replay_path_buf[1024];
static const char *g_osr_replay_path;
static char       g_osr_out_dir_buf[1024] = ".";
static unsigned   g_osr_replay_frames[OSR_REPLAY_FRAMES_CAP];
static size_t     g_n_osr_replay_frames;

/* --osr-emit <file.osr> [--osr-scenario <name>] — Trace Studio v2 M5: the port
 * writes the SAME .osr draw stream the retail capture proxy produces, from its
 * own sinks (zdd blits/clears, GDI text/fonts, anchors, seed pins), so the M6
 * studio can tick-join the two sides.  All sinks gate internally on the open
 * file — zero cost when the flag is absent. */
static char       g_osr_emit_path_buf[1024];
static const char *g_osr_emit_path;
static char       g_osr_scenario_buf[40];
static int        g_osr_state_on;     /* --osr-state: emit OSR_STATE (rng census) */

static LRESULT CALLBACK wndproc(HWND, UINT, WPARAM, LPARAM);
static int  register_window_class(void);
static int  create_main_window(void);
static void parse_cmdline(LPSTR);
static void init_game_dir(void);
static void resolve_launch_path(char *buf, size_t buflen, const char *what);
static int  acquire_singleton(void);
static void release_singleton(void);
static void main_loop_body(void);
static void frame_limiter(void);
static void log_line(const char *fmt, ...);
static int  init_ddraw(void);
static void shutdown_ddraw(void);
static void sync_window_position(void);
static int  build_title_drive(int skip_intro);
static void init_title_drive(void);
static void reenter_title(void);
static void enter_newgame(void);
static void leave_newgame_to_title(const char *why);
static void newgame_render(void *user);
static void init_sprite_banks(void);
static void render_glyph_test(void);
static void init_alpha_ramps(void);
static void maybe_capture_frame(unsigned flip_frame);
static int  capture_primary_to_bmp(const char *path);
static void drive_present(void *user);
static void drive_log_flip(void *user);
static uint32_t game_rng_seed(void);

/* The .osr emitter's injected surface reader (M5 SHEET grab): lock a source
 * zdd_object and expose its pixels/geometry; done() unlocks.  The grab is
 * once-per-surface (cached by the emitter), so a slow video-memory Lock is
 * amortized.  Bit depth comes from the ZDD's display format (windowed boot =
 * 16bpp RGB565; every cel is converted to it at decode). */
static int osr_surf_read(zdd_object *obj, osr_emit_surf *out)
{
    if (!zdd_object_lock(obj)) return 0;
    void *buf = NULL;
    int32_t pitch = 0, height = 0;
    zdd_object_get_locked_info(obj, &buf, &pitch, &height);
    int32_t width = zdd_object_get_locked_width(obj);
    if (buf == NULL || pitch <= 0 || height <= 0 || width <= 0) {
        zdd_object_unlock(obj);
        return 0;
    }
    out->pixels   = buf;
    out->pitch    = (uint32_t)pitch;
    out->w        = (uint32_t)width;
    out->h        = (uint32_t)height;
    out->bitcount = (uint16_t)(g_zdd != NULL && g_zdd->pixel_format_bpp > 0
                               ? g_zdd->pixel_format_bpp : 16);
    return 1;
}
static void osr_surf_done(zdd_object *obj)
{
    zdd_object_unlock(obj);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrev; (void)nCmdShow;
    g_hInstance = hInst;

    /* Our stderr lines use UTF-8 ("—" em-dashes etc.).  Without this
     * the console-subsystem build renders them as CP437 mojibake
     * (e.g. "ΓÇö" for "—").  No-op for the GUI-subsystem build when
     * no console is attached. */
    SetConsoleOutputCP(CP_UTF8);

    parse_cmdline(lpCmdLine);

    /* Absolutize file-path CLI args while the CWD is still the LAUNCH dir.
     * init_game_dir() chdir's to the (Windows) game dir below, but
     * --input-trace is opened later (init_title_drive) — so a relative or
     * WSL-form path that resolved fine at launch would miss after the chdir
     * (the recurring "failed to load input trace" footgun).  --call-trace is
     * opened just below (pre-chdir) but we resolve+log it too so its output
     * location is never a mystery. */
    resolve_launch_path(g_input_trace_path_buf, sizeof g_input_trace_path_buf,
                        "--input-trace");
    resolve_launch_path(g_held_trace_path_buf, sizeof g_held_trace_path_buf,
                        "--held-trace");
    resolve_launch_path(g_call_trace_path_buf, sizeof g_call_trace_path_buf,
                        "--call-trace");
    resolve_launch_path(g_osr_replay_path_buf, sizeof g_osr_replay_path_buf,
                        "--osr-replay");
    resolve_launch_path(g_osr_emit_path_buf, sizeof g_osr_emit_path_buf,
                        "--osr-emit");

    /* --osr-replay is a self-contained reconstruction mode: it builds its own
     * source surfaces + fonts from the .osr and snapshots to BMP, so it needs
     * neither a visible window nor the title scene.  Force headless. */
    if (g_osr_replay_path != NULL) {
        g_hide_window    = 1;
        g_no_title_scene = 1;
    }

    /* Open the port-side call trace (no-op unless --call-trace given).
     * Done before any traced function (boot DDraw path) can run. */
    call_trace_init_from_cli(g_call_trace_path,
                             g_call_trace_frames, g_n_call_trace_frames);

    /* Open the port-side .osr emitter (no-op unless --osr-emit given).  Done
     * before init_ddraw so the boot fonts (ar_gdi_create_font) land as FONT
     * records; the primary-dest filter + surface reader bind after the ZDD
     * boot below.  Header mirrors the proxy's: seed-pinned, windowed RGB565. */
    if (g_osr_emit_path != NULL) {
        osr_emit_open(g_osr_emit_path, game_rng_seed(), OSR_FLAG_SEED_PIN,
                      g_osr_scenario_buf, DEFAULT_WIDTH, DEFAULT_HEIGHT,
                      OSR_PIXFMT_RGB565);
        osr_emit_state_enable(g_osr_state_on);   /* M8: arm the OSR_STATE pass */
    }

    /* Install MessageBox→stderr hook as early as possible.  Any modal
     * popup between here and uninstall (in shutdown) gets redirected
     * instead of blocking the process invisibly. */
    if (!g_show_msgbox) {
        dev_hooks_install_messagebox();
    }

    /* Single-instance enforcement catches the dev-loop foot-gun of
     * accidentally leaving a stray .exe alive from a previous SIGKILL'd
     * run.  Without this, the second invocation silently shadows the
     * first and you spend an hour wondering why your changes don't take
     * effect. */
    if (!g_allow_multi && !acquire_singleton()) {
        return 2;
    }

    if (!g_skip_cd) init_game_dir();

    log_line("OpenSummoners starting (hide_window=%d, max_frames=%u)",
             g_hide_window, g_max_frames);

    timeBeginPeriod(1);

    if (!register_window_class()) {
        log_line("RegisterClassA failed: %lu", GetLastError());
        timeEndPeriod(1);
        return 1;
    }

    if (!create_main_window()) {
        log_line("CreateWindowExA failed: %lu", GetLastError());
        timeEndPeriod(1);
        return 1;
    }

    if (!g_hide_window) {
        ShowWindow(g_hwnd, SW_SHOW);
        UpdateWindow(g_hwnd);
    }

    /* DDraw init — mirrors retail FUN_00562ea0's bootstrap order:
     *   1. FUN_005b7ee0  → zdd_create            (DirectDrawCreateEx)
     *   2. FUN_005b89d0  → zdd_set_coop_level    (SetCooperativeLevel)
     *   3. FUN_00582e90  → cs_dispatch_create_screen (per-mode CreateScreen)
     * Fullscreen flag for SetCooperativeLevel is (launcher_mode != 2)
     * — only Windowed runs DDSCL_NORMAL.  cs_dispatch_create_screen
     * returns void; on failure it ExitProcess(0)'s via cs_exit. */
    /* Bracket the one-shot boot path as call-trace frame 0 so the DDraw
     * bootstrap (zdd_create / SetCooperativeLevel / CreateScreen
     * dispatch) shows up in the diff against retail's frame-0 boot
     * calls.  The per-frame loop below re-opens its own frames. */
    call_trace_begin_frame(0);
    if (!g_skip_ddraw && !init_ddraw()) {
        call_trace_end_frame();
        timeEndPeriod(1);
        return 1;
    }

    /* Stamp the ZDD's screen_pos_x/y from the window's actual client-
     * area top-left in screen coords.  Mode 2 (Windowed) present needs
     * this to BitBlt into the right desktop location — retail keeps it
     * in sync from somewhere we haven't traced yet (likely WM_MOVE
     * inside FUN_005b9130 or a sibling).  Updated again on WM_MOVE
     * below; this initial sample covers the window's spawn position. */
    sync_window_position();

    /* Bind the present target to our window so the mode-2 present BitBlts into
     * the *window* DC (retail's GetDC(hwnd) path, quirk #55) instead of the
     * desktop — a visible/focused window then composites through DWM without
     * flicker.  Harmless under --hide-window (drive_present skips the present
     * there anyway). */
    if (g_zdd != NULL)
        zdd_set_present_hwnd(g_hwnd);

    /* Bind the .osr emitter to the live ZDD: the primary compose surface is
     * the BLIT/CLEAR/TEXT dest filter (dst_handle 1), and the SHEET grab reads
     * source pixels through the lock-based reader below. */
    if (osr_emit_is_active() && g_zdd != NULL && g_zdd->primary_obj != NULL) {
        osr_emit_set_primary(g_zdd->primary_obj);
        osr_emit_set_surf_reader(osr_surf_read, osr_surf_done);
    }

    /* --osr-replay: reconstruct frames from a captured .osr, then exit.  Runs
     * before init_title_drive (forced off above) — it composes onto the same
     * offscreen primary surface the title scene would, building its own source
     * surfaces (SHEET) + HFONTs (FONT) from the stream.  No sprite banks needed. */
    if (g_osr_replay_path != NULL && g_zdd != NULL && g_zdd->primary_obj != NULL) {
        osr_recon_run(g_zdd, g_zdd->primary_obj, g_osr_out_dir_buf,
                      g_osr_replay_path, g_osr_replay_frames,
                      g_n_osr_replay_frames);
        g_shutdown = 1;
    }

    /* Register the title sprite banks from sotesd.dll BEFORE the drive starts,
     * so the render sink's bank getter (ar_pool_get_slot 19/20) resolves to
     * populated slots instead of short-circuiting to NULL.  Without this the
     * whole decode→slice→8d chain never runs and the window stays blank. */
    if ((!g_no_title_scene || g_render_glyph_test) && !g_skip_ddraw && g_zdd != NULL)
        init_sprite_banks();

    /* --render-glyph-test: render the glyph grid into an offscreen DIB, dump a
     * BMP, then shut down before the title scene starts (the pixel-diff gate
     * for the GDI text renderer).  Runs after init_sprite_banks so the HFONTs
     * are registered. */
    if (g_render_glyph_test && !g_skip_ddraw && g_zdd != NULL) {
        render_glyph_test();
        g_shutdown = 1;
    }

    /* Stand up the title-scene drive (the caller side of FUN_0056aea0): bind
     * the render sink to the live primary surface, install the sprite-bank
     * self-decode hook, and allocate the scene object graph.  After this the
     * per-frame loop runs the title scene; --no-title-scene or a failed/absent
     * DDraw boot falls back to the legacy minimal present loop. */
    if (!g_no_title_scene && !g_render_glyph_test && !g_skip_ddraw && g_zdd != NULL)
        init_title_drive();

    call_trace_end_frame();   /* close boot frame 0 */

    g_base_time_ms = timeGetTime();

    /* g_max_frames == 0 → run until WM_QUIT or external close.  Non-zero
     * is the harness/smoke pattern that lets us exit voluntarily so
     * stderr flushes cleanly before the launcher's grace window. */
    while (!g_shutdown && (g_max_frames == 0 || g_frame_counter < g_max_frames)) {
        main_loop_body();
    }

    log_line("OpenSummoners exiting after %u frames (%u ms wall)",
             g_frame_counter, g_total_ms);

    if (g_prologue_active) {
        prologue_drive_shutdown(&g_prologue_drive);
        g_prologue_active = 0;
    }
    if (g_game_active) {
        game_drive_shutdown(&g_game_drive);
        g_game_active = 0;
    }
    if (g_town_loaded) {
        town_render_free(&g_town);
        g_town_loaded = 0;
    }
    if (g_world_built) {
        game_world_free(&g_world);
        g_world_built = 0;
    }
    if (g_sotes_exe != NULL) {
        FreeLibrary(g_sotes_exe);
        g_sotes_exe = NULL;
    }
    if (g_newgame_active) {
        newgame_drive_shutdown(&g_newgame_drive);
        g_newgame_active = 0;
    }
    if (g_drive_active) {
        title_drive_shutdown(&g_drive);
        g_drive_active = 0;
    }
    if (g_input_trace_active) {
        input_trace_free(&g_input_trace);
        g_input_trace_active = 0;
    }
    if (g_held_trace_active) {
        held_trace_free(&g_held_trace);
        g_held_trace_active = 0;
    }
    shutdown_ddraw();
    if (g_sotesd != NULL) {
        FreeLibrary(g_sotesd);
        g_sotesd = NULL;
    }
    call_trace_shutdown();
    osr_emit_close();
    timeEndPeriod(1);
    dev_hooks_uninstall_messagebox();
    release_singleton();
    return 0;
}

/* Build the ZDD + drive the per-mode CreateScreen dispatcher.  Logs
 * the chosen mode + the result of each step.  Returns 1 on success.
 *
 * On cs_dispatch_create_screen failure, the real Win32 cs_exit is
 * ExitProcess(0) — control never returns from that call.  We log
 * "dispatch returned" after it as a marker for the success path. */
static int init_ddraw(void)
{
    log_line("init_ddraw: launcher_mode=%d (0=Full 1=Safe 2=Wind 3=DB 4=Zoom)",
             g_launcher_mode);

    if (!zdd_create(&g_zdd)) {
        log_line("init_ddraw: zdd_create (DirectDrawCreateEx) failed");
        return 0;
    }

    int fullscreen = (g_launcher_mode != 2);
    if (!zdd_set_coop_level(g_zdd, g_hwnd, fullscreen)) {
        log_line("init_ddraw: SetCooperativeLevel(fullscreen=%d) failed",
                 fullscreen);
        zdd_destroy(g_zdd);
        g_zdd = NULL;
        return 0;
    }
    log_line("init_ddraw: SetCooperativeLevel ok (fullscreen=%d)", fullscreen);

    cs_dispatch_create_screen(g_zdd, g_launcher_mode,
                              DEFAULT_ZOOM_TARGET_W, DEFAULT_ZOOM_TARGET_H);
    log_line("init_ddraw: CreateScreen dispatch returned (success path)");
    return 1;
}

static void shutdown_ddraw(void)
{
    if (g_zdd != NULL) {
        zdd_destroy(g_zdd);
        g_zdd = NULL;
    }
}

/* TITLE_DRAW_FLIP → present the frame (retail FUN_005b8fc0).  --no-present
 * suppresses it (DDraw init still ran, the surface just isn't blitted). */
static void drive_present(void *user)
{
    (void)user;
    /* Field-bearing flow-trace probe at the Flip (retail FUN_005b8fc0, hooked
     * once-per-frame by the agent).  `rng` is the live LCG state (port rng
     * global == retail DAT_008a4f94 under --seed-pin) read at the frame
     * boundary — the once-per-frame determinism anchor.  A [data] divergence on
     * rng the frame AFTER a mismatched draw localizes a stray RNG consumption
     * (parity pillar 3).  This is the cross-side B2 probe: both sides emit one
     * rng event per frame at the same engine VA.  tools/flow/retail_fields.json
     * declares the matching retail read; docs/plans/trace-tooling-phase-b.md. */
    CALL_TRACE_BEGIN(0x5b8fc0);
    CALL_TRACE_HEX("rng", rng_peek_state());
    CALL_TRACE_END();
    /* The Flip count is the input-trace's frame axis (matching the harness's
     * Flip-anchored injection), so bump it on every present. */
    g_present_frame++;
    /* M8: push this frame's engine STATE (the RNG census + whatever else is
     * annotated) before the frame boundary; flushed as OSR_STATE after FRAMEBEG.
     * No-op unless --osr-state armed.  Add more osr_emit_state_field() calls here
     * as relevant game state is annotated (player px/py, scene id, flags, …). */
    osr_emit_state_field("rng",      OSR_ST_HEX, (int64_t)rng_peek_state(), 0.0);
    osr_emit_state_field("rngcalls", OSR_ST_INT, (int64_t)rng_call_count(), 0.0);
    /* The freeroam mover's live command + body — proves the input→command chain in
     * the real exe (cmd_pose flips 0/10/0xb on DOWN/UP; cmd_lr 0/5/6 on a dash). */
    osr_emit_state_field("fr_locked", OSR_ST_INT,
        (int64_t)(g_freeroam_active &&
                  (g_errands_dlg_pending || cutscene_active(&g_cutscene))), 0.0);
    osr_emit_state_field("fr_csln", OSR_ST_INT,
        (int64_t)(cutscene_active(&g_cutscene)
                      ? g_cutscene.room_idx * 10 + g_cutscene.line_idx : -1), 0.0);
    osr_emit_state_field("fr_pose",  OSR_ST_INT, (int64_t)g_freeroam_char.cmd_pose, 0.0);
    osr_emit_state_field("fr_sword", OSR_ST_INT, (int64_t)g_freeroam_char.sword_out, 0.0);
    osr_emit_state_field("fr_lr",    OSR_ST_INT, (int64_t)g_freeroam_char.cmd_lr,   0.0);
    osr_emit_state_field("fr_wx",    OSR_ST_INT, (int64_t)g_freeroam_char.world_x,  0.0);
    osr_emit_state_field("fr_vel",   OSR_ST_INT, (int64_t)g_freeroam_char.vel,      0.0);
    /* M5: the .osr frame boundary — PRESENT closes the open frame, FRAMEBEG
     * opens the next (the proxy's present-then-framebeg order; the draws under
     * FRAMEBEG(f) are issued after flip f, presented by flip f+1). */
    osr_emit_flip(g_present_frame, g_sim_tick_count);
    /* Capture BEFORE the flip: the fully-composed frame is on primary_obj's
     * surface here, exactly as the sink drew it this iteration. */
    maybe_capture_frame(g_present_frame);
    /* In Windowed mode (the drop-in default, launcher_mode 2) zdd_present
     * BitBlts the composed primary onto the *desktop* at the window's screen
     * position (zdd_desktop_present → GetDC(NULL)) — faithful to retail, which
     * paints into its window's client area.  But under --hide-window the window
     * is SW_HIDE'd while that desktop blit still paints a 640×480 rectangle on
     * the real screen every frame: the "hidden window flickers over my screen"
     * bleed-through.  Hidden ⇒ nothing to present into, so skip the blit; the
     * frame is already fully composed on primary_obj (captures read it above,
     * before this), so suppressing the present is lossless for headless runs. */
    if (!g_no_present && !g_hide_window && g_zdd != NULL)
        zdd_present(g_zdd);
}

/* TITLE_DRAW_LOG_FLIPPING → the engine's "Title Menu - Flipping" marker. */
static void drive_log_flip(void *user)
{
    (void)user;
    log_line("Title Menu - Flipping");
}

/* ─── 8d hook adapters (asset_register → ZDD) ────────────────────────
 *
 * These bridge the decoupled asset_register slicer to the live ZDD: the
 * per-cell surface builder, the per-sheet format conversion, and the
 * per-frame surface release.  Kept in main.c (not asset_register) so the
 * decoder TU stays ZDD-free. */

/* ar_frame_build_hook (0x5b9280) — build one cell's keyed DDraw surface. */
static void *title_frame_build(ar_sprite_slot *slot,
                               const struct bitmap_session *sheet,
                               int src_x, int src_y, int cell_w, int cell_h,
                               uint32_t colorkey, void *aux_entry)
{
    zdd *parent = (slot != NULL && slot->zdd != NULL) ? (zdd *)slot->zdd
                                                      : g_zdd;
    if (parent == NULL) return NULL;
    zdd_object *out = NULL;
    int ok = zdd_object_new_cell(parent, &out, sheet,
                                 src_x, src_y, cell_w, cell_h,
                                 (int32_t)colorkey,
                                 (int)slot->scale_flag,   /* videomem / create p5 */
                                 (int)slot->type,         /* trim_count */
                                 (const struct bs_trim_rect *)aux_entry);
    return ok ? (void *)out : NULL;
}

/* ar_frame_free_hook (FUN_005b9390 + delete) — release one built surface. */
static void title_frame_free(void *surface)
{
    zdd_object *o = (zdd_object *)surface;
    zdd_obj_destroy(&o);
}

/* ar_sheet_format_hook (slicer 0x4189f2 switch) — convert a decoded sheet
 * to the god-object's display depth before cells are built.  Windowed mode
 * runs at 16bpp, so a 24bpp title sheet is packed to RGB565 via the
 * descriptor zdd_bind_pixel_format stamped on the 16bpp boot. */
static void title_sheet_format(ar_sprite_slot *slot,
                               struct bitmap_session *sheet, uint32_t colorkey)
{
    zdd *z = (slot != NULL && slot->zdd != NULL) ? (zdd *)slot->zdd : g_zdd;
    if (z == NULL || sheet == NULL) return;

    int depth = z->pixel_format_bpp;                 /* [zdd+0x168] */
    int src   = (int)bs_get_bit_count(sheet);
    uint32_t key_color = (colorkey == 0x1ffffffu) ? 0u : 0xff00ffu;

    /* In-game palette color-grade (retail FUN_00417c40 / FUN_00490f30): remap
     * every palette channel through the tone-curve LUT BEFORE the 8bpp sheet is
     * packed to the display depth — matching retail's order (LUT the palette,
     * then convert), so the result is bit-exact (not LUT-after-565).  Only 8bpp
     * sheets carry a palette; only armed once in-game (g_color_grade_on).  8bpp
     * color-keying is by index, so grading the colors is key-safe.
     *
     * EXCEPT the plain-getter UI sheets: retail binds these via FUN_00418470(0)
     * (NO 0x417c40 grade descriptor), so their palettes are NOT graded —
     * grading renders them ~10% too dark:
     *   - the area-title banner scroll (slot 53 / res 0x449; differ_px=0 vs
     *     retail once skipped, ckpt 101);
     *   - the dialogue bubble 9-slice (slot 50 / res 0x456) + name tab
     *     (slot 52 / res 0x44a) — the widget tree resolves cels through the
     *     plain getter family (0x48d940's FUN_004184a0(0)); an exact-pixel
     *     match of the decoded tab against the live retail frame confirms the
     *     ungraded palette (565-quantized only).
     * PORT-DEBT(banner-grade): the faithful gate is "grade only via the
     * 0x417c40 getter" (the tiles/sky use it, the UI sheets don't); the port's
     * 8bpp grade is global, so the plain-getter sheets are skipped by slot. */
    if (g_color_grade_on && src == 8 &&
        slot != &g_ar_sprite_slots[BANNER_SCROLL_SLOT] &&
        slot != &g_ar_sprite_slots[DIALOGUE_BOX_BANK_SLOT] &&
        slot != &g_ar_sprite_slots[DIALOGUE_TAB_BANK_SLOT] &&
        slot != &g_ar_sprite_slots[SCENE_FADE_ALPHA_SLOT] &&
        slot != &g_ar_sprite_slots[LETTERBOX_BANK_SLOT] &&
        slot != &g_ar_sprite_slots[FIRE_BANK_SLOT] &&
        /* the font-texture / button-prompt UI banks (res 0x455 book+cursor, res
         * 0x6fa key-caps) — plain-getter sheets retail does NOT grade; the port's
         * global 8bpp grade was over-darkening the key-caps (USER: "dimmer than
         * retail", ckpt 153).  Same class as the dialogue box/tab + the fire. */
        slot != &g_ar_sprite_slots[AR_SPR_FONT_TEX_455] &&
        slot != &g_ar_sprite_slots[AR_SPR_KEYCAP_6FA])
        color_grade_apply_palette(sheet->palette, 256, g_color_lut);

    switch (depth) {
    case 8:
        break;                                       /* 8bpp display: no convert */
    case 16:
        if (src == 8 || src == 24)
            bs_convert_to_16bpp(sheet, (const uint8_t *)&z->color_desc,
                                colorkey, key_color);
        break;
    case 24:
        if (src == 8)
            bs_convert_8bpp_to_24bpp(sheet, colorkey, key_color);
        break;
    case 32:
        if (src == 8)
            bs_convert_8bpp_to_24bpp(sheet, colorkey, key_color);
        else if (src != 24)
            break;
        bs_convert_24bpp_to_32bpp(sheet);
        break;
    default:
        break;
    }
}

/* Load sotesd.dll and register the title sprite banks against the live ZDD.
 *
 * Mirrors retail's boot driver FUN_00562ea0 @ 0x5749b0 call:
 *   FUN_005749b0(ZDD=DAT_008a93cc, group=4, settings=DAT_008a6e74[sotesd])
 * where DAT_008a6e74 is the sotesd.dll HMODULE the launcher loaded.  The
 * registrar stamps g_ar_sprite_slots[0..29] + stragglers with their resource
 * IDs (no decode yet — that happens lazily when a bank is first painted).
 * The two the title sink reads are slot 6 (MAIN, id 0x91b → pool index 19)
 * and slot 7 (CURSOR, id 0x91c → pool index 20).
 *
 * Both the `settings` and `sotesp_module` parameters take the SAME sotesd
 * handle: retail sources every main-sprite slot (including slot 0's palette
 * seed, id 0x90b) from DAT_008a6e74.  (Verified: 0x90b lives in sotesd.dll,
 * not sotesp.dll, despite the historical parameter name.)
 *
 * On failure to load sotesd.dll we log and continue: the drive still boots
 * (cleared+flipped window, no sprites), matching the pre-registration state. */
static void init_sprite_banks(void)
{
    g_sotesd = LoadLibraryA("sotesd.dll");
    if (g_sotesd == NULL) {
        log_line("init_sprite_banks: LoadLibraryA(sotesd.dll) failed: %lu — "
                 "title banks stay unregistered (blank render)", GetLastError());
        return;
    }

    /* Set up the sprite/info/gdi/sound pool pointer tables once before any
     * registrar touches them (idempotent; globals start zeroed). */
    ar_state_init();

    ar_register_main_sprites(g_zdd, /*group=*/4, /*settings=*/g_sotesd,
                             /*sotesp_module=*/g_sotesd);

    /* The 8 GDI HFONTs (+ font-texture sprites 0x457/0x455) the dynamic-text
     * renderer (0x48e200) selects into the DC.  Canonical boot passes group 1
     * (FUN_00562ea0:613, ar_boot_register_all); the HFONTs are built by
     * CreateFontIndirectA and need no module resources, so settings is only
     * consumed by the two font-texture sprite slots. */
    ar_register_fonts(g_zdd, /*group=*/1, /*settings=*/g_sotesd);

    /* In-game sprite banks (milestone 2).  A live res-probe of retail's
     * opening-town render (map 0x3f2, game_enter@flip 1092) showed the in-game
     * engine 0x5a00c0 lazily decodes 74 distinct sprite banks via the SAME
     * ar_sprite_decode path the title uses — no per-map resource file, all from
     * sotesd.dll (plus a small EXE-embedded set, see below).  Cross-referenced
     * against the register tables, those banks are exactly the deferred boot
     * batches retail's ar_boot_register_all registers but init_sprite_banks
     * skipped: group 2 (palette ramps + the dialogue face portraits 0x3ea/…),
     * group 3 (the bulk of the town: 0x423-0x481, 0x769-0x76b, 0x8b7-0x8bb), and
     * group 5 (character/party sprites 0x592-0x5fb, 0x7ef-0x7f9).  Banks decode
     * lazily on first use, so registering them now is inert until a game_drive
     * renders in-game — it just primes the pool the engine port (next) will walk.
     * See docs/findings/in-game-intro.md "Resource banks (plan 3a)".
     *
     * NOT covered here: the EXE-embedded banks 0x570-0x572 (loaded with
     * hModule=NULL via FindResourceA(NULL,…) — present only in sotes.exe's own
     * .rsrc, absent from sotesd.dll) appear in no boot batch; retail registers
     * them with settings=NULL, most likely at engine time.  They are a
     * milestone-2 (game_drive) registration unit, deferred with the engine. */
    ar_register_palette_ramps(g_zdd, /*group=*/2, /*settings=*/g_sotesd,
                              /*sotesp_module=*/g_sotesd);
    ar_register_group3_sprites(g_zdd, /*group=*/3, /*settings=*/g_sotesd);
    ar_register_game_sprites  (g_zdd, /*group=*/5, /*settings=*/g_sotesd);

    /* The area-title banner's GDI font (0x494a60 DAT_008a9274[6], the
     * font picked for the town name's length 17): Courier New, h20 w10,
     * weight 400, charset 1 (the live LOGFONT, engine-quirk #96).  Built once
     * here through the same CreateFontIndirectA path as the menu fonts. */
    g_banner_font = (void *)ar_gdi_create_font(/*width=*/10, /*height=*/20,
                                               /*italic=*/0, "Courier New");

    /* The dialogue text font: Courier New h18 w7 weight 400 charset 1 (the
     * live TextOutA LOGFONT, runs/dialogue-probe textout.jsonl). */
    g_dialogue_font = (void *)ar_gdi_create_font(/*width=*/7, /*height=*/18,
                                                 /*italic=*/0, "Courier New");

    log_line("init_sprite_banks: sotesd.dll=%p, registered title banks "
             "(MAIN=pool19/id0x91b, CURSOR=pool20/id0x91c) + 8 GDI fonts "
             "(slot1 hfont=%p) + in-game batches g2/g3/g5", (void *)g_sotesd,
             (void *)(g_ar_gdi_table[1] ? g_ar_gdi_table[1]->array[0] : NULL));
}

/* Build a standalone menu node, lay the title-menu labels into its cells, and
 * render the grid into an offscreen 24bpp DIB with the registered HFONTs, then
 * dump it as a BMP.  This is the PORT side of the GDI text renderer's
 * pixel-diff gate (glyph_render.c / FUN_0048e200): the retail side renders the
 * same labels with the same font — and because both build the HFONT through
 * the same CreateFontIndirectA(LOGFONTA) path (ar_make_font), the GDI glyph
 * rasterization is bit-identical — for a differ_px comparison.
 *
 * menu_node embeds a menu_ctrl at +0x00 (list/entries/rows coincide at
 * +0x174/178/17c), so we build through the menu_ctrl cast then render through
 * the node view — exactly the dual-view the engine uses (quirk: the renderer's
 * `this` is the child node, the parent supplies the x/y base; here one node is
 * both, with x/y=0 and the base carried in node->field_c/field_10). */
static void render_glyph_test(void)
{
    static const char *const labels[] = {
        "Start", "Continue", "Bonus Menu", "Options", "Exit",
    };
    const int n_rows = (int)(sizeof labels / sizeof labels[0]);

    /* Pull the requested HFONT out of the registered GDI slots (array[0]). */
    void *hfont = NULL;
    if (g_glyph_test_font >= 0 && g_glyph_test_font < AR_GDI_SLOT_COUNT &&
        g_ar_gdi_table[g_glyph_test_font] != NULL &&
        g_ar_gdi_table[g_glyph_test_font]->array != NULL) {
        hfont = g_ar_gdi_table[g_glyph_test_font]->array[0];
    }
    if (hfont == NULL) {
        log_line("render_glyph_test: GDI font slot %d has no HFONT "
                 "(ar_register_fonts not run?) — aborting", g_glyph_test_font);
        return;
    }

    menu_node *node = (menu_node *)calloc(1, sizeof *node);
    if (node == NULL) return;
    menu_ctrl *ctrl = (menu_ctrl *)node;

    /* n_rows rows × 1 column, type-2 grid, all rows on one page. */
    menu_ctrl_build(ctrl, /*f_c=*/0, /*f_10=*/0,
                    /*alloc_a=*/n_rows, /*alloc_b=*/1,
                    /*stride=*/n_rows, /*type=*/2);
    menu_list_hdr *hdr = ctrl->list;
    hdr->count  = n_rows;
    hdr->cursor = 0;            /* row 0 ("Start") focused */
    hdr->sel2   = 0;

    /* Lay each label into (row, col 0) and mark the row enabled (flag8=1) so
     * the renderer takes the live colour paths — a disabled row would draw the
     * dead label-pointer "colours" (quirk #62c) and skip the drop shadow. */
    for (int r = 0; r < n_rows; r++) {
        glyph_cell_layout(ctrl, r, 0, labels[r]);
        ctrl->rows[r].flag8 = 1;
    }

    /* Node display config (the +0x180.. block 0x40f3e0 seeds): retail's live
     * menu colours; the label-pointer fields stay 0 (dead paths). */
    node->field_c   = 8;        /* x base */
    node->field_10  = 8;        /* y base */
    node->field_14  = 0;        /* no ruby pass */
    node->color0    = 0x3e537d; /* normal text   */
    node->color1    = 0xa8b9cc; /* drop shadow   */
    node->color2    = 0xf08080; /* focused text  */
    node->color3    = 0xf08080;
    node->field_1ac = 0x1c;     /* row pitch (28 px) */

    /* Offscreen 24bpp bottom-up DIB (= BMP pixel order); black background. */
    const int W = 256;
    const int H = node->field_10 + node->field_1ac * n_rows + 8;
    HDC ref = GetDC(NULL);
    HDC mem = CreateCompatibleDC(ref);
    BITMAPINFO bi;
    memset(&bi, 0, sizeof bi);
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = W;
    bi.bmiHeader.biHeight      = H;
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 24;
    bi.bmiHeader.biCompression = BI_RGB;
    void *bits = NULL;
    HBITMAP dib = CreateDIBSection(ref, &bi, DIB_RGB_COLORS, &bits, NULL, 0);

    if (dib != NULL && bits != NULL) {
        const uint32_t row_b = (uint32_t)((W * 3 + 3) & ~3);
        const uint32_t pixsz = row_b * (uint32_t)H;
        memset(bits, 0, pixsz);                 /* black background */

        HGDIOBJ old = SelectObject(mem, dib);
        SetBkMode(mem, TRANSPARENT);

        glyph_gdi_ops ops = glyph_gdi_ops_win32(mem);
        glyph_grid_render(node, &ops, 0, 0, hfont, hfont);
        GdiFlush();                             /* flush before reading bits */

        char path[1200];
        snprintf(path, sizeof path, "%s/port_glyph_test.bmp", g_capture_dir);
        BITMAPFILEHEADER fh;
        memset(&fh, 0, sizeof fh);
        fh.bfType    = 0x4D42;                  /* 'BM' */
        fh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        fh.bfSize    = fh.bfOffBits + pixsz;
        FILE *f = fopen(path, "wb");
        if (f != NULL) {
            fwrite(&fh, sizeof fh, 1, f);
            fwrite(&bi.bmiHeader, sizeof(BITMAPINFOHEADER), 1, f);
            fwrite(bits, 1, pixsz, f);
            fclose(f);
            log_line("render_glyph_test: wrote %s (%dx%d, font slot %d, "
                     "%d rows)", path, W, H, g_glyph_test_font, n_rows);
        } else {
            log_line("render_glyph_test: fopen(%s) failed", path);
        }
        SelectObject(mem, old);
    } else {
        log_line("render_glyph_test: CreateDIBSection(%d) failed", W);
    }

    if (dib != NULL) DeleteObject(dib);
    DeleteDC(mem);
    ReleaseDC(NULL, ref);

    menu_ctrl_clear(ctrl);      /* frees list/entries/rows + each cell's obj0 */
    free(node);
}

/* Build the alpha-ramp blend descriptors and expose them as the ramp_a/ramp_b
 * pointer tables the render wrappers consume.  pd_boot_init_slots(NULL) builds
 * the descriptors for RGB565 (the NULL-format default = the 16bpp windowed
 * display), matching retail's per-group ramps (group A: weight/20 mode 1 =
 * ramp_a; group B: weight/22 mode 0 = ramp_b).  Idempotent. */
static void init_alpha_ramps(void)
{
    if (g_ramps_built) return;
    pd_boot_init_slots(NULL);             /* NULL fmt ⇒ RGB565 descriptors */
    for (int i = 0; i < PD_BOOT_GROUP_A_COUNT; i++)
        g_ramp_a[i] = (const zdd_blend_desc *)&g_pd_boot_group_a[i];
    for (int i = 0; i < PD_BOOT_GROUP_B_COUNT; i++)
        g_ramp_b[i] = (const zdd_blend_desc *)&g_pd_boot_group_b[i];
    g_ramps_built = 1;
    log_line("init_alpha_ramps: built ramp_a/ramp_b (20+20 RGB565 blend "
             "descriptors) — menu cursor + sprite-level fades now alpha-blend");
}

/* Grab the composed primary surface into a 24bpp DIB and write it as a BMP.
 * Returns 1 on success.  Uses the already-ported zdd_object GetDC/ReleaseDC
 * primitives (DirectDrawSurface7::GetDC) + a plain GDI BitBlt into a bottom-up
 * DIB section — whose memory layout is exactly BMP pixel order, so the bits
 * write straight out after the two headers. */
static int capture_primary_to_bmp(const char *path)
{
    if (g_zdd == NULL || g_zdd->primary_obj == NULL) return 0;

    const int W = DEFAULT_WIDTH, H = DEFAULT_HEIGHT;
    void *src_hdc = NULL;
    if (!zdd_object_get_dc(g_zdd->primary_obj, &src_hdc) || src_hdc == NULL)
        return 0;

    HDC mem = CreateCompatibleDC((HDC)src_hdc);
    if (mem == NULL) { zdd_object_release_dc(g_zdd->primary_obj, src_hdc); return 0; }

    BITMAPINFO bi;
    memset(&bi, 0, sizeof bi);
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = W;
    bi.bmiHeader.biHeight      = H;          /* +H = bottom-up = BMP order */
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 24;
    bi.bmiHeader.biCompression = BI_RGB;

    void *bits = NULL;
    HBITMAP dib = CreateDIBSection((HDC)src_hdc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    int ok = 0;
    if (dib != NULL && bits != NULL) {
        HGDIOBJ old = SelectObject(mem, dib);
        if (BitBlt(mem, 0, 0, W, H, (HDC)src_hdc, 0, 0, SRCCOPY)) {
            const uint32_t row    = (uint32_t)((W * 3 + 3) & ~3);
            const uint32_t pixsz  = row * (uint32_t)H;
            BITMAPFILEHEADER fh;
            memset(&fh, 0, sizeof fh);
            fh.bfType    = 0x4D42;            /* 'BM' */
            fh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
            fh.bfSize    = fh.bfOffBits + pixsz;

            FILE *f = fopen(path, "wb");
            if (f != NULL) {
                fwrite(&fh, sizeof fh, 1, f);
                fwrite(&bi.bmiHeader, sizeof(BITMAPINFOHEADER), 1, f);
                fwrite(bits, 1, pixsz, f);
                fclose(f);
                ok = 1;
            } else {
                /* The #1 capture footgun: --capture-dir is a WSL path (e.g.
                 * /tmp/foo) the Windows exe can't fopen.  Default "." (the game
                 * dir after chdir) or a /mnt/c path works. */
                log_line("capture: fopen(\"%s\") failed (errno %d) — is "
                         "--capture-dir a Windows-writable path?  Default is the "
                         "game dir.", path, errno);
            }
        }
        SelectObject(mem, old);
    }
    if (dib != NULL) DeleteObject(dib);
    DeleteDC(mem);
    zdd_object_release_dc(g_zdd->primary_obj, src_hdc);
    return ok;
}

/* If flip_frame is in the --capture-frames whitelist, dump it. */
static void maybe_capture_frame(unsigned flip_frame)
{
    if (g_capture_all && flip_frame >= g_capture_all_from) {
        char path[1200];
        /* The _t suffix = the sim-tick axis (easer-call count), mirroring the
         * retail capture's frame_<flip>_t<tick>.png names — the studio pairs/
         * attributes on it.  --capture-frames names stay bare (tas_diff et al
         * parse the plain form). */
        snprintf(path, sizeof path, "%s/port_frame_%05u_t%06u.bmp",
                 g_capture_dir, flip_frame, g_sim_tick_count);
        capture_primary_to_bmp(path);   /* dense video dump — no per-frame log */
        return;
    }
    for (size_t i = 0; i < g_n_capture_frames; i++) {
        if (g_capture_frames[i] != flip_frame) continue;
        char path[1200];
        snprintf(path, sizeof path, "%s/port_frame_%05u.bmp",
                 g_capture_dir, flip_frame);
        int ok = capture_primary_to_bmp(path);
        /* Log the scene's pulse state too — phase (local_64), fade (uVar15)
         * and menu_fade (local_58 / the cursor level_num).  This lets the
         * parity harness match a port capture to the retail golden at the
         * same cursor-pulse phase (parity-ledger R1). */
        log_line("capture: frame %u -> %s (%s) phase=%d fade=%d menu_fade=%d "
                 "cam_x60=%ld cam_x5c=%ld",
                 flip_frame, path, ok ? "ok" : "FAILED",
                 g_drive_active ? (int)g_drive.scene.fade.phase   : -1,
                 g_drive_active ? (int)g_drive.scene.fade.fade     : -1,
                 g_drive_active ? (int)g_drive.scene.fade.menu_fade : -1,
                 g_game_camera_armed ? (long)g_game_camera.cur_x : -1L,
                 g_game_camera_armed ? (long)g_game_camera.cur_y : -1L);
        /* Particle census at the capture frame — water/sky counts + the sky
         * world bbox.  A capture-time diagnostic (dev-only path) that lets a
         * frame be cross-checked against retail's particle positions without
         * eyeballing an amplified crop (the 0x18704 smoke is faint + high). */
        {
            int nw = 0, ns = 0;
            int32_t xmn = 1<<30, xmx = -(1<<30), ymn = 1<<30, ymx = -(1<<30);
            for (int k = 0; k < PARTICLE_POOL_SLOTS; k++) {
                if (!g_fountain_pp.states[k].active) continue;
                uint32_t code = g_fountain_pp.actors[k].code;
                if (code == 0x18708u) { nw++; continue; }
                ns++;
                int32_t wx = g_fountain_pp.states[k].world_x;
                int32_t wy = g_fountain_pp.states[k].world_y;
                if (wx < xmn) xmn = wx;
                if (wx > xmx) xmx = wx;
                if (wy < ymn) ymn = wy;
                if (wy > ymx) ymx = wy;
            }
            log_line("  particles: %d water, %d sky; sky world x[%ld..%ld] y[%ld..%ld]",
                     nw, ns, (long)xmn, (long)xmx, (long)ymn, (long)ymx);
        }
        return;
    }
}

/* Build the title-scene drive against the live ZDD.  Binds the render sink to
 * g_zdd->primary_obj (so the cmd stream drives real blits) and installs
 * ar_sprite_decode_hook so the title banks self-decode once registered, plus
 * the 8d hooks (build / format / free) so the decoded banks become real keyed
 * surfaces.  The alpha ramps (0x8a92b8/0x8a9308) and the compositor display
 * group are unfilled/unmodeled at a cold boot, so they pass through as NULL
 * (plain blits / no compose — faithful).  skip_intro stays 0 so a first boot
 * plays the studio fade from the start, exactly like retail's cold launch. */
/* Resolve the pinned engine RNG seed: OSS_RNG_DEFAULT_SEED, overridable by the
 * OPENSUMMONERS_RNG_SEED env var (for experiments).  The retail parity harness
 * pins DAT_008a4f94 to the SAME value at two points: the boot title sparkle
 * (FUN_0056c070) and — re-aligning the non-deterministic title->town stream
 * (engine-quirk #77) — the first 0x41f200 effect spawn after the game_enter
 * entry (0x59f2c0).  The port mirrors both: rng_srand at boot (build_title_drive)
 * and at the top of enter_game, so the town SPAWN draws march in lockstep. */
static uint32_t game_rng_seed(void)
{
    char sbuf[32];
    uint32_t seed = OSS_RNG_DEFAULT_SEED;
    DWORD n = GetEnvironmentVariableA("OPENSUMMONERS_RNG_SEED", sbuf, sizeof sbuf);
    if (n > 0 && n < sizeof sbuf)
        seed = (uint32_t)strtoul(sbuf, NULL, 0);
    return seed;
}

/* Build (or rebuild) the title-scene drive against the live ZDD.  Shared by
 * the cold-boot init_title_drive and the post-dispatch reenter_title; the only
 * difference is `skip_intro` — retail's FUN_00562ea0 passes the title runner a
 * non-zero arg (`local_164=1`) on every re-display.  That does NOT skip the
 * intro (the runner restarts at phase 0 and replays the logos/sparkles, just
 * like retail); it only enables a phase-0 button-press to jump to the menu
 * (otherwise only phases 1+ honour a skip-press — see 56aea0.c:177/:182).
 * Returns 1 on success, 0 if title_drive_init failed.  Does NOT touch the
 * input trace (loaded once by init_title_drive); the bank registration + decode
 * hooks installed here are global and idempotent across rebuilds. */
static int build_title_drive(int skip_intro)
{
    /* Self-decode the sprite banks on first frame access, and build real
     * per-cell surfaces (8d) once a bank is decoded. */
    ar_sprite_decode_hook = ar_sprite_decode;
    ar_frame_build_hook   = title_frame_build;
    ar_frame_free_hook    = title_frame_free;
    ar_sheet_format_hook  = title_sheet_format;

    /* Build the alpha-ramp descriptors so the cursor / sprite-level fades have
     * real blend tables (without them the cursor's alpha draw no-ops). */
    init_alpha_ramps();

    /* Pin the engine RNG so the phase-7 sparkle spawn is reproducible (retail
     * srand(time()) is wall-clock dependent; the parity harness pins retail's
     * DAT_008a4f94 to the same value).  OPENSUMMONERS_RNG_SEED overrides the
     * default for experiments. */
    {
        uint32_t seed = game_rng_seed();
        uint32_t before = rng_peek_state();
        rng_srand(seed);
        osr_emit_seed(g_present_frame, before, seed);
        log_line("build_title_drive: RNG seed pinned to 0x%08lx", (unsigned long)seed);
    }

    /* Reset the sparkle-particle pool (retail allocates it count=0 at scene
     * setup; FUN_0056c2b0 clears it on exit). */
    title_particle_pool_init(&g_particles);

    title_drive_cfg cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.primary       = g_zdd->primary_obj;
    cfg.present       = drive_present;
    cfg.log_flip      = drive_log_flip;
    cfg.user          = NULL;
    cfg.select_key    = 0;        /* no saved menu pick at a cold boot */
    cfg.quiet         = 0;
    cfg.skip_intro    = skip_intro;
    cfg.ramp_a        = g_ramp_a; /* 0x8a92b8 — menu cursor + compositor */
    cfg.ramp_b        = g_ramp_b; /* 0x8a9308 — sprite-level fades + logo */
    cfg.compose_group = &g_particles.group; /* phase-7 sparkle twinkles      */
    cfg.hooks         = &g_title_hooks;     /* spawn_sparkle (FUN_0056c070)   */

    if (!title_drive_init(&g_drive, &cfg))
        return 0;

    g_drive_active = 1;
    log_line("build_title_drive: title scene driven (primary=%p, 8d surface "
             "builder wired: depth=%d bpp, skip_intro=%d)",
             (void *)g_zdd->primary_obj, g_zdd->pixel_format_bpp, skip_intro);
    return 1;
}

static void init_title_drive(void)
{
    if (!build_title_drive(/*skip_intro=*/0)) {
        log_line("init_title_drive: title_drive_init failed — "
                 "falling back to legacy present loop");
        return;
    }

    /* Load the optional input-replay trace now that the drive (and its input
     * ring) exists.  Injection happens per-frame in main_loop_body.  Loaded
     * once at cold boot; reenter_title keeps this same trace (its cursor has
     * already advanced, so no further events re-inject). */
    if (g_input_trace_path != NULL) {
        if (input_trace_load(g_input_trace_path, &g_input_trace)) {
            g_input_trace_active = 1;
            log_line("init_title_drive: input trace loaded (%zu entries) from %s",
                     g_input_trace.count, g_input_trace_path);
        } else {
            input_trace_free(&g_input_trace);
            log_line("init_title_drive: failed to load input trace %s",
                     g_input_trace_path);
        }
    }
    if (g_held_trace_path != NULL) {
        if (held_trace_load(g_held_trace_path, &g_held_trace)) {
            g_held_trace_active = 1;
            log_line("init_title_drive: held trace loaded (%zu entries) from %s",
                     g_held_trace.count, g_held_trace_path);
        } else {
            held_trace_free(&g_held_trace);
            log_line("init_title_drive: failed to load held trace %s",
                     g_held_trace_path);
        }
    }
}

/* Re-display the title menu after a sub-scene dispatch (retail's FUN_00562ea0
 * loop re-running FUN_0056aea0).  Tears down the finished drive and rebuilds it
 * with skip_intro=1 — faithful to retail's `local_164=1` re-display arg.  The
 * intro replays from phase 0 (retail does too); skip_intro only lets a phase-0
 * press jump to the menu.  On a rebuild failure the port shuts down rather than
 * spin on a dead drive. */
static void reenter_title(void)
{
    if (g_drive_active) {
        title_drive_shutdown(&g_drive);
        g_drive_active = 0;
    }
    if (!build_title_drive(/*skip_intro=*/1)) {
        log_line("reenter_title: drive rebuild failed — shutting down");
        g_shutdown = 1;
    }
}

/* ── newgame_box_ops adapter — the 9-slice panel's blit primitives ──────
 *
 * frame(): resolve slice `id` of the box-art bank (PE resource 0x457 =
 * g_ar_sprite_slots[AR_SPR_FONT_TEX_457], already registered by
 * ar_register_fonts) to its decoded sprite (lazy-decoded on first access).
 * blt(): the keyed clipped blit FUN_005b9bf0 = zdd_object_blt_clipped — draw
 * the frame at dest (x,y) with dest size w×h onto the primary surface. */
static void *newgame_box_frame_resolve(void *user, int id)
{
    (void)user;
    return ar_sprite_slot_frame(&g_ar_sprite_slots[AR_SPR_FONT_TEX_457],
                                (uint16_t)id);
}

static void newgame_box_blt(void *user, void *frame, int x, int y, int w, int h)
{
    (void)user;
    if (frame == NULL || g_zdd == NULL || g_zdd->primary_obj == NULL)
        return;
    zdd_object_blt_clipped((zdd_object *)frame, g_zdd->primary_obj,
                           x, y, w, h, /*src_x=*/0, /*src_y=*/0);
}

/* ── newgame_cursor_ops adapter — the selection-cursor sprite primitives ──
 *
 * frame(): resolve frame `id` of the cursor bank (slot NEWGAME_CURSOR_BANK_SLOT)
 * to its decoded sprite.  blt(): the plain keyed blit FUN_005b9b70 =
 * zdd_object_blt_keyed — draw the frame at base (x,y); the frame's own
 * metric_0c/_10 placement offset is added inside the blit (FUN_0048d940's plain
 * branch). */
static void *newgame_cursor_frame_resolve(void *user, int id)
{
    (void)user;
    return ar_sprite_slot_frame(&g_ar_sprite_slots[NEWGAME_CURSOR_BANK_SLOT],
                                (uint16_t)id);
}

static void newgame_cursor_blt_adapter(void *user, void *frame, int x, int y)
{
    (void)user;
    if (frame == NULL || g_zdd == NULL || g_zdd->primary_obj == NULL)
        return;
    zdd_object_blt_keyed((zdd_object *)frame, g_zdd->primary_obj, x, y);
}

/* The cursor's animation index advances once per rendered new-game frame
 * (the port has no FUN_0043c2e0 child-widget animator; this stands in for
 * node+0x72's per-frame step so the gold vine breathes through frames 16–19).
 * Reset when the scene (re)opens. */
static int g_newgame_cursor_anim;

/* Gate for the selection-cursor render (see newgame_render (1b)).
 *
 * The cursor's sprite BANK is now POSITIVELY IDENTIFIED (ckpt 42): the sibling
 * box atlas 0x455 (slot 43), frames 16–19 — exactly what the geometry port
 * already targeted.  The earlier "0x455 sweep matches nothing" was a decode-
 * ORIENTATION error: the offline sweep read the Lizsoft bitmap TOP-DOWN, landing
 * frames 16–19 on the row-4 ► chevrons (9×17), when the engine reads it
 * BOTTOM-UP — bottom-up frames 16–19 are the drooping gold feather/quill over a
 * soft white shadow.  Confirmed: bottom-up frame metrics match the live
 * --box-probe EXACTLY (frame 17 = 22×41 @ (4,3), 18 = 22×40 @ (4,4),
 * 19 = 22×41 @ (4,3)).  See tools/extract/cursor_frame_match.py + quirk #68.
 *
 * RENDER BUG FIXED (ckpt 43, quirk #69) — the earlier "scale_flag=1 videomem
 * path" diagnosis was WRONG.  The real cause was a TRANSPOSED trim scan:
 * bs_trim_opaque_rect's args were named (height, width) but retail's arg4 is
 * cell_w (the column/x loop) and arg5 is cell_h (the row/y loop).  ar_sprite_slice
 * passes (cell_w, cell_h), so a non-square cell was scanned transposed — invisible
 * on the square 32×32 box bank 0x457, but it scrambled the 32×48 cursor bank 0x455
 * into a wrong-size, wrong-offset, un-keyed cell (the live "opaque-black 16×24 @
 * x72").  With the param order corrected, the port's slot-43 frame 17 trim is
 * 22×41 @ (4,3) — exactly the --box-probe golden (verified offline via
 * tools/extract/cursor_trim_probe.c).  Gate now ON. */
static int g_newgame_cursor_enable = 1;

/* ── tooltip text node geometry (the second GDI-text node) ───────────────────
 * The tooltip box is (32,392)576×80; the text node insets it by (40,24) — the
 * FUN_0040dee0 ctor args — so the first row's glyphs land at (72,416), matching
 * the golden's TextOutA stream.  Rows step by the font line height (28, the menu
 * node's +0x1ac pitch); the wrap width is the ctor's 0x44 = 68 glyph-columns. */
#define NEWGAME_TOOLTIP_X      72
#define NEWGAME_TOOLTIP_Y      416
#define NEWGAME_TOOLTIP_PITCH  28
#define NEWGAME_TOOLTIP_WRAP   68

/* The option picker submenu box (FUN_00567ba0 default arm: FUN_00411940(this,
 * 0x120,0x80,0x100,…)) — a 1-column value grid at (288,128) width 256.  The
 * height grows with the value count (the row region at pitch 28 plus the same
 * vertical margin the 124-tall / 3-row menu box uses: 124 - 3*28 = 40).  The
 * exact box geometry is an OPEN verification gate (the retail flip counter
 * freezes in 0x565d10's modal pump, so the open picker can't be captured) —
 * the decomp gives the position/width; the height + text inset mirror the menu
 * box so the look is consistent.  See HANDOFF / docs/findings/new-game-flow.md. */
#define NEWGAME_PICKER_X       288   /* 0x120 */
#define NEWGAME_PICKER_Y       128   /* 0x80  */
#define NEWGAME_PICKER_W       256   /* 0x100 */

/* Draw one wrapped tooltip row at (x,y): the menu's 2-copy drop shadow (down 1,
 * right 1) in 0xa8b9cc, then the glyphs in 0x3e537d — the colours + offsets the
 * golden's per-glyph draws use (and glyph_grid_render's normal-row path).  The
 * font is monospace 7px (Courier New 7×18), so a single TextOutA of the row is
 * pixel-identical to retail's per-glyph stream. */
static void newgame_draw_tooltip_row(const glyph_gdi_ops *ops, void *hfont,
                                     const char *row, int x, int y)
{
    int len = (int)strlen(row);
    if (len == 0)
        return;
    ops->select_font(ops->user, hfont);
    ops->set_text_color(ops->user, 0xa8b9cc);          /* drop shadow */
    ops->text_out(ops->user, x,     y + 1, row, len);
    ops->text_out(ops->user, x + 1, y,     row, len);
    ops->set_text_color(ops->user, 0x3e537d);          /* normal text */
    ops->text_out(ops->user, x,     y,     row, len);
}

/* The new-game config scene's per-frame render (the drive's `render` callback).
 *
 * Composes the frame the way retail does (box chrome behind GDI text): clear
 * the primary to black, draw the two 9-slice box panels via DDraw, THEN GetDC
 * and glyph_grid_render the menu text on top.  The DDraw box blits must precede
 * the GDI DC acquisition (GDI locks the surface).
 *
 * The panels are the bit-exact 9-slice port (newgame_box / FUN_0048cf80, quirk
 * #67) over bank 0x457: the menu box (32,32)400×124 and the tooltip box
 * (32,392)576×80.  The menu text (newgame_menu builder + glyph_render, bit-exact
 * vs the retail TextOutA stream) lands over the cream center fill.
 *
 * The tooltip TEXT node (the focused row's help string, word-wrapped onto two
 * rows at y=416/444) is drawn over the tooltip box: newgame_scene_tooltip picks
 * the string, glyph_wrap_layout (FUN_0040e5e0) wraps it at width 68, and each
 * row is drawn at (72,416+r·28) with the same shadow+colours as the menu.
 *
 * DEFERRED seams (documented, not drawn yet): the animated sparkle corner
 * (FUN_0048d940, bank 0x3e8, frames 16–19) overlaid on the box top-left; and the
 * box fade-in (FUN_0048cf80's alpha arm) — the steady-state panel is opaque. */
static void newgame_render(void *user)
{
    newgame_drive *d = (newgame_drive *)user;
    if (g_zdd == NULL || g_zdd->primary_obj == NULL)
        return;

    /* The HFONT the menu draws with (Courier New 7×18 = slot 5), with fallbacks
     * if the font registration was partial. */
    void *hfont = NULL;
    static const int font_slots[] = { NEWGAME_FONT_SLOT, 3, 2, 1 };
    for (size_t i = 0; i < sizeof font_slots / sizeof font_slots[0]; i++) {
        int s = font_slots[i];
        if (s >= 0 && s < AR_GDI_SLOT_COUNT && g_ar_gdi_table[s] != NULL &&
            g_ar_gdi_table[s]->array != NULL && g_ar_gdi_table[s]->array[0] != NULL) {
            hfont = g_ar_gdi_table[s]->array[0];
            break;
        }
    }
    if (hfont == NULL)
        return;                       /* no registered font → nothing to draw */

    /* (1) Clear the primary to black, then draw the two 9-slice box panels via
     *     DDraw — BEFORE acquiring the GDI DC (GDI locks the surface).  The
     *     menu text renders over the menu box's cream center fill. */
    zdd_object_clear(g_zdd->primary_obj);

    static const int box_frames[9] = {
        NEWGAME_BOX_TL, NEWGAME_BOX_TOP,    NEWGAME_BOX_TR,
        NEWGAME_BOX_L,  NEWGAME_BOX_CENTER, NEWGAME_BOX_R,
        NEWGAME_BOX_BL, NEWGAME_BOX_BOTTOM, NEWGAME_BOX_BR,
    };
    newgame_box_ops bops = { newgame_box_frame_resolve, newgame_box_blt, NULL };
    newgame_box_render(&bops, /*x=*/32, /*y=*/32,  /*w=*/400, /*h=*/124,
                       box_frames, NEWGAME_BOX_CELL);   /* menu box    */
    newgame_box_render(&bops, /*x=*/32, /*y=*/392, /*w=*/576, /*h=*/80,
                       box_frames, NEWGAME_BOX_CELL);   /* tooltip box */

    /* (1c) The option picker submenu box, drawn over the menu when active (a
     *      modal overlay): a value-list grid at (288,128) width 256, height
     *      grown to the choice count.  Geometry is an OPEN gate (see the
     *      NEWGAME_PICKER_* defines). */
    if (d->picker_active) {
        int ph = d->picker.count * NEWGAME_TOOLTIP_PITCH + 40;
        newgame_box_render(&bops, NEWGAME_PICKER_X, NEWGAME_PICKER_Y,
                           NEWGAME_PICKER_W, ph, box_frames, NEWGAME_BOX_CELL);
    }

    /* (1b) The selection cursor (the drooping gold vine over the menu box's
     *      top-left, toward the focused row) — a keyed sprite blit on the DDraw
     *      side (before the GDI DC lock).  The geometry port (newgame_cursor,
     *      FUN_0048d940 type-1) and the row→base math are validated (base (40,26)
     *      = box + node fields +0x7c/-32, +0x80/-30, matching the live --box-probe
     *      and the text col0/row0 origins).  The sprite BANK is now confirmed:
     *      the sibling box atlas 0x455 (slot 43), frames 16–19, read BOTTOM-UP
     *      (the drooping gold feather/quill + soft white shadow).  The earlier
     *      "0x455 matches nothing" was an offline top-down decode error; the
     *      bottom-up frame metrics match the live --box-probe exactly.  The
     *      res_id=0x3e8 the probe reads at slot+0x40 is a reused/garbage marker
     *      (PE resource 0x3e8 is an 80×352 portrait in sotesd, a WMV in sotesw,
     *      absent in sotesp) — the reliable signal was the per-frame trim size,
     *      which is read via the entries[frameSel]→frec chain, not slot+0x40. */
    if (g_newgame_cursor_enable) {
        const menu_list_hdr *hdr = (const menu_list_hdr *)d->scene.node.ctrl_list;
        if (hdr != NULL) {
            newgame_cursor_ops cops = {
                newgame_cursor_frame_resolve, newgame_cursor_blt_adapter, NULL,
            };
            newgame_cursor_render(&cops, /*box_x=*/32, /*box_y=*/32,
                                  hdr->cursor, hdr->sel2,
                                  (int)d->scene.node.field_1ac,
                                  g_newgame_cursor_anim);
        }
        g_newgame_cursor_anim++;
    }

    /* (2) GDI text on top: the menu grid at the box base (32,32). */
    void *hdc = NULL;
    if (!zdd_object_get_dc(g_zdd->primary_obj, &hdc) || hdc == NULL)
        return;

    SetBkMode((HDC)hdc, TRANSPARENT);
    osr_emit_gdi_bkmode(hdc, TRANSPARENT);
    glyph_gdi_ops ops = glyph_gdi_ops_win32(hdc);
    glyph_grid_render(&d->scene.node, &ops, /*x=*/32, /*y=*/32, hfont, hfont);

    /* (2b) The picker's value rows over its box (when the modal submode is up):
     *      glyph_grid_render walks d->picker.node at the picker box base. */
    if (d->picker_active) {
        glyph_grid_render(&d->picker.node, &ops,
                          NEWGAME_PICKER_X, NEWGAME_PICKER_Y, hfont, hfont);
    }

    /* (3) The tooltip text node: the focused row's help string, word-wrapped
     *     onto rows over the tooltip box (FUN_0040e360 → FUN_0040e5e0). */
    char tooltip[512];
    newgame_scene_tooltip(&d->scene, tooltip);
    if (tooltip[0] != '\0') {
        glyph_wrap_result wr;
        glyph_wrap_layout(tooltip, NEWGAME_TOOLTIP_WRAP, &wr);
        for (int r = 0; r < wr.row_count; r++) {
            newgame_draw_tooltip_row(&ops, hfont, wr.rows[r],
                                     NEWGAME_TOOLTIP_X,
                                     NEWGAME_TOOLTIP_Y + r * NEWGAME_TOOLTIP_PITCH);
        }
    }
    GdiFlush();

    zdd_object_release_dc(g_zdd->primary_obj, hdc);
}

/* Enter the new-game config scene from a title-menu Start commit (app_flow
 * NEW_GAME).  Tears down the finished title drive and stands up the newgame
 * drive bound to the live primary surface (render = newgame_render, present =
 * drive_present so the Flip counter + capture path keep working).  The input
 * trace (if any) keeps its cursor; events still injecting at the current Flip
 * reach the newgame drive's ring via main_loop_body. */
static void enter_newgame(void)
{
    if (g_drive_active) {
        title_drive_shutdown(&g_drive);
        g_drive_active = 0;
    }
    g_newgame_cursor_anim = 0;       /* deterministic cursor anim per scene open */

    newgame_drive_cfg cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.settings  = NULL;             /* defaults: difficulty Easy, auto-guard On */
    cfg.render    = newgame_render;
    cfg.present   = drive_present;     /* reuse the title present thunk (Flip++)  */
    cfg.user      = &g_newgame_drive;
    cfg.gate_step = 0;                 /* → NEWGAME_DRIVE_GATE_STEP (50/frame)    */

    newgame_drive_init(&g_newgame_drive, &cfg);
    g_newgame_active = 1;
    emit_anchor("newgame_enter");
    log_line("enter_newgame: new-game config scene driven (primary=%p) — "
             "nav gate ramping open, 0x24=confirm 0x27=back",
             (void *)g_zdd->primary_obj);
}

/* Leave the new-game scene back to the title menu (BACK, or the START stub).
 * Tears down the newgame drive and re-displays the title (reenter_title). */
static void leave_newgame_to_title(const char *why)
{
    if (g_newgame_active) {
        newgame_drive_shutdown(&g_newgame_drive);
        g_newgame_active = 0;
    }
    log_line("leave_newgame: %s — re-displaying title", why);
    reenter_title();
}

/* ─── the Elemental-Stone intro cutscene (FUN_0056cd20) ──────────────────────
 *
 * Group-4 pool slots ar_register_main_sprites stamped (idx = (BSS-0x8a7640)/4):
 *   aura    g_ar_sprite_slots[1]  0x49f  224×224  (DAT_008a7644)
 *   caption g_ar_sprite_slots[2]  0x448  152×40   (DAT_008a7648) — narration tiles
 *   gem     g_ar_sprite_slots[3]  0x4a2  144×108  (DAT_008a764c)
 *
 * slot[2] (0x448) is the pre-baked prologue NARRATION text strip: 24 frames =
 * 6 lines × 4 horizontal tiles (152px each ≈ screen width), drawn as the
 * "caption" grid — confirmed live (the gem rises while the story text scrolls). */
#define PROLOGUE_SLOT_AURA    1
#define PROLOGUE_SLOT_CAPTION 2
#define PROLOGUE_SLOT_GEM     3

/* Blit one element of the cutscene's draw list.  gem/caption blend through
 * ramp_b with a keyed fallback (idx>=20 or a NULL ramp entry → plain keyed
 * blit, retail 0x5b9b70); the aura blends through ramp_a with its index
 * pre-clamped to [0,19] (no fallback).  The alpha path adds the decoded frame's
 * trim metric (+0xc/+0x10); the keyed path uses the raw logical (x,y) — exactly
 * as retail's two FUN_005bd550 / FUN_005b9b70 arms differ. */
static void prologue_blit(const prologue_draw *pd, int slot,
                          const zdd_blend_desc *const *ramp,
                          int allow_keyed_fallback)
{
    if (!pd->draw)
        return;
    zdd_object *frame =
        (zdd_object *)ar_sprite_slot_frame(&g_ar_sprite_slots[slot],
                                           (uint16_t)pd->frame);
    if (frame == NULL)
        return;

    const zdd_blend_desc *desc = NULL;
    if (pd->ramp_idx >= 0 && pd->ramp_idx < 20 && ramp != NULL)
        desc = ramp[pd->ramp_idx];

    if (desc != NULL) {
        zdd_blit_orchestrate(desc, g_zdd->primary_obj, frame,
                             frame->metric_0c + pd->x, frame->metric_10 + pd->y,
                             frame->metric_14, frame->metric_18,
                             0, 0, frame->colorkey_out, NULL);
    } else if (allow_keyed_fallback) {
        zdd_object_blt_keyed(frame, g_zdd->primary_obj, pd->x, pd->y);
    }
}

/* Compose one cutscene frame: clear to black, then gem → aura → caption tiles
 * (retail's iVar9==1 draw order: 0x56d2c0 gem, 0x56d38d aura, 0x56d460 caption). */
static void prologue_render(void *user)
{
    prologue_drive *d = (prologue_drive *)user;
    if (g_zdd == NULL || g_zdd->primary_obj == NULL)
        return;

    zdd_object_clear(g_zdd->primary_obj);    /* black background */

    prologue_render_out out;
    prologue_stone_render(&d->scene, &out);

    prologue_blit(&out.gem,  PROLOGUE_SLOT_GEM,  g_ramp_b, /*keyed=*/1);
    prologue_blit(&out.aura, PROLOGUE_SLOT_AURA, g_ramp_a, /*keyed=*/0);
    for (int k = 0; k < PROLOGUE_CAPTION_DRAWS; k++)
        prologue_blit(&out.caption[k], PROLOGUE_SLOT_CAPTION, g_ramp_b, /*keyed=*/1);
}

/* Hand off from the new-game Start-Game commit to the gem cutscene.  Tears down
 * the newgame drive and stands up the prologue drive bound to the live primary
 * (render = prologue_render, present = drive_present so Flip++ keeps working). */
static void enter_prologue(void)
{
    if (g_newgame_active) {
        newgame_drive_shutdown(&g_newgame_drive);
        g_newgame_active = 0;
    }

    prologue_drive_cfg cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.render  = prologue_render;
    cfg.present = drive_present;       /* reuse the title present thunk (Flip++) */
    cfg.user    = &g_prologue_drive;

    prologue_drive_init(&g_prologue_drive, &cfg);
    g_prologue_active = 1;
    emit_anchor("prologue_enter");
    log_line("enter_prologue: Elemental-Stone cutscene driven (primary=%p) — "
             "gem/aura/sparkles on black; 0x22=abort, 3 beats begin exit",
             (void *)g_zdd->primary_obj);
}

/* Leave the cutscene.  On ABORT (id 0x22) → re-display the title (the faithful
 * abort path).  The NORMAL exit (3rd beat → PROLOGUE_DONE) goes to enter_game
 * instead; see below. */
static void leave_prologue_to_title(const char *why)
{
    if (g_prologue_active) {
        prologue_drive_shutdown(&g_prologue_drive);
        g_prologue_active = 0;
    }
    log_line("leave_prologue: %s — re-displaying title", why);
    reenter_title();
}

/* ── town_render callbacks — the three engine globals the pure pipeline takes
 *    as seams (the sprite-bank pool, the bank pixel sizes, the zdd blit) ──── */

/* mr_sprite_fn — resolve a backdrop tile's (bank, frame) to a cel handle.
 * Faithful to FUN_00490f30's resolve: `this = (&DAT_008a760c)[bank]` then
 * `FUN_00418470(frame)` (lazy-decode + index entries[0].frames[frame]).  The
 * port models &DAT_008a760c[bank] as ar_pool_get_slot(bank) and FUN_00418470 as
 * ar_sprite_slot_frame; the cel is a zdd_object* the present blit casts back.
 * Returns 0 to skip the tile (retail's `iVar4 != 0` emit gate). */
static uint32_t game_sprite_resolve(uint16_t bank, uint16_t frame, void *ud)
{
    (void)ud;
    ar_sprite_slot *slot = ar_pool_get_slot(bank);
    if (slot == NULL) return 0;
    void *cel = ar_sprite_slot_frame(slot, frame);
    return (uint32_t)(uintptr_t)cel;
}

/* mg_bank_dims_fn — a sprite bank's pixel size (FUN_0058c910 reads pool[bank]
 * +0x20 width / +0x24 height for the bank-derived tile footprint).  In the port
 * those are ar_sprite_slot.width / .height. */
static void game_bank_dims(void *ctx, uint16_t bank_id, int32_t *w, int32_t *h)
{
    (void)ctx;
    ar_sprite_slot *slot = ar_pool_get_slot(bank_id);
    if (slot == NULL) { *w = 0; *h = 0; return; }
    *w = (int32_t)slot->width;
    *h = (int32_t)slot->height;
}

/* present_blit_fn — funnel a visible backdrop node into the matching ported zdd
 * blit.  map_render_walk emits mode 3 with node +0x14 == 0, so every town tile
 * is PRESENT_CLIPPED → FUN_005b9bf0 = zdd_object_blt_clipped (the cel as the
 * __thiscall `this`, the primary as dest, the projected screen dst + the node's
 * 0x20x0x20 source rect).  PRESENT_ALPHA can't fire for the backdrop (it needs a
 * blend descriptor we don't carry here); it lands with the actor renderers. */
static void game_present_blit(const present_op *op, void *ud)
{
    (void)ud;
    if (op->sprite == 0 || g_zdd == NULL || g_zdd->primary_obj == NULL)
        return;
    zdd_object *src = (zdd_object *)(uintptr_t)op->sprite;
    if (op->kind == PRESENT_CLIPPED) {
        zdd_object_blt_clipped(src, g_zdd->primary_obj,
                               op->dst_x, op->dst_y, op->w, op->h,
                               op->src_x, op->src_y);
    } else if (op->kind == PRESENT_KEYED) {
        /* Mode 0 — the opaque ACTOR blit (FUN_005b9b70): whole cel at the
         * projected pos, the source's own color key honored.  The town's static
         * villagers (bank 0x16c) land here via game_actor_walk. */
        zdd_object_blt_keyed(src, g_zdd->primary_obj, op->dst_x, op->dst_y);
    } else if (op->kind == PRESENT_ALPHA) {
        /* Mode 1 — the alpha ACTOR/particle blit (FUN_0048eac0 case 1 ->
         * FUN_005bd550).  The particle spray (engine-quirk #87): blend the cel
         * through a brightness ramp.  param8 = (ramp-selector << 8) | index
         * (PARTICLE_PARAM8_RAMP_B picks ramp_b / 0x8a9308 over ramp_a / 0x8a92b8;
         * the low byte is the 0..19 index).  The FOUNTAIN water uses ramp_a
         * (idx = 10 - sub_phase, &DAT_008a92e0[-sub_phase]); the SKY-ambient uses
         * ramp_b (idx = sky_fade_idx(life), &DAT_008a9308[idx]).  Mirrors
         * title_render's alpha_blit: zdd_blit_orchestrate adds the cel origin
         * (metric_0c/10) the keyed primitive would add internally.  A NULL ramp
         * entry falls back to keyed. */
        uint32_t p8  = (op->node != NULL) ? op->node->param8 : 0;
        uint32_t idx = p8 & PARTICLE_PARAM8_IDX_MASK;
        const zdd_blend_desc *const *ramp;
        uint32_t cap;
        if (p8 & PARTICLE_PARAM8_RAMP_B) { ramp = g_ramp_b; cap = PD_BOOT_GROUP_B_COUNT; }
        else                             { ramp = g_ramp_a; cap = PD_BOOT_GROUP_A_COUNT; }
        if (idx >= cap) idx = cap - 1;
        const zdd_blend_desc *desc = ramp[idx];
        if (desc != NULL)
            zdd_blit_orchestrate(desc, g_zdd->primary_obj, src,
                                 src->metric_0c + op->dst_x,
                                 src->metric_10 + op->dst_y,
                                 src->metric_14, src->metric_18,
                                 0, 0, src->colorkey_out, NULL);
        else
            zdd_object_blt_keyed(src, g_zdd->primary_obj, op->dst_x, op->dst_y);
    }
    /* PRESENT_SCALED (mode 2): deferred (no town producer emits it yet;
     * PORT-DEBT present-actor-modes). */
}

/* present_dims_fn — a cel's pixel size for map_present's mode-0 cull box.  The
 * cel is a zdd_object (frame); its source w/h are metric_b8/metric_bc (the same
 * dims zdd_object_blt_keyed uses for the Blt, and the render_id blit trace's
 * reqw/reqh).  (Retail's 0x48eac0 mode-0 cull reads cel +0x1c/+0x20; for these
 * frame cels those equal the source dims — using b8/bc keeps the cull box and
 * the actual blit size in lockstep, so an on-screen actor is never wrongly
 * culled.) */
static void game_cel_dims(uint32_t cel, int32_t *w, int32_t *h, void *ud)
{
    (void)ud;
    zdd_object *obj = (zdd_object *)(uintptr_t)cel;
    if (obj == NULL) { *w = 0; *h = 0; return; }
    *w = obj->metric_b8;
    *h = obj->metric_bc;
}

/* town_actor_walk_fn — emit the room's CHARACTER-band actors into the town
 * draw_pool, between the tile walk and the present (0x48c150 order).  Most
 * spawned actors go through the ckpt-77 default arm (actor_render_static =
 * FUN_0044d160 + the 0x491ae0 default tail + draw_pool_emit_actor); the 27
 * invisible volumes self-skip (bank 0), the 5 props emit mode-0 nodes.  The
 * animated protagonist (code 0x1872d) takes the 0x491ae0 case-0x1872d arm
 * (actor_render_protagonist, the 3-cel composite). */
static void game_actor_walk(draw_pool *pool, const mr_camera *cam, void *ud)
{
    (void)cam; (void)ud;
    int emitted = 0, struct_emitted = 0;

    /* The STRUCTURE band is FULLY MAP-DRIVEN (quirk #84) — every room loads its
     * own scenery/furniture from its map's object-placement layers, so it renders
     * for ANY room (the town tree/hedges, the house decor, the errands shop
     * furniture).  The town-specific cast bands (EFFECT townsfolk / CHARACTER /
     * the fountain PARTICLE) use captured town tables, so they stay suppressed for
     * non-town rooms (the room CAST is PORT-DEBT(cutscene-room-render) Phase 2b). */
    int room_is_town = (g_loaded_room_key == CUTSCENE_ROOM_ARRIVAL);

    /* The STRUCTURE band (0x493230) — the tree + hedges + decorations.  Each is
     * a static single-cel object whose blit is bit-identical to the default
     * actor arm, so actor_render_static draws it; the actor's +0xfc layer (8 or
     * 15) routes it before/after the cast in the layer-ordered present. */
    if (g_structs_loaded)
        for (int i = 0; i < g_structs.count; i++)
            struct_emitted += actor_render_static(&g_structs.actors[i],
                                                  &g_structs.states[i],
                                                  /*flip_table=*/g_actor_flip_table, pool,
                                                  game_sprite_resolve, NULL);

    /* The EFFECT band (0x493ba0) — the standing townsfolk.  For a plain
     * townsperson the static arm is bit-identical to the default actor arm (one
     * mode-0 keyed cel; verified against the hold blit trace), so reuse
     * actor_render_static; the +0xfc layer (13) routes them in front of the
     * bg decorations (layer 8) and behind the fg hedges (layer 15). */
    int effect_emitted = 0;
    if (g_effects_loaded && room_is_town)
        for (int i = 0; i < g_effects.count; i++)
            effect_emitted += actor_render_static(&g_effects.actors[i],
                                                  &g_effects.states[i],
                                                  /*flip_table=*/g_actor_flip_table, pool,
                                                  game_sprite_resolve, NULL);

    /* Phase 2b: the room CAST (the family) in the NON-town rooms — the same
     * static-cast render path (layer 13) as the town cast above, but the
     * house/errands family (Arche + parents) at their captured positions.  The
     * town keeps its own g_effects band; room_cast is empty there. */
    if (g_room_cast_loaded && !room_is_town)
        for (int i = 0; i < g_room_cast.count; i++)
            effect_emitted += actor_render_static(&g_room_cast.actors[i],
                                                  &g_room_cast.states[i],
                                                  /*flip_table=*/g_actor_flip_table, pool,
                                                  game_sprite_resolve, NULL);

    /* Phase 2b: the FREEROAM controllable Arche (errands) — her body bank 0x8b on
     * the same static-cast path (layer 13); her render-state world pos/clip/facing
     * are driven live by freeroam_step (the bit-exact mover). */
    if (g_freeroam_active && !room_is_town)
        effect_emitted += actor_render_static(&g_freeroam_actor, &g_freeroam_rs,
                                              /*flip_table=*/g_actor_flip_table, pool,
                                              game_sprite_resolve, NULL);

    if (g_actors_loaded && room_is_town)
        for (int i = 0; i < g_actors.count; i++) {
            const actor *a = &g_actors.actors[i];
            if (a->code == ACTOR_CODE_PROTAGONIST)
                emitted += actor_render_protagonist(a, &g_actors.states[i],
                                                    /*flip_table=*/g_actor_flip_table, pool,
                                                    game_sprite_resolve, NULL);
            else
                emitted += actor_render_static(a, &g_actors.states[i],
                                               /*flip_table=*/g_actor_flip_table, pool,
                                               game_sprite_resolve, NULL);
        }

    /* The PARTICLE band (0x493480 default arm), bank 0x1aa — the 0x18708 fountain
     * water (layer 11) + the 0x18704 sky-ambient particles (layer 6).  Emits
     * MODE-1 (alpha) nodes (param8 = (ramp-selector << 8) | index);
     * game_present_blit PRESENT_ALPHA orchestrates the blend via g_ramp_a (water)
     * / g_ramp_b (sky), so the particles are translucent like retail. */
    int particle_emitted = room_is_town
        ? particle_pool_render(&g_fountain_pp, pool, game_sprite_resolve, NULL)
        : 0;

    static int logged;
    if (!logged) {
        logged = 1;
        ar_sprite_slot *vb = ar_pool_get_slot(0x16c);                /* prop bank */
        ar_sprite_slot *pb = ar_pool_get_slot(ACTOR_PROT_SPRITE_BANK);/* 0x175    */
        ar_sprite_slot *tb = ar_pool_get_slot(0x15f);                /* tree bank */
        ar_sprite_slot *eb = ar_pool_get_slot(0x0f9);                /* townsperson bank */
        ar_sprite_slot *fb = ar_pool_get_slot(0x1aa);                /* fountain particle bank */
        log_line("game_actor_walk: %d actor + %d structure + %d effect + %d particle "
                 "nodes (prop bank 0x16c %s; protagonist bank 0x175 %s; tree bank 0x15f "
                 "%s; townsfolk bank 0xf9 %s; particle bank 0x1aa %s)",
                 emitted, struct_emitted, effect_emitted, particle_emitted,
                 vb ? "registered" : "NOT registered",
                 pb ? "registered" : "NOT registered -> protagonist invisible",
                 tb ? "registered" : "NOT registered -> tree invisible",
                 eb ? "registered" : "NOT registered -> townsfolk invisible",
                 fb ? "registered" : "NOT registered -> fountain invisible");
    }
    (void)particle_emitted;
}

/* Mirror FUN_00417c40's flag-3 / tint-case-0 field stamp (417c40.c:33-60).  The
 * in-game parallax far-plane banks (0x55/0x58/0x59) are 24bpp — no palette — so
 * the 8bpp palette grade (title_sheet_format) can't reach them.  Retail instead
 * grades 24bpp banks at DECODE: 0x417c40 (the palette-aware select the producers
 * 0x490cd0/0x499560 call per tile) stamps the slot's brightness descriptor right
 * before the frame getter triggers the lazy decode — f_08 (the 24bpp-pass gate),
 * the per-channel scales f_0c/f_10/f_14 = 1000 (tint case 0, the town's
 * DAT_008a93fc==0 identity), and f_18 = the tone-curve LUT base when the grade
 * is armed (uVar9 = in_ECX+0x28b0 iff gate1!=0 || gate2!=1000).  ar_sprite_decode
 * then runs ar_sheet_decode_pixels, mapping every non-key channel through the LUT
 * (scale identity).  The port's parallax sink uses the plain getter (0x418470),
 * skipping 0x417c40, so we replicate that one stamp here — without it the sky
 * decodes raw/ungraded (too bright). */
static void game_arm_parallax_grade(ar_sprite_slot *slot)
{
    if (slot == NULL) return;
    slot->f_08 = 1;       /* decode 24bpp brightness/LUT pass ON */
    slot->f_0c = 1000;    /* R scale  (tint case 0 → identity)   */
    slot->f_10 = 1000;    /* G scale                             */
    slot->f_14 = 1000;    /* B scale                             */
    slot->f_18 = g_color_grade_on ? (uint32_t)(uintptr_t)g_color_lut : 0u;
}

/* parallax_blit_fn — draw one parallax far-plane tile (the 0x417c40 select
 * -> FUN_005b9a40 blit pair the background producers 0x490cd0/0x499560 use).
 * The cel for (bank, frame) is the bank slot's frame surface (ar_sprite_slot_frame
 * = 0x418470 — the same resolve the tilemap walk uses), blitted WHOLE at (x,y)
 * via zdd_object_blt_onto (= FUN_005b9a40, src rect {0,0,w,h}).  The slot's grade
 * descriptor is armed first (game_arm_parallax_grade = 0x417c40's flag-3 stamp),
 * so the 24bpp far-plane decodes through the in-game tone-curve LUT — matching
 * retail's darker/more-saturated sky + mountains. */
static void game_parallax_blit(void *ctx, uint16_t bank, int32_t frame,
                               int32_t x, int32_t y)
{
    (void)ctx;
    if (g_zdd == NULL || g_zdd->primary_obj == NULL) return;
    ar_sprite_slot *slot = ar_pool_get_slot(bank);
    if (slot == NULL) return;
    game_arm_parallax_grade(slot);   /* 0x417c40's descriptor stamp, pre-decode */
    void *cel = ar_sprite_slot_frame(slot, (uint16_t)frame);
    if (cel == NULL) return;
    zdd_object_blt_onto((zdd_object *)cel, g_zdd->primary_obj, x, y);
}

/* The letterbox cel is main sprite-pool slot 41 = LETTERBOX_BANK_SLOT
 * (defined by the grade skip-list block up top) — PE resource 0x583, 64x4,
 * opaque) — registered by ar_register_main_sprites (extras[] idx 41).  The
 * engine binds it via FUN_00418470(0) (the plain frame getter, NO 0x417c40
 * grade) before the FUN_005b9a40 tile blits, so the port resolves the fixed
 * slot directly (frame 0) and blits it whole — same primitive the parallax
 * far-plane uses, minus the 24bpp grade stamp. */

/* letterbox_blit_fn — draw one letterbox cel at screen (x,y) (FUN_005b9a40). */
static void game_letterbox_blit(void *ctx, int x, int y)
{
    (void)ctx;
    if (g_zdd == NULL || g_zdd->primary_obj == NULL) return;
    void *cel = ar_sprite_slot_frame(&g_ar_sprite_slots[LETTERBOX_BANK_SLOT], 0);
    if (cel == NULL) return;
    zdd_object_blt_onto((zdd_object *)cel, g_zdd->primary_obj, x, y);
}

/* The establishing REVEAL fade-grid (scene_fade.{c,h}): the room-enter iris that
 * opens the static town from black, armed in enter_game.  Its two render sinks
 * mirror FUN_0048e920's primitives — slot 41 (res 0x583, opaque) for fully-black
 * cells (== the letterbox cel), slot 40 (res 0x458, the alpha-level black tile)
 * for the fading edge. */
static scene_fade_grid g_scene_fade;

/* scene_fade_opaque_fn — a solid black cell (FUN_005b9a40): same cel as the
 * letterbox, so reuse the sink. */
static void game_scene_fade_opaque(void *ctx, int x, int y)
{
    game_letterbox_blit(ctx, x, y);
}

/* scene_fade_alpha_fn — a fading black cell.  Retail (0x48e920:0x48eaa9) ALPHA-
 * COMPOSITES res 0x458 frame[level] over the backdrop via FUN_005bd550 with the
 * blend descriptor ECX = *(0x8a93b8) = the [19]/full entry of the group-E ramp
 * (0x8a936c; weight 1000, the mode-2 subtract-blend = darken the dest by the
 * source).  res 0x458 frame[level] is the per-level GRAY MASK (light = opaque,
 * black = clear), so the subtract-blend darkens the town by it: just-marked cells
 * (level 31, light) go near-black, almost-cleared cells (level 1, black) leave the
 * town — the soft translucent gradient.  A plain keyed blit (the first cut) drew
 * the gray opaquely (USER: "white outside, black inside, no transparency"); this
 * is the faithful composite (gdi_ctx = *(0x8a6ec0) is ~NULL => orchestrate's
 * simple path; colorkey 0x1ffffff > 16-bit => no keying, every pixel blends). */
static void game_scene_fade_alpha(void *ctx, int x, int y, int level)
{
    (void)ctx;
    if (g_zdd == NULL || g_zdd->primary_obj == NULL) return;
    void *cel = ar_sprite_slot_frame(&g_ar_sprite_slots[SCENE_FADE_ALPHA_SLOT],
                                     (uint16_t)level);
    if (cel == NULL) return;
    const zdd_blend_desc *desc =
        (const zdd_blend_desc *)&g_pd_boot_group_e[PD_BOOT_GROUP_E_COUNT - 1];
    zdd_blit_orchestrate(desc, g_zdd->primary_obj, (zdd_object *)cel,
                         x, y, 0x40, 4, 0, 0, 0x1ffffff, NULL);
}

/* ─── the AREA-TITLE BANNER ("Town of Tonkiness", banner.{c,h}) ──────────────
 *
 * The area card shown on entering the town: a scroll sprite (res 0x449, slot 53
 * / the 0x8a7714 bank, registered by ar_register_palette_ramps) with the area
 * name GDI-composed onto it, faded in.  Producer 0x494a60 (mode 1) + the
 * cinematic-step phase machine 0x499ab0; rendered AFTER the scene_fade grid
 * (0x48c150:176-178).  engine-quirk #96; findings "The area-title BANNER".
 * (g_banner / g_banner_font / g_banner_armed declared above, before
 * init_sprite_banks.) */

/* Compose the area name onto the scroll cel via GDI, ONCE (0x494a60 case-1:
 * GetDC the cel -> shadow 0x404040 (3 rows x 4 cols) + white 0xffffff (2x) ->
 * ReleaseDC).  TextOutA == FUN_0048e860 for the single-record ASCII string; the
 * text bakes into the shared slot-53 cel, like retail's DAT_008a7714 cache. */
static void banner_compose_text(zdd_object *cel)
{
    void *hdc = NULL;
    if (!zdd_object_get_dc(cel, &hdc) || hdc == NULL)
        return;
    banner_layout L = banner_text_layout(g_banner.text);
    SetBkMode((HDC)hdc, TRANSPARENT);                  /* bkmode 1 */
    if (g_banner_font != NULL)
        SelectObject((HDC)hdc, (HGDIOBJ)g_banner_font);
    SetTextColor((HDC)hdc, (COLORREF)0x404040);        /* the dark outline shadow */
    for (int row = 0; row < 3; row++)                  /* y = y_off+9..+11 */
        for (int dx = -2; dx < 2; dx++)                /* x = x_base-2..+1 */
            TextOutA((HDC)hdc, L.x_base + dx, L.y_off + 9 + row, g_banner.text, L.len);
    SetTextColor((HDC)hdc, (COLORREF)0xffffff);        /* the white fill */
    TextOutA((HDC)hdc, L.x_base - 1, L.y_off + 10, g_banner.text, L.len);
    TextOutA((HDC)hdc, L.x_base,     L.y_off + 10, g_banner.text, L.len);
    zdd_object_release_dc(cel, hdc);
}

/* Render the area-title banner (0x494a60 case 1), AFTER scene_fade_render.
 * Composes the text once, then blits the scroll cel at (160,64): keyed when fully
 * opaque (alpha 1000), alpha-composited through ramp_b while fading in. */
static void game_render_banner(void)
{
    if (!banner_active(&g_banner))
        return;
    if (g_zdd == NULL || g_zdd->primary_obj == NULL)
        return;
    /* The scroll sheet (slot 53) is decoded UNGRADED — retail binds it via the
     * plain getter FUN_00418470(0) (NO 0x417c40 grade), so its palette skips the
     * in-game colour-grade (the skip is in the decode hook).  With that, the
     * parchment is bit-exact vs retail (differ_px=0); a graded decode rendered it
     * ~10% too dark. */
    zdd_object *cel = (zdd_object *)ar_sprite_slot_frame(
        &g_ar_sprite_slots[BANNER_SCROLL_SLOT], 0);
    if (cel == NULL)
        return;
    if (!g_banner.composed) {
        banner_compose_text(cel);
        g_banner.composed = 1;
    }
    int idx = banner_alpha_ramp_index(g_banner.alpha);
    const zdd_blend_desc *desc =
        (idx >= 0 && idx < PD_BOOT_GROUP_B_COUNT) ? g_ramp_b[idx] : NULL;
    if (desc != NULL) {
        zdd_blit_orchestrate(desc, g_zdd->primary_obj, cel,
                             cel->metric_0c + BANNER_DST_X,
                             cel->metric_10 + BANNER_DST_Y,
                             cel->metric_14, cel->metric_18,
                             0, 0, cel->colorkey_out, NULL);
    } else {
        zdd_object_blt_keyed(cel, g_zdd->primary_obj, BANNER_DST_X, BANNER_DST_Y);
    }
}

/* ─── the in-game DIALOGUE BOX (dialogue.{c,h}, 0x48c820 widget tree) ────────
 *
 * Rendered AFTER the 3 banner slots (0x48c150:179).  Per frame: the 9-slice
 * bubble frame (res 0x456 frames 0-8) at the pop-in scale (0x48c820 mode
 * +0x1c==1), then — once the content gate opens (scale==1000) — the cels in
 * retail cell order (bubble corner 9 / tail 10 / name tab / portrait), the GDI
 * text pass (name + revealed body rows, 3 TextOut passes each), and the
 * advance arrow (0x48d940, after the content loop). */

static void *dialogue_box_frame_resolve(void *user, int id)
{
    (void)user;
    return ar_sprite_slot_frame(&g_ar_sprite_slots[DIALOGUE_BOX_BANK_SLOT],
                                (uint16_t)id);
}

/* One GDI text row, the 0x48da70/0x48e200 3-pass shape: full shadow pass at
 * (x,y+1), full shadow pass at (x+1,y), then the main pass at (x,y) — each a
 * per-char TextOutA at the 7px advance. */
static void dialogue_text_row(HDC hdc, const char *s, int n, int x, int y,
                              uint32_t shadow, uint32_t main_color)
{
    if (n <= 0)
        return;
    SetTextColor(hdc, (COLORREF)shadow);
    osr_emit_gdi_color(hdc, shadow);
    for (int i = 0; i < n; i++) {
        osr_emit_gdi_text(hdc, x + i * DIALOGUE_ADVANCE, y + 1, &s[i], 1);
        TextOutA(hdc, x + i * DIALOGUE_ADVANCE, y + 1, &s[i], 1);
    }
    for (int i = 0; i < n; i++) {
        osr_emit_gdi_text(hdc, x + i * DIALOGUE_ADVANCE + 1, y, &s[i], 1);
        TextOutA(hdc, x + i * DIALOGUE_ADVANCE + 1, y, &s[i], 1);
    }
    SetTextColor(hdc, (COLORREF)main_color);
    osr_emit_gdi_color(hdc, main_color);
    for (int i = 0; i < n; i++) {
        osr_emit_gdi_text(hdc, x + i * DIALOGUE_ADVANCE, y, &s[i], 1);
        TextOutA(hdc, x + i * DIALOGUE_ADVANCE, y, &s[i], 1);
    }
}

/* ─── inline @@<code> KEY-CAP icons (res 0x6fa / slot 55, ckpt 153) ──────────
 *
 * The dialogue body carries `@@<code>` escapes (0x40 0x40 + a 1-2 byte code) that
 * retail renders as 17x17 key-cap sprites from res 0x6fa (the keyboard
 * button-prompt sheet; findings/res0-ui-banks.md, 279/279-px verified): the
 * errands movement tutorial shows ← → and X.  The `@@<code>` consumes the @@ +
 * code from the source but advances the monospace body cursor by 3 cells (21px),
 * blitting the key-cap there.  (The grid renderer 0x48e200 sprite-cell mode; the
 * frames are bottom-up-sliced like res 0x455.) */
#define DIALOGUE_KEYCAP_DY     2      /* the 17px cel sits 2px above the text row y  */
#define DIALOGUE_MAX_ICONS     16

struct dlg_icon { int frame, x, y; };

/* Render one body row's revealed text, turning `@@<code>` escapes into key-cap
 * icons.  Text glyphs go to `hdc` (the dialogue_text_row 3-pass GDI shape, but at
 * monospace CELL positions so the layout survives icon insertion); each icon is
 * SKIPPED in the text and appended to `icons` (blitted by the caller AFTER the DC
 * is released — DDraw forbids Blt while a GetDC is checked out).  The cell count
 * (char=1, icon=DIALOGUE_KEYCAP_CELLS) matches dialogue_expand_text's wrap. */
static void dialogue_body_row_text(HDC hdc, const char *row, int revealed,
                                   int x0, int y, uint32_t shadow, uint32_t main_color,
                                   struct dlg_icon *icons, int *n_icons)
{
    int len = (int)strlen(row);
    if (revealed > len) revealed = len;
    for (int pass = 0; pass < 3; pass++) {            /* shadow@y+1, shadow@x+1, main */
        uint32_t col = (pass < 2) ? shadow : main_color;
        int dx = (pass == 1) ? 1 : 0, dy = (pass == 0) ? 1 : 0;
        SetTextColor(hdc, (COLORREF)col);
        if (pass != 1) osr_emit_gdi_color(hdc, col);  /* x+1 reuses the shadow colour */
        int c = 0, i = 0;
        while (i < revealed) {
            int blen = 1, tcells = 1;
            int frame = dialogue_keycap_token(row + i, len - i, &blen, &tcells);
            if (frame != -2) {                        /* an @@<code> key-cap token */
                if (pass == 2 && frame >= 0 && *n_icons < DIALOGUE_MAX_ICONS) {
                    icons[*n_icons].frame = frame;
                    icons[*n_icons].x     = x0 + c * DIALOGUE_ADVANCE;
                    icons[*n_icons].y     = y - DIALOGUE_KEYCAP_DY;
                    (*n_icons)++;
                }
                c += tcells;
                i += blen;
            } else {
                char ch = row[i];
                osr_emit_gdi_text(hdc, x0 + c * DIALOGUE_ADVANCE + dx, y + dy, &ch, 1);
                TextOutA(hdc, x0 + c * DIALOGUE_ADVANCE + dx, y + dy, &ch, 1);
                c += 1;
                i += 1;
            }
        }
    }
}

/* Render ONE dialogue box `d` (the 9-slice frame at its pop-in/out scale, then —
 * once content-visible — the tail/tab cels, the portrait, and the GDI text).
 * `emit_trace` gates the cross-side CALL_TRACE points (box pos + frame) to the
 * MAIN box only: the closing box (the OLD box popping out during a speaker
 * change, ckpt 134 / quirk #107) is port-only polish with no retail trace to
 * align.  A closing box has scale < 1000, so it renders ONLY the shrinking frame
 * (the content gate stops the cels/text) — retail's disappearing box. */
static void render_dialogue_box(dialogue_box *d, int emit_trace)
{
    if (!dialogue_active(d))
        return;
    if (g_zdd == NULL || g_zdd->primary_obj == NULL)
        return;

    /* The live speaker-anchored box position (0x49c640 over the 0x490b90
     * projection): the box rides the speaker's world pos projected through the
     * live in-game camera (g_game_camera_mr) — replacing the old hardcoded
     * (174,148).  Harness-verified bit-exact for the town intro (box-pos-inputs). */
    /* The box position is FROZEN: 0x49c640 anchors it when the line opens, but the
     * widget holds that SCREEN position (it does NOT re-project each frame), so the
     * box stays put while the camera pans — retail's run-off "Cool!" box is static
     * at its open pos through the whole pan (draw-stream ground truth), not riding
     * the camera/speaker.  Compute once (held), then on the run-off CLOSE the empty
     * frame slides off per the captured curve. */
    int box_x, box_y, tail_x;
    if (!d->held) {
        dialogue_box_position(d, DIALOGUE_BOX_W, DIALOGUE_BOX_H,
                              g_game_camera_mr.off60, g_game_camera_mr.off5c,
                              g_game_camera_mr.off74, &box_x, &box_y, &tail_x);
        d->pos_x = box_x; d->pos_y = box_y; d->pos_tail = tail_x;
        d->held  = 1;
    } else {
        box_x = d->pos_x; box_y = d->pos_y; tail_x = d->pos_tail;
    }
    /* Mirror retail's box-position read-point (0x49c910 boxpos_out): the values
     * 0x49c640 wrote to box+0xc/+0x10.  flow_diff aligns this against the retail
     * capture (tools/flow/box_pos_inputs_fields.json). */
    if (emit_trace) {
        CALL_TRACE_BEGIN(0x49c910);
        CALL_TRACE_I32("box_x", box_x);
        CALL_TRACE_I32("box_y", box_y);
        CALL_TRACE_END();
    }

    /* the 9-slice bubble frame at the current pop-in scale, centered on the box */
    static const int frames9[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    int bx, by, bw, bh;
    if (d->slide_idx >= 0) {
        /* The run-off CLOSE = the empty frame SHRINKS SYMMETRICALLY toward its frozen
         * center per the captured curve (the box-corner offset dx,dy; the box reads
         * (corner..corner) so width = box_w-2*dx).  Bit-exact vs retail.osr; the
         * empty frame (no body/portrait) — dialogue-runoff-box-slide. */
        int i = (d->slide_idx < DIALOGUE_RUNOFF_SLIDE_N)
                ? d->slide_idx : DIALOGUE_RUNOFF_SLIDE_N - 1;
        int dx = DIALOGUE_RUNOFF_SLIDE[i][0], dy = DIALOGUE_RUNOFF_SLIDE[i][1];
        bx = box_x + dx;            by = box_y + dy;
        bw = DIALOGUE_BOX_W - 2*dx; bh = DIALOGUE_BOX_H - 2*dy;
    } else {
        dialogue_scaled_rect(d, box_x, box_y, &bx, &by, &bw, &bh);
    }
    if (emit_trace) {
        CALL_TRACE_BEGIN(0x48cf80);          /* the box-frame draw, cross-side */
        CALL_TRACE_I32("x", bx);
        CALL_TRACE_I32("y", by);
        CALL_TRACE_I32("w", bw);
        CALL_TRACE_I32("h", bh);
        CALL_TRACE_END();
    }
    if (bw > 0 && bh > 0) {
        newgame_box_ops bops = { dialogue_box_frame_resolve, newgame_box_blt, NULL };
        newgame_box_render(&bops, bx, by, bw, bh, frames9, NEWGAME_BOX_CELL);
    }
    if (d->slide_idx >= 0)
        return;                          /* run-off close: empty frame only (no content) */
    if (!dialogue_content_visible(d))
        return;                          /* 0x48c820's +0x54<1000 content gate */

    /* content cels, retail cell creation order ([0x17] tail notch, [0x18]
     * tail spike, [0x1b] tab, then the portrait pair) — all plain keyed
     * blits; the cel's own placement metrics are applied inside
     * zdd_object_blt_keyed.  The tail pair hangs at the box bottom at the
     * speaker-anchored x (0x49c640). */
    zdd_object *cel;
    cel = (zdd_object *)ar_sprite_slot_frame(
        &g_ar_sprite_slots[DIALOGUE_BOX_BANK_SLOT], DIALOGUE_BOX_FRAME_CORNER);
    if (cel != NULL)
        zdd_object_blt_keyed(cel, g_zdd->primary_obj,
                             box_x + tail_x, box_y + DIALOGUE_TAIL_NOTCH_Y);
    cel = (zdd_object *)ar_sprite_slot_frame(
        &g_ar_sprite_slots[DIALOGUE_BOX_BANK_SLOT], DIALOGUE_BOX_FRAME_TAIL);
    if (cel != NULL)
        zdd_object_blt_keyed(cel, g_zdd->primary_obj,
                             box_x + tail_x, box_y + DIALOGUE_TAIL_SPIKE_Y);
    /* the name TAB — 0x439690:401-421: the tab CELL x AND the tab FRAME depend on the
     * NAME LENGTH (the narrow tab + box_w-0x80 for <= 12 chars; the wide tab +
     * box_w-0xc0 for longer), so a single constant can't serve both "Arche" (5) and
     * "Arche's Father" (14).  The name glyphs sit at tab_dx + (NAME_DX-TAB_DX). */
    int name_long = ((int)strlen(d->name) > 12);
    int tab_dx    = DIALOGUE_BOX_W - (name_long ? 0xc0 : 0x80);
    int tab_frame = name_long ? DIALOGUE_TAB_FRAME_LONG : DIALOGUE_TAB_FRAME_SHORT;
    int name_dx   = tab_dx + (DIALOGUE_NAME_DX - DIALOGUE_TAB_DX);
    cel = (zdd_object *)ar_sprite_slot_frame(
        &g_ar_sprite_slots[DIALOGUE_TAB_BANK_SLOT], tab_frame);
    if (cel != NULL)
        zdd_object_blt_keyed(cel, g_zdd->primary_obj,
                             box_x + tab_dx, box_y + DIALOGUE_TAB_DY);

    /* the portrait bust (24bpp magenta-keyed, drawn 1:1): the cross-fade blends
     * through ramp_b while fading (0x49c910), then snaps to the plain keyed
     * blit.  PER-SPEAKER (ckpt 123): the slot is resolved per line by the
     * 0x49d6e0 face table (portrait.c — speaker head-state + face → pool-slot),
     * carried on the box (portrait_slot); -1 = no portrait (the no-record path). */
    int pslot = d->portrait_slot;
    /* The 0x49d6e0 face table (portrait.c) returns a POOL index, NOT a raw
     * g_ar_sprite_slots array index — the sprite pool is offset by the ramp
     * slots (ar_pool_get_slot maps pool i → g_ar_sprite_slots[i-(RAMP+1)]).  The
     * ckpt-123 render indexed g_ar_sprite_slots[pslot] DIRECTLY, so every
     * portrait was shifted +13 slots → the wrong bust resource AND dims per line
     * (HARNESS-VERIFIED ckpt 131: retail pool 676=res 0x7ef/160x176 lives at the
     * port's slot 663; the raw index rendered slot 676=res 0x8a4 = retail pool
     * 689's bust — a different expression, and for var-B slots a 176x144 vs the
     * correct 160x176).  Resolve via the pool accessor so the right bust loads. */
    ar_sprite_slot *pobj = (pslot >= 0) ? ar_pool_get_slot((uint16_t)pslot) : NULL;
    cel = (pobj != NULL) ? (zdd_object *)ar_sprite_slot_frame(pobj, 0) : NULL;
    if (cel != NULL) {
        int ridx = dialogue_portrait_ramp_index(d);
        if (ridx == DIALOGUE_PORTRAIT_GONE) {
            /* the speaker-change OLD bust has fully dissolved out (fade-out past
             * idx 2) — draw NO portrait (quirk #108).  In practice the closing
             * box's content gate (scale < 1000) hits the same tick; this guards
             * the case the frame lingers full a tick longer. */
        } else {
            const zdd_blend_desc *desc =
                (ridx >= 0 && ridx < PD_BOOT_GROUP_B_COUNT) ? g_ramp_b[ridx] : NULL;
            if (desc != NULL)
                zdd_blit_orchestrate(desc, g_zdd->primary_obj, cel,
                                     cel->metric_0c + box_x + DIALOGUE_PORTRAIT_DX,
                                     cel->metric_10 + box_y + DIALOGUE_PORTRAIT_DY,
                                     cel->metric_14, cel->metric_18,
                                     0, 0, cel->colorkey_out, NULL);
            else
                zdd_object_blt_keyed(cel, g_zdd->primary_obj,
                                     box_x + DIALOGUE_PORTRAIT_DX,
                                     box_y + DIALOGUE_PORTRAIT_DY);
        }
    }

    /* the GDI text pass (0x48c820: GetDC the paint target, TRANSPARENT bk).  The
     * body rows may carry @@<code> key-cap escapes — the text renders with them
     * SKIPPED (their cells reserved) and each icon is collected here, then blitted
     * AFTER release_dc (DDraw forbids Blt while a GetDC is out). */
    struct dlg_icon kc_icons[DIALOGUE_MAX_ICONS];
    int kc_n = 0;
    void *hdc = NULL;
    if (zdd_object_get_dc(g_zdd->primary_obj, &hdc) && hdc != NULL) {
        SetBkMode((HDC)hdc, TRANSPARENT);
        osr_emit_gdi_bkmode(hdc, TRANSPARENT);
        if (g_dialogue_font != NULL) {
            SelectObject((HDC)hdc, (HGDIOBJ)g_dialogue_font);
            osr_emit_gdi_select_font(hdc, g_dialogue_font);
        }
        /* speaker name (the [0x1c] cell: white main, 0x455f7b shadow) */
        dialogue_text_row((HDC)hdc, d->name,
                          (int)strlen(d->name),
                          box_x + name_dx, box_y + DIALOGUE_NAME_DY,
                          DIALOGUE_NAME_SHADOW, DIALOGUE_NAME_MAIN);
        /* revealed body rows (the [0x1d] grid: 0x3e537d main, 0xa8b9cc shadow) */
        for (int r = 0; r < d->row_count; r++)
            dialogue_body_row_text((HDC)hdc, d->rows[r],
                                   dialogue_row_revealed(d, r),
                                   box_x + DIALOGUE_TEXT_DX,
                                   box_y + dialogue_body_row_dy(d, r),
                                   DIALOGUE_BODY_SHADOW, DIALOGUE_BODY_MAIN,
                                   kc_icons, &kc_n);
        zdd_object_release_dc(g_zdd->primary_obj, hdc);
    }
    /* the collected inline @@ key-cap icons (res 0x6fa / slot 55) */
    for (int k = 0; k < kc_n; k++) {
        zdd_object *kc = (zdd_object *)ar_sprite_slot_frame(
            &g_ar_sprite_slots[AR_SPR_KEYCAP_6FA], (uint16_t)kc_icons[k].frame);
        if (kc != NULL)
            zdd_object_blt_keyed(kc, g_zdd->primary_obj,
                                 kc_icons[k].x, kc_icons[k].y);
    }

    /* The advance "next" indicator (0x48d940, drawn after the content loop): the
     * green BOOK icon at the box bottom-right.  HIDDEN while the typewriter runs
     * (the +0x174[0]==1 early-out) and shown only once the line is fully revealed
     * and waiting for Z (dialogue_awaiting_advance), on the MAIN box only.
     *
     * BANK RESOLVED (ckpt 153, Frida runs/ui-bank): god+0xb8c = PE resource 0x455
     * (sotesd.dll) = port slot AR_SPR_FONT_TEX_455 (43, the SAME atlas the newgame
     * cursor uses) — a 24-frame 4x6 32x48 atlas whose frames 20-23 are the book
     * "next" icon (base 0x14 + anim table {0,1,2,3}, step per 10 updates; the 1px
     * bob is baked into the per-frame cel placement metrics, applied inside the
     * keyed blit).  Pos = box+(0x170,0x5c) = (368,92) (0x410560 +0x7c/+0x80) — at
     * the errands box (32,192) that lands the cel at (400,284), == retail.osr seq
     * 825 tick 1823 (32x31). */
    if (emit_trace && dialogue_awaiting_advance(d)) {
        zdd_object *acel = (zdd_object *)ar_sprite_slot_frame(
            &g_ar_sprite_slots[AR_SPR_FONT_TEX_455],
            (uint16_t)dialogue_arrow_frame(d));
        if (acel != NULL) {
            CALL_TRACE_BEGIN(0x48d940);          /* the advance-arrow draw */
            CALL_TRACE_I32("x", box_x + DIALOGUE_ARROW_DX);
            CALL_TRACE_I32("y", box_y + DIALOGUE_ARROW_DY);
            CALL_TRACE_I32("frame", dialogue_arrow_frame(d));
            CALL_TRACE_END();
            zdd_object_blt_keyed(acel, g_zdd->primary_obj,
                                 box_x + DIALOGUE_ARROW_DX,
                                 box_y + DIALOGUE_ARROW_DY);
        }
    }
}

/* Render the dialogue: during a speaker-change transition the OLD box (closing,
 * at the previous speaker's anchor) draws FIRST = BEHIND, then the MAIN box (the
 * opening/current line) draws on top = IN FRONT — retail's z-order (the new box
 * has the higher draw seq; engine-quirk #107, drawcall-verified).  The closing
 * box is owned + stepped by the cutscene (cutscene_closing_box). */
static void game_render_dialogue(void)
{
    const dialogue_box *closing = cutscene_closing_box(&g_cutscene);
    if (closing != NULL)
        render_dialogue_box((dialogue_box *)closing, 0); /* old box (behind)     */
    render_dialogue_box(&g_dialogue, 1);                 /* new/current (in front)*/
}

/* Forward decl: reload_room_backdrop (below) re-derives the projection cam via
 * game_camera_to_mr, which is defined later with the camera-step helpers. */
static void game_camera_to_mr(const camera_view *v, mr_camera *out);

/* Load the room's map-data DATA resource from the original sotes.exe and build
 * the town backdrop scene.  Mirrors retail FUN_00587970: FindResourceA(EXE,
 * scene&0xffff, "DATA") + LoadResource + LockResource, then the parse + decode
 * (here town_render_load).  The opening town (map 0x3f2 → room 210110) is scene
 * 1022.  Returns 1 on success.  On any failure the scene stays unloaded and
 * game_render shows the faithful black map-load frame. */
static int load_town_scene(uint16_t scene, int parallax_p2, int parallax_p3)
{
    if (g_sotes_exe == NULL) {
        /* The original packed sotes.exe is still in the game-dir CWD; Steamless
         * leaves its .rsrc readable.  AS_DATAFILE maps it for FindResource only
         * (no DllMain / no code execution). */
        g_sotes_exe = LoadLibraryExA("sotes.exe", NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (g_sotes_exe == NULL) {
            log_line("load_town_scene: LoadLibraryExA(sotes.exe, AS_DATAFILE) "
                     "failed: %lu — backdrop stays black", GetLastError());
            return 0;
        }
    }

    HRSRC hres = FindResourceA(g_sotes_exe, MAKEINTRESOURCEA(scene), "DATA");
    if (hres == NULL) {
        log_line("load_town_scene: FindResourceA(DATA %u) failed: %lu",
                 scene, GetLastError());
        return 0;
    }
    HGLOBAL hmem = LoadResource(g_sotes_exe, hres);
    DWORD    len = SizeofResource(g_sotes_exe, hres);
    const uint8_t *bytes = (hmem != NULL) ? (const uint8_t *)LockResource(hmem) : NULL;
    if (bytes == NULL || len == 0) {
        log_line("load_town_scene: Load/LockResource(DATA %u) failed (len=%lu)",
                 scene, len);
        return 0;
    }

    if (town_render_load(&g_town, bytes, (size_t)len, game_bank_dims, NULL,
                         parallax_p2, parallax_p3) != 0) {
        log_line("load_town_scene: town_render_load(DATA %u, %lu B) failed "
                 "(malformed resource?) — backdrop stays black", scene, len);
        town_render_free(&g_town);
        return 0;
    }

    g_town_loaded = 1;
    char nm[0x21];
    log_line("load_town_scene: DATA %u loaded (%lu B): \"%s\" %ux%ux%u, %u layers "
             "— town backdrop scene up", scene, len,
             map_data_name(&g_town.map, nm),
             g_town.map.dim0, g_town.map.dim1, g_town.map.dim2, g_town.map.count);
    return 1;
}

/* Resolve a room KEY (room_state+0x4024, e.g. 0x334be town / 0x334c8 house /
 * 0x334dc errands) to its DATA scene id + 0x587e00 parallax params via the room
 * registry, then load that backdrop.  This is the faithful map-load selection
 * (FUN_00586010:697 keys FindResourceA on room[GW_ROOM_SCENE] + passes
 * room[0x44]/room[0x43] to the decode prologue).  Builds the registry lazily.
 * Returns 1 on success; 0 leaves the backdrop black (the faithful map-load frame). */
static int load_room(uint32_t room_key)
{
    if (!g_world_built) {
        if (game_world_build(&g_world) != 0) {
            log_line("load_room: game_world_build failed — backdrop stays black");
            return 0;
        }
        g_world_built = 1;
    }
    uint16_t scene = 0;
    int p2 = TOWN_RENDER_PARALLAX_P2, p3 = TOWN_RENDER_PARALLAX_P3;
    if (game_world_room_render_cfg(&g_world, room_key, &scene, &p2, &p3) != 0) {
        log_line("load_room: room key 0x%05x not in registry — backdrop stays black",
                 room_key);
        return 0;
    }
    log_line("load_room: room 0x%05x -> DATA scene %u, parallax (p2=%d p3=%d)",
             room_key, scene, p2, p3);
    int ok = load_town_scene(scene, p2, p3);
    if (ok)
        g_loaded_room_key = room_key;
    return ok;
}

/* The per-room SETTLED camera scroll origin (the view object's cur_x/cur_y the
 * room snaps to on entry — camera_follow.h +0x60/+0x5c).  HARNESS-CAPTURED off
 * the live scene view across the cutscene chain (runs/room-render-gt/camera,
 * tools/flow/room_camera_fields.json, ckpt 130): the house + errands cameras are
 * STATIC (constant across every flip in the room).  PORT-DEBT(ingame-camera-snap):
 * captured constants per room (the town's MAP_RENDER_CAM_TOWN_3F2 precedent), not
 * yet derived from the room entry params.  Returns 1 if known. */
static int room_camera_origin(uint32_t room_key, int32_t *cur_x, int32_t *cur_y)
{
    switch (room_key) {
    case CUTSCENE_ROOM_HOUSE:   *cur_x = 89600; *cur_y = 3200;  return 1;  /* map 153600x51200 */
    case CUTSCENE_ROOM_ERRANDS: *cur_x = 0;     *cur_y = 16000; return 1;  /* map 108800x64000 */
    default: return 0;   /* town stays on its own spawn-snap + pan path */
    }
}

/* Swap the loaded backdrop to a new room (the cutscene room-key swap / the
 * errands hand-off): free the current scene, load the new one, update the live
 * camera's map bounds, and re-snap it to that room's settled origin.  Returns
 * load_room's result. */
static int reload_room_backdrop(uint32_t room_key)
{
    log_line("game: room swap 0x%05x -> 0x%05x — reloading backdrop",
             g_loaded_room_key, room_key);
    town_render_free(&g_town);
    g_town_loaded = 0;
    int ok = load_room(room_key);
    if (ok) {
        int32_t cx, cy;
        if (room_camera_origin(room_key, &cx, &cy)) {
            /* the view's map bounds drive camera_apply_snap's [0, map-vp] clamp */
            g_game_camera.map_w = (int32_t)g_town.map.dim0 * (int32_t)MG_PX_PER_DIM;
            g_game_camera.map_h = (int32_t)g_town.map.dim1 * (int32_t)MG_PX_PER_DIM;
            camera_apply_snap(&g_game_camera, cx, cy);
            game_camera_to_mr(&g_game_camera, &g_game_camera_mr);
        }
        /* Re-spawn the room's STRUCTURE band (scenery/furniture) from the NEW
         * room's map — it is fully map-driven (quirk #84), so the house decor /
         * errands shop furniture come straight from DATA 1023/1025's object
         * layers.  game_actor_walk renders it for every room.  The town-specific
         * EFFECT/CHARACTER cast bands stay as-is (suppressed for non-town rooms;
         * the room CAST/NPCs are PORT-DEBT(cutscene-room-render) Phase 2b). */
        int sn = actor_spawn_struct_from_map(&g_structs, &g_town.map);
        g_structs_loaded = (sn > 0);
        log_line("reload_room_backdrop: actor_spawn_struct_from_map -> %d "
                 "STRUCTURE objects for room 0x%05x", sn, room_key);

        /* Phase 2b: the room CAST (the family).  In the house/errands the town
         * cast band is suppressed, so spawn this room's cast (Arche + parents)
         * as static-cast actors at their captured positions + idle clips.  The
         * town (arrival) keeps its own cast band (g_effects); room_cast is empty
         * there. */
        int cn = actor_spawn_room_cast(&g_room_cast, room_key);
        g_room_cast_loaded = (cn > 0);
        g_arche_house_turning = 0;   /* a fresh cast spawns Arche on her idle clip */
        log_line("reload_room_backdrop: actor_spawn_room_cast -> %d CAST members "
                 "for room 0x%05x", cn, room_key);
    }
    return ok;
}

/* Phase 2b: the FREEROAM hand-off.  When the cutscene chain completes at the
 * errands room, spawn controllable Arche at her errands world position (the
 * +0x200==0 char-AI hand-off, quirk #103 — the bit-exact mover takes over).  Her
 * body bank 0x8b + flip table persist from the town cast registration. */
static void freeroam_begin(void)
{
    character_init(&g_freeroam_char, FREEROAM_ARCHE_SPAWN_WX,
                   FREEROAM_ARCHE_SPAWN_WY, CHAR_FACE_RIGHT);
    memset(&g_freeroam_actor, 0, sizeof g_freeroam_actor);
    memset(&g_freeroam_rs, 0, sizeof g_freeroam_rs);
    g_freeroam_actor.layer = 13u;                       /* the cast layer */
    g_freeroam_actor.sprite_table[0].bank       = 0x8bu; /* Arche body */
    g_freeroam_actor.sprite_table[0].frame_base = 0;
    g_freeroam_rs.active     = 1;
    g_freeroam_rs.world_x    = FREEROAM_ARCHE_SPAWN_WX;
    g_freeroam_rs.world_y    = FREEROAM_ARCHE_SPAWN_WY;
    g_freeroam_rs.facing     = CHAR_FACE_RIGHT;
    g_freeroam_rs.dst_base_x = -30;                     /* Arche cast render anchor */
    g_freeroam_rs.dst_base_y = -24;
    g_freeroam_rs.clip       = arche_freeroam_clip(/*moving=*/0, 0, 0);  /* idle */
    /* The LEFT-facing walk mirror for bank 0x8b (cels +4) — set the persisted
     * flip table so facing==3 picks the mirrored walk cels. */
    if ((uint16_t)0x8bu < (uint16_t)AR_SPRITE_SLOT_COUNT)
        g_actor_flip_table[0x8bu] = ARCHE_FREEROAM_FLIP;
    g_freeroam_active = 1;
    log_line("freeroam_begin: controllable Arche at world (%d,%d) — errands hand-off",
             FREEROAM_ARCHE_SPAWN_WX, FREEROAM_ARCHE_SPAWN_WY);
}

/* One freeroam sim-tick: run the bit-exact mover on the live held axis, mirror its
 * world pos/facing into the render-state, and advance the walk/idle clip. */
static void freeroam_step(void)
{
    /* Control is LOCKED while the errands OPENING DIALOGUE is pending or playing: retail
     * keeps Arche STATIONARY through the movement-tutorial lines and hands control off
     * only when they FINISH (USER ckpt 153 — the ckpt-152 "concurrent" hand-off applied
     * input too early; she shouldn't be controllable until the dialogue is over).  When
     * locked, drive the mover with a ZEROED axis (no ring either) so any held key/double
     * -tap is ignored and she brakes to + holds idle — not the live keyboard.  The gate:
     * g_errands_dlg_pending covers the fade-in window before the lines arm, then
     * cutscene_active covers the lines themselves (g_cutscene re-armed = the errands
     * dialogue during freeroam); both clear when the 3 lines complete. */
    static const int zero_axis[CHAR_AXIS_COUNT + 2] = {0};
    int locked = g_errands_dlg_pending || cutscene_active(&g_cutscene);
    /* axis[0..3] = UP/DOWN/LEFT/RIGHT (input_mgr +0x114 array A), [4] = jump (C, +0x124). */
    const int *axis = locked ? zero_axis
                             : (const int *)g_game_drive.input.axis_held;
    struct input_mgr *ring = locked ? NULL : &g_game_drive.input;
    /* jump = the C button LEVEL (axis_held[4]); character_step detects the rising
     * edge + runs the bit-exact windup/impulse/variable-height arc.  run = the DASH
     * flag, resolved from the live event ring by character_resolve_run (the 0x478ba0
     * double-tap detection 0x479e70, ckpt 150): a USER tap-tap-hold of a direction
     * dashes.  The ring records its timestamps in GetTickCount() ms, so the window
     * compare uses the same clock. */
    int jump = axis[CHAR_AXIS_COUNT];   /* axis_held[4] = the jump button level */
    uint32_t now = GetTickCount();
    int run  = character_resolve_run(&g_freeroam_char, ring,
                                     now, axis, CHAR_DASH_WINDOW_MS);
    /* Resolve the U/D POSE command (cmd[3] = 10 DOWN / 0xb UP) off the same ring each
     * tick — the self-sustain state must advance every tick (0x478ba0:248-259, ckpt 153);
     * it stores into g_freeroam_char.cmd_pose, which character_step reads for the apply
     * states 2/5 brake (crouch / slide / UP-stops-you-faster, ckpt 153 — bit-exact -800
     * vs runs/pose-demo/cap-body). */
    (void)character_resolve_pose(&g_freeroam_char, ring, now, axis);
    /* Resolve the SWORD unsheathe/sheathe TOGGLE off the same ring: a Z (ring 9)
     * press flips g_freeroam_char.sword_out (ckpt 155, USER ground truth).  ring is
     * NULL while locked (the opening dialogue), so Z is dead until control hands off
     * — matching retail's post-tutorial sword enable (PORT-DEBT(sword-quest-gate)). */
    (void)character_resolve_sword(&g_freeroam_char, ring, now);
    character_step(&g_freeroam_char, axis, jump, run);
    g_freeroam_rs.world_x = g_freeroam_char.world_x;
    g_freeroam_rs.world_y = g_freeroam_char.world_y;
    g_freeroam_rs.facing  = g_freeroam_char.facing;
    int moving = (g_freeroam_char.cmd_dir != 0) || (g_freeroam_char.vel != 0);
    /* The U/D-POSE sprite (crouch / up-defensive) takes priority over walk/idle:
     * arche_pose_clip drives the enter->hold->exit cel FSM off cmd_pose (ckpt
     * 153b, engine-quirk #114; retires PORT-DEBT(char-pose-anim)), falling
     * through to arche_freeroam_clip when no pose is engaged or exiting.  The
     * LEFT-facing pose/walk/idle emerge from the bank-0x8b +152 flip the renderer
     * applies on facing==3 (ARCHE_FREEROAM_FLIP) — so we render at the character
     * facing and the flip mirrors all the freeroam cels uniformly. */
    /* The SWORD draw/sheathe transient wins over the pose/walk/idle: arche_sword_clip
     * plays the UNSHEATHE cels 96-103 (~48t) on the sword_out 0->1 edge / a reverse
     * SHEATHE on 1->0, then delegates to arche_pose_clip when no transient is active
     * (the sword-out idle/walk reuse the base cels; ckpt 155, engine-quirk #115).  The
     * LEFT-facing draw emerges from the bank-0x8b +152 flip like every freeroam cel. */
    static arche_pose_anim  g_freeroam_pose_anim;
    static arche_sword_anim g_freeroam_sword_anim;
    const anim_clip *want = arche_sword_clip(&g_freeroam_sword_anim,
                                             g_freeroam_char.sword_out,
                                             &g_freeroam_pose_anim,
                                             g_freeroam_char.cmd_pose,
                                             moving, g_freeroam_char.airborne, run);
    if (g_freeroam_rs.clip != want) {
        g_freeroam_rs.clip  = want;
        g_freeroam_rs.timer = 0;
        g_freeroam_rs.frame = 0;
        g_freeroam_rs.done  = 0;
    }
    actor_anim_advance(&g_freeroam_rs);   /* advance the clip one sim-tick */
}

/* The in-game frame render (game_drive cfg.render).  Clears to black (the
 * faithful map-load fill) then, once the town scene is loaded (enter_game),
 * walks the static backdrop through the live-verified first-frame camera
 * MAP_RENDER_CAM_TOWN_3F2 — map_render_walk emits the visible tiles, map_present
 * projects + blits them via game_present_blit.  Before the scene loads (or if
 * the resource is unavailable) this is the black map-load frame retail shows
 * from game_enter (flip ~1092) until the town first renders (~flip 1150).
 *
 * DEFERRED: the entry fade + the black-load window timing (the port draws the
 * town from game_enter rather than after retail's ~58-flip load/fade), the
 * foreground-tree + dialogue/caption layers (PORT-DEBT ingame-nontile-layers;
 * present modes 0/1/2 = PORT-DEBT present-actor-modes), and retail's zoomed-out
 * intro establishing shot at the hold (PORT-DEBT ingame-establishing-zoom).
 * The parallax sky/mountain far-plane IS now drawn (FUN_00490cd0). */
/* Project the live camera_view (FUN_0043d1d0's view object) onto the mr_camera
 * subset the backdrop walk/parallax read — the same retail view-object at the
 * offsets each module models (+0x34/+0x4c accumulators, +0x5c/+0x60 scroll,
 * +0x64/+0x68 viewport; +0x74 vertical shear is 0 across the town pan). */
static void game_camera_to_mr(const camera_view *v, mr_camera *out)
{
    out->off34 = v->accum_x;   /* +0x34 */
    out->off4c = v->accum_y;   /* +0x4c */
    out->off5c = v->cur_y;     /* +0x5c */
    out->off60 = v->cur_x;     /* +0x60 */
    out->off64 = v->vp_w;      /* +0x64 */
    out->off68 = v->vp_h;      /* +0x68 */
    out->off74 = 0;            /* +0x74 — shear, 0 across the pan */
}

/* Advance the live camera one frame.  The in-game SIM update (retail 0x439690,
 * which processes the camera commands then calls the easer FUN_0043d1d0 at
 * :1123) runs at HALF the Flip rate — measured from a retail field-spec trace
 * across the pan: the easer fires once per 2 Flips and cam +0x60 cruises down
 * 300 per 2 Flips (cap 300/tick) = 150/flip; cam +0x60 is a STEP function,
 * flat between sim ticks.  The pan command fires at game_enter + 184 Flips
 * (Flip 1616 still HOLD tgt=128000/cap=0, Flip 1617 = PAN tgt=12800/cap=300).
 * The port presents once per frame, so gate the sim to every 2nd frame to match
 * retail's cadence.  (Sub-tick wall-clock jitter in retail's accumulator — the
 * 1618-1621 plateau, the 1616 double-tick — is NOT reproduced; the port is a
 * clean fixed 2:1 step.  PORT-DEBT ingame-camera-pan: the exact phase + the
 * cutscene-script TRIGGER source, vs this measured stand-in.) */
static void game_camera_step(void)
{
    /* Process the scripted pan command BEFORE the easer (retail order: 439690
     * sets tgt/cap at :599-664, then the easer eases at :1123).  Synthetic
     * trigger (PORT-DEBT ingame-camera-pan): the +0x4c command the cutscene
     * script fires at hold-end. */
    if (g_game_camera_hold == GAME_CAMERA_HOLD_FRAMES)
        camera_apply_pan(&g_game_camera, 12800, 12800, 300);

    /* The sim ticks every 2nd Flip (phase aligned so the trigger Flip is also a
     * sim tick — GAME_CAMERA_HOLD_FRAMES is even, matching retail's 1617 = pan
     * command + first easer tick on the same Flip). */
    if ((g_game_camera_hold & 1u) == 0u) {
        g_sim_tick_count++;     /* the capture's tick axis (retail: 0x43d1d0 hook) */
        /* Fields read at onEnter = the state going INTO the easer (retail reads
         * the view before the step), so emit before camera_follow_step. */
        CALL_TRACE_BEGIN(0x43d1d0);
        CALL_TRACE_I32("cur_x", g_game_camera.cur_x);
        CALL_TRACE_I32("tgt_x", g_game_camera.tgt_x);
        CALL_TRACE_I32("vel_x", g_game_camera.vel_x);
        CALL_TRACE_I32("vel_y", g_game_camera.vel_y);
        CALL_TRACE_I32("cap",   g_game_camera.cap);
        CALL_TRACE_I32("flag",  g_game_camera.flag);
        CALL_TRACE_END();
        camera_follow_step(&g_game_camera);
    }
    g_game_camera_hold++;   /* Flips since the spawn snap (drives trigger + phase) */
    game_camera_to_mr(&g_game_camera, &g_game_camera_mr);
}

/* The per-sim-tick actor UPDATE (0x46cd70 main-band slice → 0x54f980's frame
 * stepper): advance the room's animated actors' clips by one tick.  The
 * protagonist (0x1872d) trots its horses, and the EFFECT townsfolk run their
 * idle breathing clip (0x6290e0) from the RNG start phase the spawn set
 * (engine-quirk #76: the anim rides the sim-tick clock; #86: the start phase is
 * RNG).  Both steppers are deterministic (anim_clip_advance reads no RNG); the
 * RNG-driven behaviour half — wander / particles — stays deferred (ckpt 73). */
static void game_actor_update(void)
{
    int advanced = actor_pool_update(&g_actors);
    if (g_effects_loaded)
        advanced += actor_pool_update(&g_effects);  /* the townsfolk breathe */
    CALL_TRACE_BEGIN(0x46cd70);           /* port mirror of the per-tick driver */
    CALL_TRACE_HEX("rng", rng_peek_state()); /* the shared LCG state at onEnter   */
    CALL_TRACE_I32("advanced", advanced); /* port-side: actors stepped this tick */
    CALL_TRACE_END();

    /* The per-tick LCG stream, in 0x46cd70's band order (engine-quirk #95):
     *
     * (1) EFFECT band (0x47b990): the 4 BUTTERFLIES, the band's only per-tick RNG
     *     consumer (the townsfolk take the RNG-free arm).  Stepped FIRST so their
     *     draws precede the emitters' — keeping the shared stream aligned.
     *     Then the EFFECT-band event timer 0x467380 (the 0xe2a5 object, via
     *     0x442a70) — it follows the butterflies in slot order (fires tick 183). */
    /* Chip-1 validation emit (the sim-tick axis vs runs/butterfly-fsm): the
     * butterfly state at AI onEnter — i.e. BEFORE butterfly_step runs this tick's
     * AI+apply — mirroring the retail 0x47b990 onEnter capture (worldX is the
     * value left by the previous tick's apply).  One record per butterfly, in
     * registration (map) order; the validator matches by spawn worldX. */
    for (int i = 0; i < g_butterflies.count; i++) {
        butterfly *b = &g_butterflies.b[i];
        CALL_TRACE_BEGIN(0x47b990);
        CALL_TRACE_HEX("code", 0xe29au);
        CALL_TRACE_I32("sim_tick", (int32_t)g_sim_tick_count);
        CALL_TRACE_I32("wx", b->world_x);
        CALL_TRACE_I32("heading", b->heading);
        CALL_TRACE_I32("facing", b->facing);
        CALL_TRACE_I32("cooldown", b->cooldown);
        CALL_TRACE_I32("cmd_dir", b->cmd_dir);
        CALL_TRACE_I32("hvel", b->hvel);
        CALL_TRACE_END();
    }

    butterfly_step(&g_butterflies);
    /* The apply pass (inside butterfly_step) integrated each butterfly's open-air
     * worldX/facing every tick — mirror it into the rendered EFFECT actor's
     * render-state so the blit drifts with it (in retail 0x485fc0 writes the real
     * body; here the butterfly module owns the reduced open-air body). */
    if (g_effects_loaded) {
        for (int i = 0; i < g_butterflies.count; i++) {
            int slot = g_butterflies.b[i].effect_slot;
            if (slot >= 0 && slot < g_effects.count) {
                g_effects.states[slot].world_x = g_butterflies.b[i].world_x;
                g_effects.states[slot].world_y = g_butterflies.b[i].world_y;
                g_effects.states[slot].facing  = (int16_t)g_butterflies.b[i].facing;
            }
        }
    }

    /* THEME 2: advance Arche's run-off (the L7->L8 "runs to the house" beat).  One
     * sim-tick of accel/cruise/decel toward the house door; mirror world_x into her
     * render-state and switch her clip (run -> decel -> arrival-idle) on a phase
     * change.  actor_pool_update (top of this fn) steps the active clip each tick,
     * so the switch lands next tick (a 1-tick latency, immaterial to the cycle). */
    if (g_arche_runoff.active && g_effects_loaded &&
        g_arche_slot >= 0 && g_arche_slot < g_effects.count) {
        const anim_clip *c = arche_runoff_step(&g_arche_runoff);
        actor_render_state *rs = &g_effects.states[g_arche_slot];
        rs->world_x = g_arche_runoff.world_x;
        if (c != NULL && rs->clip != c) {
            rs->clip  = c;
            rs->timer = 0;
            rs->frame = 0;
            rs->done  = 0;
        }
    }

    ambient_effect_step(&g_ambient);

    /* (2) The PARTICLE band (+0x13e0, 0x46e510) steps BEFORE the CHARACTER band
     *     in 0x46cd70's walk (bands: 0x1160 EFFECT → 0x1060 → 0x13e0 PARTICLES →
     *     0x23e0 → 0x11e0 CHARACTER) — so a droplet spawned this tick renders
     *     UNSTEPPED (spawn pos, frame 6, timer 0) until the next tick.  The old
     *     emit-then-step order integrated fresh droplets one tick early — the R7
     *     one-anim-tick lead.  RNG-free, so the move leaves the stream intact. */
    particle_pool_step(&g_fountain_pp);

    /* (3) CHARACTER band (0x54f980): the particle EMITTERS.  The FOUNTAIN 0x112e5
     *     spawns one 0x18708 water droplet (6 draws); the SKY emitters 0x112e2
     *     each spawn one 0x18704 ambient particle every 6th tick (4 draws each). */
    if (g_fountain_loaded)
        particle_fountain_emit(&g_fountain_pp, g_fountain_cx, g_fountain_cy,
                               &g_fountain_counter);
    for (int i = 0; i < g_sky_emit_count; i++)
        particle_sky_emit(&g_fountain_pp, g_sky_cx[i], g_sky_cy[i],
                          &g_sky_counter[i]);

    /* (4) The CHARACTER-band tail: the two 0x5531b0 ambient sound emitters
     *     (0x1136f/0x11370) and the wagon 0x1872d's idle-wander, AFTER the
     *     fountain/sky in slot order (their tick-0 init draws then fire on cue at
     *     ticks 33/134/189).  With (1)-(4) the per-tick stream matches retail
     *     bit-exact across the whole settled-town window (engine-quirk #95) — this
     *     retires the RNG residual of PORT-DEBT(fountain-rng-phase). */
    ambient_character_step(&g_ambient);
}

static void game_render(void *user)
{
    (void)user;
    if (g_zdd == NULL || g_zdd->primary_obj == NULL)
        return;
    zdd_object_clear(g_zdd->primary_obj);    /* black map-load fill */
    if (g_town_loaded) {
        /* The live stepped camera (spawn snap → hold → scripted pan), falling
         * back to the static first-frame hold if it was never armed. */
        const mr_camera *cam = &MAP_RENDER_CAM_TOWN_3F2;
        if (g_game_camera_armed) {
            /* retail's in-game loop 0x439690 runs the per-tick body once per sim
             * tick: the actor update 0x46cd70 (:1108) THEN the camera easer
             * 0x43d1d0 (:1123).  Mirror that order + cadence — the port's sim
             * tick is every 2nd Flip (game_camera_step gates the easer on the
             * same g_game_camera_hold parity, then bumps it), so the horses trot
             * in lockstep with the pan.  Reset is automatic: enter_game
             * re-spawns the pool (frame/timer 0) and zeroes the hold counter. */
            int is_sim_tick = (g_game_camera_hold & 1u) == 0u;
            /* The town cast/effects (props/villagers/butterflies/arrival family)
             * are spawned for the TOWN (DATA 1022); a house/errands room swap
             * suppresses them (the room cast is PORT-DEBT cutscene-room-render
             * Phase 2b) so they don't render over the new backdrop. */
            int room_is_town = (g_loaded_room_key == CUTSCENE_ROOM_ARRIVAL);
            if (g_actors_loaded && is_sim_tick && room_is_town)
                game_actor_update();
            else if (is_sim_tick && g_room_cast_loaded) {
                /* Phase 2b: animate the house/errands room cast (the family's
                 * idle breathing) — the town's full game_actor_update doesn't
                 * run in non-town rooms, so step just the room cast's clips. */
                actor_pool_update(&g_room_cast);
                /* USER notes #3-5: when Arche's one-shot house TURN clip finishes
                 * (cel 7 held, rs->done), settle her to the post-turn standing idle
                 * (base-0 breathe) — RE'd off retail.osr res 0x570: 158(4t)→7(4t)
                 * →idle 0/1/2 (14t).  HOUSE_CAST[0] = Arche (bank 0x8b). */
                if (g_arche_house_turning && g_room_cast.count > 0 &&
                    g_room_cast.states[0].done) {
                    g_room_cast.states[0].clip  = arche_house_turn_idle_clip();
                    g_room_cast.states[0].frame = 0;
                    g_room_cast.states[0].timer = 0;
                    g_room_cast.states[0].done  = 0;
                    g_arche_house_turning = 0;
                }
            }
            /* Phase 2b: the FREEROAM mover — one sim-tick of controllable Arche on
             * the live held axis (the errands hand-off; independent of the room
             * cast above, both run in the errands). */
            if (is_sim_tick && g_freeroam_active)
                freeroam_step();
            game_camera_step();
            /* 0x439690:1124 — the scene cinematic step 0x499ab0 runs once/sim-tick
             * AFTER the camera easer (0x43d1d0:1123); it advances the REVEAL iris
             * (2 rows/tick -> 8px/sim-tick, the measured envelope).  Deterministic
             * (no RNG), so it rides the sim-tick clock like the actor steppers.
             * UNFENCED (R6 resolution, ckpt 106): retail's frame stamped tick u
             * presents the post-update-u grid (mask-level extraction on intro-1
             * at forced stamp equality: retail s5(a)==a exactly), so the port
             * steps BEFORE rendering every sim tick from the first.  The ckpt-105b
             * "hold>=2 one-tick fence" was a misfix: its dt-scan minimum was
             * computed over GRADED mask cels (one 5-bit step weak), which biased
             * the cost surface by exactly one tick.  With the cels ungraded
             * (quirk #96 family, slots 40/41 on the grade skip-list) the unfenced
             * step is bit-exact at stamp equality. */
            if (is_sim_tick) {
                /* Port mirror of the retail 0x499ab0 grid dump (R6-B): the
                 * fade-grid's column-0 cell per row, packed state|timer<<16,
                 * read BEFORE this tick's update = retail's onEnter view.
                 * Same field names as tools/flow/retail_fields.json. */
                CALL_TRACE_BEGIN(0x499ab0);
                CALL_TRACE_I32("sf_mode",    g_scene_fade.mode);
                CALL_TRACE_I32("sf_variant", g_scene_fade.variant);
                CALL_TRACE_U32("sf_rad",     g_scene_fade.radius);
                CALL_TRACE_I32("sf_done",    g_scene_fade.done);
                {
                    char rn[8];
                    for (int r = 40; r <= 80; r++) {
                        const scene_fade_cell *c =
                            &g_scene_fade.cells[r * SCENE_FADE_W];
                        snprintf(rn, sizeof rn, "r%d", r);
                        call_trace_field_u32(rn,
                            (uint32_t)c->state | ((uint32_t)c->timer << 16));
                    }
                }
                CALL_TRACE_END();
            }
            if (is_sim_tick)
                scene_fade_step(&g_scene_fade);
            /* The area-title banner (0x494a60) is updated by the SAME cinematic
             * step 0x499ab0 — arm it at the measured +78-flip trigger, then run
             * its mode-1 phase machine once/sim-tick. */
            if (is_sim_tick) {
                if (!g_banner_armed && g_game_camera_hold >= BANNER_ARM_FRAMES) {
                    banner_arm(&g_banner, "Town of Tonkiness", BANNER_HOLD_DUR);
                    g_banner_armed = 1;
                    log_line("enter_game: banner_arm \"Town of Tonkiness\" "
                             "@hold=%u (game_enter+%u)", g_game_camera_hold,
                             g_game_camera_hold);
                }
                banner_step(&g_banner);
                /* The town-intro CUTSCENE CHAIN (0x4d7d80 -> the beat-runner
                 * 0x439690).  Armed at the measured trigger (the same
                 * PORT-DEBT(dialogue-trigger) window as before, so LINE 1 stays
                 * bit-exact), then cutscene_step sequences the room chain
                 * (arrival 0x334be 10 lines -> house 0x334c8 8 lines) advancing
                 * on Z, modeling the 0x401d40/0x402030 room-key swap by walking
                 * the rooms list.  Box widget updates (pop-in / portrait fade /
                 * typewriter / arrow) run once per sim-tick inside cutscene_step
                 * -> dialogue_step.  PORT-DEBT(cutscene-room-render): the house
                 * backdrop is not loaded yet, so its lines play over the town
                 * scene; the chain ends at the errands boundary (0x334dc). */
                if (!g_cutscene_armed && g_game_camera_hold >= DIALOGUE_ARM_FRAMES) {
                    int n_rooms = 0;
                    const cutscene_room *chain = cutscene_town_chain(&n_rooms);
                    cutscene_arm(&g_cutscene, chain, n_rooms,
                                 exe_data_string, &g_dialogue);
                    if (cutscene_active(&g_cutscene))
                        log_line("enter_game: cutscene_arm town-intro chain "
                                 "(%d rooms) @hold=%u", n_rooms, g_game_camera_hold);
                    else
                        log_line("enter_game: cutscene town-intro line1 VA "
                                 "unresolved — box stays disarmed");
                    g_cutscene_armed = 1;
                }
                /* USER (ckpt 152): the errands OPENING DIALOGUE.  The town chain
                 * completed + handed off to freeroam (g_errands_dlg_pending set);
                 * once the entry reveal has fully receded (retail plays the dialogue
                 * AFTER the fade-from-black, not during), re-arm g_cutscene with the
                 * 3-line errands chain.  It then plays CONCURRENT with freeroam control
                 * (freeroam_step runs each sim-tick above; this drives the box + the
                 * confirm-advance below) — retail's beat pump 0x439680 likewise runs the
                 * game loop while each line waits.  All same-speaker (Arche) so the box
                 * stays up across the advances. */
                if (g_errands_dlg_pending && !scene_fade_active(&g_scene_fade)) {
                    g_errands_dlg_pending = 0;
                    int n_ev = 0;
                    const cutscene_room *ev = cutscene_errands_intro(&n_ev);
                    cutscene_arm(&g_cutscene, ev, n_ev, exe_data_string, &g_dialogue);
                    log_line("errands: opening dialogue armed (3 Arche lines) "
                             "@hold=%u — reveal complete", g_game_camera_hold);
                }
                /* Confirm (ENTER/X = ring id 0x24): poll+consume one edge from the
                 * ring; cutscene_step SKIPS the typewriter if the line is still
                 * revealing, else ADVANCES the fully-typed line (the 0x43bca0
                 * state-1 skip / 0x43b980 state-2 advance, ~2 confirms/line). */
                {
                    int confirm = input_poll_consume(&g_game_drive.input,
                                                     GetTickCount(),
                                                     CUTSCENE_ADVANCE_RING_ID);
                    /* THEME 3: feed the live scene_fade grid state so a FADE beat
                     * completes exactly when the grid settles (retail's case-2 gate),
                     * not a guessed duration. */
                    cutscene_set_fade_active(&g_cutscene,
                                             scene_fade_active(&g_scene_fade));
                    int done = cutscene_step(&g_cutscene, confirm);
                    /* The room-key swap drives the BACKDROP: when cutscene_step
                     * advances to a new room (arrival 0x334be -> house 0x334c8),
                     * reload that room's scene + snap its camera — the port's
                     * model of the 0x401d40 stage / 0x402030 commit -> 0x586010
                     * map reload (ckpt-130 harness-confirmed: a fresh 0x587e00
                     * per room).  Retires PORT-DEBT(cutscene-room-render). */
                    uint32_t want = cutscene_room_key(&g_cutscene);
                    if (want != 0 && want != g_loaded_room_key)
                        reload_room_backdrop(want);
                    /* THEME 3: a non-dialogue beat may have issued a one-shot this
                     * tick — the L7→L8 CAMERA PAN (the 0x43d1d0 easer follows Arche
                     * running ahead to 28000) or a scene-transition FADE.  Perform
                     * it against the live camera / scene_fade (cutscene.c is pure C
                     * so it cannot).  AFTER the room reload so a fade riding a room
                     * swap paints over the NEW backdrop (note #7's house reveal). */
                    cutscene_action act;
                    if (cutscene_take_action(&g_cutscene, &act)) {
                        if (act.kind == CS_ACT_CAMERA_PAN) {
                            camera_apply_pan(&g_game_camera, act.a, act.b, act.c);
                            log_line("cutscene beat: camera_apply_pan(%d,%d,%d) "
                                     "@hold=%u", act.a, act.b, act.c,
                                     g_game_camera_hold);
                            /* THEME 2: the L7->L8 camera pan IS the "Arche runs ahead
                             * to the house" beat — start her run-off (clip + motion)
                             * here, concurrent with the pan, so she runs to the door
                             * (USER note tick 1027).  Only the arrival's run-off pans
                             * (the establishing pan is a separate path; house entry
                             * fades), so the cutscene CAMERA_PAN action is unambiguous. */
                            if (g_arche_slot >= 0 && g_arche_slot < g_effects.count &&
                                !g_arche_runoff.active) {
                                arche_runoff_begin(&g_arche_runoff,
                                                   g_effects.states[g_arche_slot].world_x,
                                                   ARCHE_RUNOFF_TARGET_X);
                                log_line("cutscene beat: Arche run-off begin "
                                         "(slot %d, %d -> %d)", g_arche_slot,
                                         g_effects.states[g_arche_slot].world_x,
                                         ARCHE_RUNOFF_TARGET_X);
                            }
                        } else if (act.kind == CS_ACT_FADE) {
                            /* RE'd arm (0x439690:563): the iris VARIANT is an LCG
                             * draw (rand*3)>>15 in {0,1,2}.  act.a = mode
                             * (SCENE_FADE_MODE_OUT reveal / _IN cover), act.c = speed.
                             *
                             * PORT-DEBT(cutscene-fade-variant): retail's seed-pinned
                             * capture draws variant 0 (CENTER-OUT) for BOTH the
                             * arrival-exit cover (retail.osr tick 1234) AND the
                             * house-entry reveal (tick 1261) — confirmed off the draw
                             * stream (tools/trace_studio2/draw_probe.py: the black
                             * grows/recedes from the MIDDLE).  The port's LCG is
                             * aligned at game_enter (the establishing reveal correctly
                             * rolls 0, quirk #94) but DRIFTS by these arms because the
                             * unported cast (PORT-DEBT(cutscene-party-chars) — the
                             * running Arche, butterflies, villagers) consumes the LCG
                             * differently in between, so the live draw here rolls 1
                             * (edges-in).  Force the beat's fade_var (0 = center-out)
                             * as a cast-debt STAND-IN — the same family as the L7→L8
                             * run-off dur; it derives from the live RNG once the cast
                             * lands.  KEEP the rng_rand() draw to preserve retail's
                             * per-arm LCG consumption COUNT (one draw per fade arm). */
                            (void)rng_rand();         /* consume (retail draws per arm) */
                            int sf_var = act.b;       /* center-out stand-in, see above */
                            scene_fade_arm(&g_scene_fade, act.a, sf_var, act.c);
                            log_line("cutscene beat: scene_fade_arm(mode=%d var=%d "
                                     "speed=%d) @hold=%u [center-out stand-in]",
                                     act.a, sf_var, act.c, g_game_camera_hold);
                        } else if (act.kind == CS_ACT_ACTOR_TURN) {
                            /* USER notes #3-5: the house emote 0x401e60(Arche,1) —
                             * play her one-shot TURN clip (cels 158→7) on the
                             * room-cast Arche (HOUSE_CAST[0], bank 0x8b); when it
                             * finishes she settles to the post-turn standing idle
                             * (the done-swap after actor_pool_update).  RE'd off
                             * retail.osr res 0x570 (ticks 1579-1587). */
                            if (g_room_cast_loaded && g_room_cast.count > 0 &&
                                g_room_cast.actors[0].sprite_table[0].bank == 0x8bu) {
                                g_room_cast.states[0].clip  = arche_house_turn_clip();
                                g_room_cast.states[0].frame = 0;
                                g_room_cast.states[0].timer = 0;
                                g_room_cast.states[0].done  = 0;
                                g_arche_house_turning = 1;
                                log_line("cutscene beat: Arche house TURN begin "
                                         "(dir %d) @hold=%u", act.a,
                                         g_game_camera_hold);
                            }
                        }
                    }
                    if (done) {
                        log_line("game: town-intro cutscene chain COMPLETE @hold=%u "
                                 "-> errands room 0x334dc (the freeroam scene)",
                                 g_game_camera_hold);
                        /* The chain ends at the errands boundary: load that room's
                         * backdrop (the freeroam scene) and HAND OFF to the
                         * controllable mover (Phase 2b — the +0x200==0 char-AI path).
                         * Retail then plays a short errands OPENING DIALOGUE (the
                         * questline 0x4dc510's entry case — 3 Arche lines, the movement
                         * tutorial) once control is handed off; the port arms it after
                         * the entry reveal recedes (g_errands_dlg_pending below), played
                         * concurrent with freeroam by re-arming g_cutscene. */
                        if (g_loaded_room_key != CUTSCENE_ROOM_ERRANDS)
                            reload_room_backdrop(CUTSCENE_ROOM_ERRANDS);
                        /* THEME B: the errands ENTRY reveal (USER studio note #7,
                         * retail.osr tick ~1707).  The house EXIT (cutscene.c
                         * HOUSE_EXIT) just covered the screen to full black with the
                         * errands key staged under it; the room reload above loaded
                         * the errands backdrop UNDER that black.  Now recede the black
                         * with a fade-FROM-black REVEAL — CENTER-OUT (variant 0):
                         * retail.osr shows the errands opening from the middle band
                         * outward (full-frame dump ticks 1707-1725).  scene_fade_arm
                         * re-fills the grid all-opaque (state 0) so the first rendered
                         * tick stays fully black, then scene_fade_step recedes it
                         * (~25 sim-ticks).  This stands in for the errands script's
                         * (0x4dc510) own reveal arm — that script + its opening
                         * dialogue are unported (PORT-DEBT(cutscene-scene-chain)), so
                         * the port arms the reveal directly on the hand-off.  Same
                         * forced-variant + per-arm rng_rand() draw as the cutscene
                         * fades (PORT-DEBT(cutscene-fade-variant)). */
                        if (!g_freeroam_active) {
                            (void)rng_rand();   /* per-arm LCG draw (retail draws var) */
                            scene_fade_arm(&g_scene_fade, SCENE_FADE_MODE_OUT,
                                           /*variant=*/0, 1000);
                            log_line("chain-complete: errands reveal "
                                     "scene_fade_arm(mode=OUT var=0 speed=1000)");
                            freeroam_begin();
                            /* USER (ckpt 152): play the errands OPENING DIALOGUE — defer
                             * the arm until the reveal recedes (retail shows it AFTER the
                             * fade-from-black, line 1 first-glyph ~tick 1758 vs reveal
                             * ~1707-1725).  The deferred-arm check (before cutscene_step)
                             * re-arms g_cutscene with the 3-line errands chain. */
                            g_errands_dlg_pending = 1;
                        }
                    }
                }
            }
            cam = &g_game_camera_mr;
        }
        /* 0x48c150 order: the parallax far-plane FIRST (0x490cd0, behind the
         * tiles), then the tilemap walk + present (0x490f30 / 0x48eac0). */
        town_render_parallax(&g_town, cam, game_parallax_blit, NULL);
        int deferred = 0;
        /* The tile walk + the CHARACTER-band actor walk (game_actor_walk) +
         * the present, in one pass — actor mode-0 nodes get their cull box from
         * game_cel_dims and blit via game_present_blit's PRESENT_KEYED arm. */
        town_render_step_ex(&g_town, cam,
                            game_sprite_resolve, NULL,
                            game_present_blit, NULL,
                            game_cel_dims, NULL,
                            /* game_actor_walk now renders for EVERY room: the
                             * map-driven STRUCTURE band (room scenery/furniture)
                             * always, the town cast bands only when room_is_town. */
                            game_actor_walk, NULL, &deferred);
        /* 0x48c150:124-162 — the cinematic letterbox tiled ON TOP of the
         * backdrop (after the present pass), during the establishing shot.
         * GATED to the TOWN/arrival room: retail's letterbox is the town-arrival
         * establishing-shot framing (quirk #74) and is DROPPED at the arrival→house
         * room swap — verified off retail.osr (present through the arrival t1190-1245,
         * GONE in the house t1291+, USER studio note).  The room swap sets
         * g_loaded_room_key to the house under the cover, so the bars vanish under
         * full black and the house renders bar-free, matching retail.  The constant
         * heights stay PORT-DEBT(ingame-letterbox) — only the on/off is now faithful. */
        if (g_loaded_room_key == CUTSCENE_ROOM_ARRIVAL)
            letterbox_render(g_letterbox_top, g_letterbox_bottom,
                             game_letterbox_blit, NULL);
        /* 0x48c150:175 — the establishing REVEAL fade-grid, rendered AFTER the
         * letterbox bars: the black iris that opens the town from center-out. */
        scene_fade_render(&g_scene_fade,
                          game_scene_fade_opaque, game_scene_fade_alpha, NULL);
        /* 0x48c150:176-178 — the area-title banner ("Town of Tonkiness"),
         * rendered AFTER the reveal grid (slot0 of the 3 banner calls). */
        game_render_banner();
        /* 0x48c150:179 — the widget tree (0x48c820): the dialogue bubble. */
        game_render_dialogue();
    }
}

/* The in-game seam.  On the prologue's 3rd beat (PROLOGUE_DONE) retail's boot
 * driver calls 0x59ec30(0,0,0x3f2) — the scene LOAD/UNLOAD wrapper around the
 * in-game engine 0x59f2c0(map=0x3f2,…), which loads + runs the opening map
 * (the town of Tilelia → intro story dialogue; retail golden runs/tas-ingame-1,
 * renders from ~flip 1150).  The engine is unported (milestone 2: 0x59f2c0 is
 * 3522 B of setup around the per-frame update 0x586010 (6 KB) + render dispatch
 * 0x5a00c0 (13.7 KB)).  enter_game emits the game_enter TAS anchor — matching
 * the retail-side 0x59f2c0 anchor so tas_diff aligns the in-game frames — and
 * stands up a game_drive (mirror of prologue_drive): one game_drive_step per
 * presented frame renders the faithful black map-load frame + presents, so the
 * early in-game frames now match retail's black entry window (the prior stub
 * wrongly re-displayed the title).  When 0x5a00c0 is ported, game_render
 * grows into the town render walk and game_drive_step returns GAME_EXIT on the
 * engine's scene-transition codes.  Full plan: docs/findings/in-game-intro.md. */
static void enter_game(void)
{
    if (g_prologue_active) {
        prologue_drive_shutdown(&g_prologue_drive);
        g_prologue_active = 0;
    }
    emit_anchor("game_enter");

    /* RNG ANCHOR (ckpt 86): re-seed the engine LCG here, at scene entry, BEFORE
     * the town spawn consumes any rand().  The title->town RNG is
     * non-deterministic run-to-run even under the boot seed-pin (engine-quirk
     * #77: a per-present consumer desyncs the shared stream between the title
     * sparkle pin and the town), so the town SPAWN burst (the EFFECT activator
     * 0x41f200's per-object position jitter / idle-phase / particle draws) would
     * otherwise start from an unpredictable phase.
     *
     * Retail re-pins DAT_008a4f94 at the matching point: the FIRST 0x41f200
     * call after the game_enter anchor (the agent arms at 0x59f2c0, fires at
     * 0x41f200 onEnter — NOT at game_enter itself, because a pre-spawn one-off
     * draw 0x4c5e00 fires in between).  This re-seed is the port's mirror of THAT
     * point: nothing between here and actor_spawn_effect_from_map (the port's
     * effect-spawn replay) draws rand — the character/protagonist/structure
     * spawns are RNG-free, exactly as retail's 0x431e30/0x438a60 dispatches are —
     * so the effect spawn is the first consumer on both sides and the two streams
     * march in lockstep.  (Invariant: keep all pre-effect-spawn enter_game code
     * RNG-free, or move this re-seed down to the effect spawn.)  USER directive:
     * pin the RNG seed on both sides, compare by anchor/tick. */
    {
        uint32_t before = rng_peek_state();
        rng_srand(game_rng_seed());
        osr_emit_seed(g_present_frame, before, game_rng_seed());
    }

    game_drive_cfg cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.render  = game_render;
    cfg.present = drive_present;   /* same present thunk: captures + bumps Flip */
    cfg.user    = &g_game_drive;
    game_drive_init(&g_game_drive, &cfg);
    g_game_active = 1;

    /* Arm the in-game palette color-grade (FUN_00562ea0's LUT, gates 700/850 —
     * the town defaults; PORT-DEBT color-grade-gates to read them from config).
     * Set BEFORE the town banks decode (they decode lazily in game_render), so
     * title_sheet_format grades them while the already-converted title sheets
     * stay untouched. */
    if (!g_color_grade_on) {
        color_grade_build_lut(g_color_lut,
                              COLOR_GRADE_TOWN_GATE1, COLOR_GRADE_TOWN_GATE2);
        g_color_grade_on = color_grade_is_active(COLOR_GRADE_TOWN_GATE1,
                                                 COLOR_GRADE_TOWN_GATE2);
    }

    /* Build the town backdrop scene from the map's DATA resource.  Map 0x3f2 →
     * room 210110 "Town of Tonkiness" → scene 1022 (proven in game_world/game_map,
     * findings/in-game-intro.md).  Retail keys the resource on room[3] = the scene
     * index (FUN_00586010:690 → FUN_00587970(EXE, scene)); we resolve it the same
     * way once game_map is wired, but for the opening town it is the constant
     * 1022.  On failure game_render falls back to the faithful black frame. */
    g_town_loaded = 0;
    g_actors_loaded = 0;
    g_structs_loaded = 0;
    g_effects_loaded = 0;
    /* Map 0x3f2 → room 0x334be "Town of Tonkiness" → DATA scene 1022 (resolved
     * from the registry: scene + parallax params now come from the room record,
     * not a hardcoded constant — the cutscene room swap re-loads the same way). */
    load_room(/*room_key=*/0x334beu);

    /* Register Arche's EXE-embedded body banks 0x8b-0x8e (slots 126-129, res
     * 0x570-0x573) now that load_town_scene has opened the sotes.exe datafile.
     * These DATA sprites live ONLY in sotes.exe's .rsrc (the same ids are WAVE
     * sounds in sotesd.dll), loaded from the user's own file at runtime — not
     * embedded.  With them registered, the party leader Arche (bank 0x8b) decodes
     * and renders instead of culling (ckpt 94).  Idempotent re-register per enter. */
    ar_register_party_exe_sprites(g_zdd, g_sotes_exe);

    /* Arm the live in-game camera once the map dims are known (the easer clamps
     * the pan target to [0, map_w-vp_w] x [0, map_h-vp_h]).  camera_apply_snap
     * spawn-snaps cur=tgt to the hold origin (128000/12800 = 40/4 cells, the
     * live-probed MAP_RENDER_CAM_TOWN_3F2 value); game_render then steps it. */
    g_game_camera_armed = 0;
    if (g_town_loaded) {
        memset(&g_game_camera, 0, sizeof g_game_camera);
        g_game_camera.map_w = (int32_t)g_town.map.dim0 * 0xc80;
        g_game_camera.map_h = (int32_t)g_town.map.dim1 * 0xc80;
        g_game_camera.vp_w  = MAP_RENDER_CAM_TOWN_3F2.off64;   /* 64000 */
        g_game_camera.vp_h  = MAP_RENDER_CAM_TOWN_3F2.off68;   /* 48000 */
        camera_apply_snap(&g_game_camera,
                          MAP_RENDER_CAM_TOWN_3F2.off60,        /* 128000 */
                          MAP_RENDER_CAM_TOWN_3F2.off5c);       /* 12800  */
        g_game_camera_hold  = 0;
        game_camera_to_mr(&g_game_camera, &g_game_camera_mr);
        g_game_camera_armed = 1;

        /* Arm the establishing-shot letterbox (top == bottom == 64 = quirk #74).
         * PORT-DEBT(ingame-letterbox): a constant stand-in for the 0x5a00c0
         * cutscene op that writes the scene-controller's +0x44/+0x48 bar
         * heights; the grid-fill is bit-exact (letterbox.c). */
        g_letterbox_top    = LETTERBOX_INTRO_BAR;
        g_letterbox_bottom = LETTERBOX_INTRO_BAR;

        /* Spawn the room's CHARACTER-band actors from the map (0x58d460 ->
         * 0x431e30 slice).  game_actor_walk then emits them each frame. */
        int n = actor_spawn_from_map(&g_actors, &g_town.map);

        /* Plus the animated protagonist (code 0x1872d) — the town intro
         * cutscene spawn (0x4d7d80 -> 0x431d10 -> 0x431e30 case-0x1872d).
         * Static stand-in at the census world pos (54400, 32000); it enters the
         * window only once the camera pans left off the 128000 hold.  See
         * actor_spawn.h / findings "The protagonist SPAWN". */
        int ps = actor_spawn_protagonist(&g_actors, 54400, 32000);
        g_actors_loaded = (n > 0 || ps >= 0);
        log_line("enter_game: actor_spawn_from_map -> %d CHARACTER actors "
                 "+ protagonist slot %d (5 visible props + 27 volumes, DATA 1022)",
                 n, ps);

        /* Spawn the room's STRUCTURE-band scenery from the map (0x58d460 ->
         * 0x438a60 slice) — fully map-driven (the TREE 0xec55, hedges 0xec60,
         * decorations 0xec6a; quirk #84).  game_actor_walk also walks g_structs. */
        int sn = actor_spawn_struct_from_map(&g_structs, &g_town.map);
        g_structs_loaded = (sn > 0);
        log_line("enter_game: actor_spawn_struct_from_map -> %d STRUCTURE objects "
                 "(tree + hedges + decorations, DATA 1022)", sn);

        /* Spawn the room's EFFECT-band townsfolk from the map (0x58d460 ->
         * 0x41f200 slice) — the standing villagers in the square, map-driven
         * (world = (map - dst) * 100), frozen on the idle clip's frame 0.
         * game_actor_walk also walks g_effects (layer 13).  The 4 wandering
         * 0xe29a + the non-map party townsfolk are deferred (RNG / Phase 2). */
        butterfly_pool_reset(&g_butterflies);
        ambient_reset(&g_ambient);   /* the irregular ambient/event RNG timers */
        int en = actor_spawn_effect_from_map(&g_effects, &g_town.map, &g_butterflies);
        g_effects_loaded = (en > 0);
        /* Fill the mirror/flip table so the facing==3 townsfolk pick the mirrored
         * cel (frame_base + flip).  Faithful to retail's DAT_008a8440 global. */
        int fn = actor_spawn_effect_fill_flip_table(g_actor_flip_table,
                                                    AR_SPRITE_SLOT_COUNT);
        log_line("enter_game: actor_spawn_effect_from_map -> %d EFFECT townsfolk "
                 "(standing villagers, DATA 1022; %d flip-table banks; 0xe29a "
                 "wanderers deferred)", en, fn);

        /* The town-intro cutscene arrival family (0x4d7d80 -> 0x41f0e0), resolved
         * through the dramatist table (party.c): Dr. Barnard (0xc3f0 -> 0xeb),
         * Arche's Father (0xc3dc -> 0xe3), Arche's Mother (0xc440 -> 0xb5 — her
         * own sheet, ckpt 92), and Arche the party LEADER (0xc35a -> body bank
         * 0x8b, the EXE banks registered just above, ckpt 94).  Appended to the
         * EFFECT pool (layer-13 path); she renders via the same 0x493ba0 static
         * path as the rest of the cast (PORT-DEBT cutscene-party-chars: the party
         * band 0x4997b0 + multi-part body remain Phase 2). */
        if (g_effects_loaded) {
            int cn = actor_spawn_cutscene_cast(&g_effects, g_actor_flip_table,
                                               AR_SPRITE_SLOT_COUNT);
            log_line("enter_game: actor_spawn_cutscene_cast -> %d arrival family "
                     "(Dr. Barnard 0xeb / Father 0xe3 / Mother 0xb5 / Arche 0x8b)",
                     cn);
            /* THEME 2: find Arche's slot (the cast member on body bank 0x8b) so the
             * run-off driver can move/animate her on the L7->L8 beat. */
            g_arche_slot = -1;
            for (int i = 0; i < g_effects.count; i++)
                if (g_effects.actors[i].sprite_table[0].bank == 0x8bu) {
                    g_arche_slot = i;
                    break;
                }
        }
        g_arche_runoff.active = 0;   /* not running until the L7->L8 beat fires */

        /* Arm the establishing REVEAL fade-grid (0x439690:555-583).  Town params
         * (live: runs/reveal-grid): mode 1 (fade-out), speed 1000; the iris VARIANT
         * is the LCG draw (rand*3)>>15 in {0,1,2} = center-out/edges-in/sweep.
         * Retail arms it in the first in-game frame FSM AFTER the room-load EFFECT
         * spawn burst (19 objects = 15 map + 4 cutscene cast, all consumed above),
         * so the variant is drawn at the post-spawn phase.  With the full 213-draw
         * burst now replayed (171 map via effect_from_map + 42 cutscene via
         * cutscene_cast) the draw yields retail's town value 0 (center-out) — proven
         * offline from the pinned seed and live (engine-quirk #94).  This retires the
         * RNG-phase half of PORT-DEBT(scene-fade-rng-phase); the residual is now only
         * the skipped black-load WINDOW (the reveal's absolute start tick offset). */
        int sf_variant = (int)((rng_rand() * 3u) >> 15);
        scene_fade_arm(&g_scene_fade, SCENE_FADE_MODE_OUT, sf_variant, 1000);
        log_line("enter_game: scene_fade_arm mode=1 speed=1000 variant=%d "
                 "(0=center-out; DRAWN at the post-spawn LCG phase, engine-quirk #94)",
                 sf_variant);

        /* The area-title banner arms later (at +78 flips, in the sim-tick block,
         * engine-quirk #96), not here — just clear it + the one-shot latch so a
         * re-entry re-arms and no stale card renders during the 0..78 window. */
        g_banner.enable = 0;
        g_banner.composed = 0;
        g_banner_armed = 0;

        /* The town-arrival cutscene + its box likewise arm later (at the
         * measured trigger); clear the state + the one-shot latch so a
         * re-entry re-arms cleanly. */
        memset(&g_dialogue, 0, sizeof(g_dialogue));
        memset(&g_cutscene, 0, sizeof(g_cutscene));
        g_cutscene_armed = 0;
        g_errands_dlg_pending = 0;

        /* Arm the PARTICLE band (Chip 3+).  Two emitters, both CHARACTER props
         * already in g_actors, both feeding the shared +0x13e0 pool (g_fountain_pp):
         *   - the FOUNTAIN 0x112e5 (bank 0x16c frame 36) emits 0x18708 water every
         *     primary sim-tick (engine-quirk #87, "The FOUNTAIN SPRAY").
         *   - the SKY emitters 0x112e2 emit 0x18704 ambient particles every 6th
         *     sim-tick ("The SKY-AMBIENT particles").
         * Find each, cache its anchor-center, reset its counter. */
        particle_pool_reset(&g_fountain_pp);
        g_fountain_loaded = 0;
        g_fountain_counter = FOUNTAIN_CYCLE_INIT;
        g_sky_emit_count = 0;
        for (int i = 0; i < g_actors.count; i++) {
            uint32_t code = g_actors.actors[i].code;
            if (code == 0x112e5u && !g_fountain_loaded) {
                /* fountain prop: 0x557370 mode-1 anchor = world + empirical +1600
                 * both axes (= +0xc/4; the decompile reads /2 — open 2×, above). */
                g_fountain_cx = g_actors.states[i].world_x + FOUNTAIN_EMIT_X_OFF;
                g_fountain_cy = g_actors.states[i].world_y + FOUNTAIN_EMIT_Y_OFF;
                g_fountain_loaded = 1;
            } else if (code == 0x112e2u && g_sky_emit_count < SKY_EMIT_MAX) {
                /* sky trigger: anchor KEPT at 0 (the prop's world pos).  NOTE
                 * (R7, ckpt 107): the 0x557370 field-spec reads this prop's
                 * box=3200 too (→ mode-1 anchor would be +1600), contradicting
                 * the old quirk-#88 "+0xc==0".  Out of R7 scope (fountain-only;
                 * smoke is letterbox-occluded in the reveal window + was USER-1:1
                 * at the settled town) — kept 0 pending a settled-town smoke
                 * render_diff.  TODO(sky-anchor): validate the +1600 anchor. */
                g_sky_cx[g_sky_emit_count] = g_actors.states[i].world_x;
                g_sky_cy[g_sky_emit_count] = g_actors.states[i].world_y;
                g_sky_counter[g_sky_emit_count] = 0;
                g_sky_emit_count++;
            }
        }
        log_line("enter_game: fountain emitter 0x112e5 %s (emit center %d,%d); "
                 "%d sky-ambient emitter(s) 0x112e2",
                 g_fountain_loaded ? "found" : "NOT found",
                 g_fountain_cx, g_fountain_cy, g_sky_emit_count);
        for (int i = 0; i < g_sky_emit_count; i++)
            log_line("enter_game:   sky emitter[%d] 0x112e2 center %d,%d",
                     i, g_sky_cx[i], g_sky_cy[i]);
    }

    log_line("enter_game: 0x59ec30(0,0,0x3f2) — opening map 0x3f2 → room 210110 "
             "(scene 1022); town backdrop scene %s",
             g_town_loaded ? "loaded — rendering tiles" : "unavailable — black frame");
}

/* Retail's "Start Game" commit, on the way from the new-game config scene to
 * the prologue cutscene, writes the initial save file — 0x5b6990 (resource
 * 0x2711, save subsystem unported) — and obfuscates the buffer with a salt
 * built from TWO rand() draws
 * (FUN_005bf505 ×2 @ 0x5b6acc/0x5b6ae9).  The save subsystem itself is deferred
 * (milestone 4), but those two draws are on the TAS critical path: they advance
 * the engine LCG between the newgame_enter and prologue_enter anchors, so
 * without them the port's seed desyncs from retail exactly at the cutscene
 * (port 0x404a0a8f vs retail 0x40d00581 — pinpointed by --rand-probe: 2 calls,
 * callers 0x5b6acc/0x5b6ae9).  Consume the two values so the RNG state matches
 * when the cutscene starts and any rand-driven cutscene effect stays in sync.
 * (The salt's *value* is irrelevant to parity — only the seed advance is.) */
static void newgame_start_save_salt(void)
{
    (void)rng_rand();   /* FUN_005bf505 @ 0x5b6acc */
    (void)rng_rand();   /* FUN_005bf505 @ 0x5b6ae9 */
}

/* Anchor CWD + DLL search dir to the game directory.  Reads
 * OPENSUMMONERS_GAME_DIR from the environment — nix develop's shellHook
 * exports it with WSLENV's /p flag so the .exe gets a Windows-form path
 * even though we cross from WSL into Windows via WSLInterop. */
/* Resolve a CLI file-path buffer to a fully-qualified absolute path, in place,
 * while the CWD is still the launch dir.  No-op on an empty buffer (arg not
 * given) or if GetFullPathNameA fails (left as-is + logged).  See the call site
 * in WinMain for why this must run before init_game_dir's chdir. */
static void resolve_launch_path(char *buf, size_t buflen, const char *what)
{
    if (buf == NULL || buf[0] == '\0') return;
    char abs[1024];
    DWORD n = GetFullPathNameA(buf, (DWORD)sizeof(abs), abs, NULL);
    if (n == 0 || n >= sizeof(abs)) {
        log_line("%s: GetFullPathNameA('%s') failed (%lu) — using as-is",
                 what, buf, GetLastError());
        return;
    }
    strncpy(buf, abs, buflen - 1);
    buf[buflen - 1] = '\0';
    log_line("%s resolved to %s", what, buf);
}

static void init_game_dir(void)
{
    char dir[MAX_PATH] = {0};
    DWORD n = GetEnvironmentVariableA("OPENSUMMONERS_GAME_DIR", dir, sizeof(dir));
    if (n == 0 || n >= sizeof(dir)) {
        /* Actionable, not just informative: an unset game dir means the
         * game-dir DLLs (sotesd/sotesp/sotesw) won't be on the search path,
         * sotesd.dll fails LoadLibrary (err 126), no sprite bank decodes, and
         * the screen renders BLANK/BLACK.  This is the #1 "why is it black"
         * footgun — run the launcher INSIDE `nix develop` (its shellHook
         * exports OPENSUMMONERS_GAME_DIR in Windows form via WSLENV /p). */
        log_line("OPENSUMMONERS_GAME_DIR unset — staying in CWD; "
                 "sotesd.dll will likely fail to load → BLANK render. "
                 "Run inside `nix develop` (it exports OPENSUMMONERS_GAME_DIR).");
        return;
    }
    if (!SetCurrentDirectoryA(dir)) {
        log_line("SetCurrentDirectoryA(%s) failed: %lu", dir, GetLastError());
        return;
    }
    /* SetDllDirectoryA puts this dir at the head of the DLL search path
     * for any further LoadLibrary calls.  Combined with the SafeDllSearch
     * default, the game-dir DLLs (sotesp/sotesd/sotesw) win over anything
     * the OS would otherwise pull from system32. */
    SetDllDirectoryA(dir);
    log_line("init_game_dir: cd %s", dir);
}

static int acquire_singleton(void)
{
    g_singleton_mutex = CreateMutexA(NULL, TRUE, SINGLETON_MUTEX_NAME);
    if (!g_singleton_mutex) {
        log_line("CreateMutexA failed: %lu", GetLastError());
        return 0;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        log_line("Another OpenSummoners instance is already running — exiting");
        CloseHandle(g_singleton_mutex);
        g_singleton_mutex = NULL;
        return 0;
    }
    return 1;
}

static void release_singleton(void)
{
    if (g_singleton_mutex) {
        ReleaseMutex(g_singleton_mutex);
        CloseHandle(g_singleton_mutex);
        g_singleton_mutex = NULL;
    }
}

static int register_window_class(void)
{
    WNDCLASSA wc = {0};
    wc.style         = CS_OWNDC;
    wc.lpfnWndProc   = wndproc;
    wc.hInstance     = g_hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    /* No erase brush: the per-frame present fully repaints the client, so a
     * background erase on activate/uncover would only flash a solid colour
     * under the next blit → flicker.  Render windows (incl. retail) leave this
     * NULL and suppress WM_ERASEBKGND (see wndproc).  Quirk #55. */
    wc.hbrBackground = NULL;
    wc.lpszClassName = OPENSUMMONERS_CLASS;
    return RegisterClassA(&wc) != 0;
}

static int create_main_window(void)
{
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT rc = { 0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT };
    AdjustWindowRect(&rc, style, FALSE);
    g_hwnd = CreateWindowExA(
        0,
        OPENSUMMONERS_CLASS, OPENSUMMONERS_TITLE,
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, g_hInstance, NULL);
    return g_hwnd != NULL;
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        g_shutdown = 1;
        PostQuitMessage(0);
        return 0;
    case WM_MOVE:
        /* Keep ZDD screen_pos_x/y in sync with the window's client
         * top-left so mode-2 present's desktop BitBlt lands inside
         * the window. */
        sync_window_position();
        return 0;
    case WM_PAINT:
        /* Port of retail's WM_PAINT consumption via FUN_005b9130
         * (zdd_window_paint).  Only consumes in mode 2 (Windowed) —
         * other modes (and unfinished DDraw init) fall through to
         * DefWindowProcA, which validates the dirty region itself.
         * Wired here directly (not via wp_handle_message) because the
         * drop-in uses its own minimal wndproc — the ported WndProc
         * module is still in isolation pending input-subsystem ports. */
        if (!g_skip_ddraw && g_zdd != NULL &&
            zdd_window_paint(g_zdd, hwnd)) {
            return 0;
        }
        break;
    case WM_ERASEBKGND:
        /* Suppress the background erase: the present repaints the whole
         * client every frame, so erasing first only flashes under the blit
         * (the focus/activate flicker).  Return non-zero = "erased". */
        return 1;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

/* Sample the window's client-area top-left in screen coordinates and
 * stamp it on g_zdd.  Called once after init_ddraw and on every
 * WM_MOVE thereafter.  No-op when DDraw is disabled or the window
 * isn't created yet. */
static void sync_window_position(void)
{
    if (g_zdd == NULL || g_hwnd == NULL) return;
    POINT pt = {0, 0};
    if (ClientToScreen(g_hwnd, &pt)) {
        g_zdd->screen_pos_x = (int32_t)pt.x;
        g_zdd->screen_pos_y = (int32_t)pt.y;
    }
}

/* Snapshot the real keyboard into the DIK-indexed buffer the producer reads
 * (the leaf 0x5ba520 = keyboard_state[scancode] & 0x80).  GetAsyncKeyState's
 * high bit is the physical-down state; map each bound DIK scancode to its VK.
 * Only the mapped keys are filled (the producer only ever reads those). */
static void live_keyboard_snapshot(uint8_t dik[256])
{
    static const struct { uint8_t sc; int vk; } M[] = {
        { DIK_UP_ARROW,    VK_UP    }, { DIK_DOWN_ARROW,  VK_DOWN  },
        { DIK_LEFT_ARROW,  VK_LEFT  }, { DIK_RIGHT_ARROW, VK_RIGHT },
        { DIK_Z, 'Z' }, { DIK_X, 'X' }, { DIK_C, 'C' },
    };
    memset(dik, 0, 256);
    for (int i = 0; i < (int)(sizeof(M) / sizeof(M[0])); i++)
        if (GetAsyncKeyState(M[i].vk) & 0x8000)
            dik[M[i].sc] = 0x80;
}

/* Feed one frame of input into the active drive's input manager.  REPLAY wins
 * (the deterministic parity path): if a --input-trace and/or --held-trace is
 * active, inject the due events and return.  Otherwise drive the LIVE keyboard
 * producer (0x46a880) — the interactive path — gated on the window being
 * focused (g_app_active_flag; retail pauses input on WM_ACTIVATEAPP deactivate,
 * wnd_proc on_deactivate).  The two paths are mutually exclusive so live
 * (wall-clock) input never perturbs a capture. */
static void feed_input(input_mgr *m, uint32_t now)
{
    if (g_input_trace_active || g_held_trace_active) {
        if (g_input_trace_active)
            input_trace_replay(&g_input_trace, g_present_frame,
                               g_sim_tick_count, m, now);
        if (g_held_trace_active)
            held_trace_replay(&g_held_trace, g_present_frame, m);
        return;
    }
    if (g_app_active_flag == 0)
        return;                       /* unfocused → input paused (faithful) */
    uint8_t dik[256];
    live_keyboard_snapshot(dik);
    input_live_step(&g_input_live, m, dik, now);
}

static void main_loop_body(void)
{
    /* Open this call-trace frame before any traced work.  Frame axis =
     * present count (g_frame_counter), matching the agent's Flip-anchored
     * counter on the retail side. */
    call_trace_begin_frame(g_frame_counter);

    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            g_shutdown = 1;
            return;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    if (g_prologue_active) {
        /* The Elemental-Stone cutscene: one prologue_drive_step per presented
         * frame (the FUN_0056cd20 update/render tick).  Same replay-inject-then-
         * step shape as the newgame scene; on DONE the game proper is unported. */
        uint32_t now = GetTickCount();
        feed_input(&g_prologue_drive.input, now);
        prologue_status st = prologue_drive_step(&g_prologue_drive, now);
        if (st == PROLOGUE_DONE)
            enter_game();   /* 3rd beat → 0x59ec30(0,0,0x3f2) in-game seam */
        else if (st == PROLOGUE_ABORT)
            leave_prologue_to_title("aborted (id 0x22)");
    } else if (g_game_active) {
        /* The in-game map drive (0x59f2c0 seam): one game_drive_step per
         * presented frame.  The engine is unported, so this renders the faithful
         * black map-load frame + presents (matching retail's black entry window).
         * Inject any due replay input so the trace's in-game Z presses land in
         * the drive's ring for when the engine (input-driven) is ported. */
        uint32_t now = GetTickCount();
        feed_input(&g_game_drive.input, now);
        (void)game_drive_step(&g_game_drive, now);   /* GAME_RUNNING until ported */
    } else if (g_newgame_active) {
        /* The new-game config scene: one newgame_drive_step per presented frame
         * (no pace machine — frame_limiter gates the rate).  Inject any due
         * replay events into the drive's ring before it polls (keyed on the
         * Flip count drive_present bumps), then act on the outcome. */
        uint32_t now = GetTickCount();
        feed_input(&g_newgame_drive.input, now);
        newgame_scene_status st = newgame_drive_step(&g_newgame_drive, now);
        if (st == NEWGAME_START) {
            newgame_start_save_salt(); /* match retail's save-write RNG advance */
            enter_prologue();          /* Start Game → the gem cutscene */
        }
        else if (st == NEWGAME_BACK)
            leave_newgame_to_title("backed out (result 0xb)");
    } else if (g_drive_active) {
        /* Mirror retail's tight outer loop (FUN_0056aea0's do/while): spin the
         * pace machine — its update steps are ~free, no per-step sleep — until
         * it PRESENTS one frame, then fall through to frame_limiter, which
         * gates the presented-frame rate.  This is the R3 fix: the fixed-
         * timestep accumulator (title_pace_step) is designed to be called in a
         * tight loop with only the present blocking on wall-clock.  Driving it
         * one pace-step per 16 ms-throttled iteration instead made `now` run
         * away (the budget refill `b += now - anchor` grew unbounded), so the
         * port did ~6 updates per render and DROPPED ~5/6 of the intro's fade
         * frames (rendered 90 of ~528 update ticks).  Spinning to one present
         * per frame_limiter tick restores ~1 update per render, so every fade
         * value is rendered — matching retail's distinct-content sequence
         * (retail renders the same values, just ~2× duplicated by its higher
         * display refresh).  See docs/findings/title-scene.md "Intro pacing". */
        unsigned guard = 0;
        for (;;) {
            uint32_t now = GetTickCount();
            /* Inject any due replay events into the drive's input ring before
             * the scene polls it (keyed on the present/Flip count, stable
             * across the spin; input_trace_replay's cursor never re-injects). */
            feed_input(&g_drive.input, now);
            unsigned pf_before = g_present_frame;
            title_scene_status st = title_drive_step(&g_drive, now);
            /* R3 intro-pace instrumentation: log each phase transition with the
             * Flip count + wall-clock elapsed.  Port-side counterpart of the
             * Frida --pace-probe (retail: ~9.2 s / ~1172 flips to the menu). */
            {
                static int s_last_phase = -1;
                int ph = (int)g_drive.scene.fade.phase;
                if (ph != s_last_phase) {
                    log_line("pace: phase %d -> %d @ flip=%u t=%ums",
                             s_last_phase, ph, g_present_frame,
                             (unsigned)(timeGetTime() - g_base_time_ms));
                    s_last_phase = ph;
                }
            }
            if (st == TITLE_SCENE_DONE) {
                int32_t result = g_drive.result;
                log_line("title scene returned result=%ld", (long)result);
                /* Dispatch the committed menu-action code like retail's
                 * FUN_00562ea0 outer loop (562ea0.c:684-734): Exit leaves the
                 * loop; every other case eventually re-runs the title.  The
                 * sub-scene runners (new-game / continue / options / bonus)
                 * are not ported yet (gated on the glyph/text pipeline + font
                 * registration), so their arms log + re-display the title — so
                 * the menu loops the way retail's does instead of exiting. */
                switch (app_flow_dispatch(result)) {
                case APP_FLOW_EXIT:
                case APP_FLOW_EXIT_9:
                    g_shutdown = 1;
                    break;
                case APP_FLOW_NEW_GAME:   /* 0x564160 case 0x24 — config scene now driven */
                    log_line("dispatch: NEW_GAME (result=%ld) → new-game "
                             "config scene", (long)result);
                    enter_newgame();
                    break;
                case APP_FLOW_CONTINUE:   /* 0x56a670                   — UNPORTED */
                case APP_FLOW_OPTIONS:    /* 0x40a5d0+0x568de0          — UNPORTED */
                case APP_FLOW_BONUS:      /* 0x583fe0                   — UNPORTED */
                case APP_FLOW_DEMO_START: /* 0x59ec30(0x2724,0,0)       — UNPORTED */
                    log_line("dispatch: scene for result=%ld not yet ported "
                             "(stub) — re-displaying title", (long)result);
                    reenter_title();
                    break;
                case APP_FLOW_REENTER_TITLE:
                default:
                    reenter_title();
                    break;
                }
                /* Leave the present-spin; the outer WinMain loop resumes the
                 * (possibly rebuilt) drive on the next main_loop_body, or ends
                 * if g_shutdown was set. */
                break;
            }
            /* drive_present bumps g_present_frame on a render step (the sink's
             * TITLE_DRAW_FLIP).  A change ⇒ this iteration presented a frame. */
            if (g_present_frame != pf_before)
                break;
            if (++guard > 10000)   /* safety: never spin unbounded on a stall */
                break;
        }
    } else if (!g_skip_ddraw && !g_no_present && g_zdd != NULL) {
        /* Legacy minimal present (--no-title-scene / failed drive init).  The
         * offscreen surface is whatever DDraw initialised it to, so the
         * windowed BitBlt produces a black/uninitialised rectangle; success is
         * "zero DDERR per frame". */
        zdd_present(g_zdd);
    }

    frame_limiter();
    call_trace_end_frame();
    g_frame_counter++;
}

/* 16 ms wall-time gate (placeholder until we resolve the engine's actual
 * frame cadence — fortune-summoners-era LizSoft games typically target
 * 60 FPS).  Same shape as OpenMare's frame_limiter, downshifted from
 * its 20 ms gate. */
static void frame_limiter(void)
{
    /* Harness knob: --no-frame-limit (g_no_frame_limit) skips the 60 FPS
     * wall-clock gate so long deterministic IN-GAME replays (--input-trace /
     * --osr-emit captures that must march thousands of frames to the
     * house/errands rooms) run as fast as the CPU allows.  GATED on g_game_active:
     * the in-game sim is frame-counter deterministic (the cutscene typewriter +
     * camera ride g_game_camera_hold parity, the nav is frame-keyed, the port
     * flips once per iteration), so uncapping there changes only the wall-clock
     * rate.  The title/menu/prologue phases are NOT uncapped — their intro pacing
     * is flip/wall-clock sensitive (uncapping desyncs the frame-keyed title nav:
     * newgame_enter slid 750 -> 5403 flips).  g_total_ms still tracks the real
     * clock for any wall-clock reader. */
    if (g_no_frame_limit && g_game_active) {
        g_total_ms = timeGetTime() - g_base_time_ms;
        return;
    }

    uint32_t now = timeGetTime();
    uint32_t elapsed = now - g_base_time_ms;
    uint32_t target  = (g_frame_counter + 1) * FRAME_PERIOD_MS;
    if (elapsed < target) {
        Sleep(target - elapsed);
        now = timeGetTime();
        elapsed = now - g_base_time_ms;
    }
    g_total_ms = elapsed;
}

static void parse_cmdline(LPSTR lpCmdLine)
{
    /* Tiny ad-hoc tokenizer.  We only need to handle flags without
     * embedded quotes; the launcher controls argv shape. */
    char buf[2048];
    strncpy(buf, lpCmdLine ? lpCmdLine : "", sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;

    char *tok = strtok(buf, " \t");
    while (tok) {
        if      (!strcmp(tok, "--hide-window"))  g_hide_window = 1;
        else if (!strcmp(tok, "--no-frame-limit")) g_no_frame_limit = 1;
        else if (!strcmp(tok, "--show-msgbox"))  g_show_msgbox = 1;
        else if (!strcmp(tok, "--allow-multi"))  g_allow_multi = 1;
        else if (!strcmp(tok, "--no-cd"))        g_skip_cd = 1;
        else if (!strcmp(tok, "--skip-ddraw"))   g_skip_ddraw = 1;
        else if (!strcmp(tok, "--no-present"))   g_no_present = 1;
        else if (!strcmp(tok, "--no-title-scene")) g_no_title_scene = 1;
        else if (!strcmp(tok, "--menu-trace"))   title_sink_menu_trace = 1;
        else if (!strcmp(tok, "--input-trace")) {
            tok = strtok(NULL, " \t");
            if (tok) {
                strncpy(g_input_trace_path_buf, tok,
                        sizeof(g_input_trace_path_buf) - 1);
                g_input_trace_path = g_input_trace_path_buf;
            }
        }
        else if (!strncmp(tok, "--input-trace=", 14)) {
            strncpy(g_input_trace_path_buf, tok + 14,
                    sizeof(g_input_trace_path_buf) - 1);
            g_input_trace_path = g_input_trace_path_buf;
        }
        else if (!strcmp(tok, "--held-trace")) {
            tok = strtok(NULL, " \t");
            if (tok) {
                strncpy(g_held_trace_path_buf, tok,
                        sizeof(g_held_trace_path_buf) - 1);
                g_held_trace_path = g_held_trace_path_buf;
            }
        }
        else if (!strncmp(tok, "--held-trace=", 13)) {
            strncpy(g_held_trace_path_buf, tok + 13,
                    sizeof(g_held_trace_path_buf) - 1);
            g_held_trace_path = g_held_trace_path_buf;
        }
        else if (!strcmp(tok, "--frames")) {
            tok = strtok(NULL, " \t");
            if (tok) g_max_frames = (unsigned)strtoul(tok, NULL, 10);
        }
        else if (!strncmp(tok, "--frames=", 9)) {
            g_max_frames = (unsigned)strtoul(tok + 9, NULL, 10);
        }
        else if (!strcmp(tok, "--launcher-mode")) {
            tok = strtok(NULL, " \t");
            if (tok) g_launcher_mode = (int)strtol(tok, NULL, 10);
        }
        else if (!strncmp(tok, "--launcher-mode=", 16)) {
            g_launcher_mode = (int)strtol(tok + 16, NULL, 10);
        }
        else if (!strcmp(tok, "--call-trace")) {
            tok = strtok(NULL, " \t");
            if (tok) {
                strncpy(g_call_trace_path_buf, tok,
                        sizeof(g_call_trace_path_buf) - 1);
                g_call_trace_path = g_call_trace_path_buf;
            }
        }
        else if (!strncmp(tok, "--call-trace=", 13)) {
            strncpy(g_call_trace_path_buf, tok + 13,
                    sizeof(g_call_trace_path_buf) - 1);
            g_call_trace_path = g_call_trace_path_buf;
        }
        else if (!strcmp(tok, "--call-trace-frames") ||
                 !strncmp(tok, "--call-trace-frames=", 20)) {
            /* Comma-separated frame whitelist, e.g. 1,2,3.  Either
             * `--call-trace-frames 1,2,3` (next token) or the `=` form.
             * Parsed with strtoul's endptr rather than a nested strtok,
             * which would clobber the outer tokenizer's saved state. */
            const char *list = NULL;
            if (!strncmp(tok, "--call-trace-frames=", 20)) {
                list = tok + 20;
            } else {
                list = strtok(NULL, " \t");
            }
            while (list && *list && g_n_call_trace_frames < CALL_TRACE_FRAMES_CAP) {
                char *end = NULL;
                unsigned v = (unsigned)strtoul(list, &end, 10);
                if (end == list) break;          /* no digits — give up */
                g_call_trace_frames[g_n_call_trace_frames++] = v;
                list = end;
                while (*list == ',' || *list == ' ') list++;
            }
        }
        else if (!strcmp(tok, "--capture-frames") ||
                 !strncmp(tok, "--capture-frames=", 17)) {
            const char *list = NULL;
            if (!strncmp(tok, "--capture-frames=", 17)) list = tok + 17;
            else                                       list = strtok(NULL, " \t");
            while (list && *list && g_n_capture_frames < CAPTURE_FRAMES_CAP) {
                char *end = NULL;
                unsigned v = (unsigned)strtoul(list, &end, 10);
                if (end == list) break;
                g_capture_frames[g_n_capture_frames++] = v;
                list = end;
                while (*list == ',' || *list == ' ') list++;
            }
        }
        else if (!strcmp(tok, "--capture-all")) {
            g_capture_all = 1;
        }
        else if (!strcmp(tok, "--capture-all-from")) {
            tok = strtok(NULL, " \t");
            if (tok) { g_capture_all = 1; g_capture_all_from = (unsigned)strtoul(tok, NULL, 10); }
        }
        else if (!strcmp(tok, "--capture-dir")) {
            tok = strtok(NULL, " \t");
            if (tok) {
                strncpy(g_capture_dir_buf, tok, sizeof(g_capture_dir_buf) - 1);
                g_capture_dir_buf[sizeof(g_capture_dir_buf) - 1] = '\0';
            }
        }
        else if (!strncmp(tok, "--capture-dir=", 14)) {
            strncpy(g_capture_dir_buf, tok + 14, sizeof(g_capture_dir_buf) - 1);
            g_capture_dir_buf[sizeof(g_capture_dir_buf) - 1] = '\0';
        }
        else if (!strcmp(tok, "--render-glyph-test")) g_render_glyph_test = 1;
        else if (!strcmp(tok, "--glyph-test-font")) {
            tok = strtok(NULL, " \t");
            if (tok) g_glyph_test_font = (int)strtol(tok, NULL, 0);
        }
        else if (!strcmp(tok, "--osr-replay")) {
            tok = strtok(NULL, " \t");
            if (tok) {
                strncpy(g_osr_replay_path_buf, tok,
                        sizeof(g_osr_replay_path_buf) - 1);
                g_osr_replay_path = g_osr_replay_path_buf;
            }
        }
        else if (!strncmp(tok, "--osr-replay=", 13)) {
            strncpy(g_osr_replay_path_buf, tok + 13,
                    sizeof(g_osr_replay_path_buf) - 1);
            g_osr_replay_path = g_osr_replay_path_buf;
        }
        else if (!strcmp(tok, "--osr-out")) {
            tok = strtok(NULL, " \t");
            if (tok) {
                strncpy(g_osr_out_dir_buf, tok, sizeof(g_osr_out_dir_buf) - 1);
                g_osr_out_dir_buf[sizeof(g_osr_out_dir_buf) - 1] = '\0';
            }
        }
        else if (!strncmp(tok, "--osr-out=", 10)) {
            strncpy(g_osr_out_dir_buf, tok + 10, sizeof(g_osr_out_dir_buf) - 1);
            g_osr_out_dir_buf[sizeof(g_osr_out_dir_buf) - 1] = '\0';
        }
        else if (!strcmp(tok, "--osr-emit")) {
            tok = strtok(NULL, " \t");
            if (tok) {
                strncpy(g_osr_emit_path_buf, tok,
                        sizeof(g_osr_emit_path_buf) - 1);
                g_osr_emit_path = g_osr_emit_path_buf;
            }
        }
        else if (!strncmp(tok, "--osr-emit=", 11)) {
            strncpy(g_osr_emit_path_buf, tok + 11,
                    sizeof(g_osr_emit_path_buf) - 1);
            g_osr_emit_path = g_osr_emit_path_buf;
        }
        else if (!strcmp(tok, "--osr-scenario")) {
            tok = strtok(NULL, " \t");
            if (tok) {
                strncpy(g_osr_scenario_buf, tok, sizeof(g_osr_scenario_buf) - 1);
                g_osr_scenario_buf[sizeof(g_osr_scenario_buf) - 1] = '\0';
            }
        }
        else if (!strncmp(tok, "--osr-scenario=", 15)) {
            strncpy(g_osr_scenario_buf, tok + 15, sizeof(g_osr_scenario_buf) - 1);
            g_osr_scenario_buf[sizeof(g_osr_scenario_buf) - 1] = '\0';
        }
        else if (!strcmp(tok, "--osr-state")) {   /* opt-in OSR_STATE (rng census) */
            g_osr_state_on = 1;
        }
        else if (!strcmp(tok, "--osr-replay-frames") ||
                 !strncmp(tok, "--osr-replay-frames=", 20)) {
            const char *list = NULL;
            if (!strncmp(tok, "--osr-replay-frames=", 20)) list = tok + 20;
            else                                           list = strtok(NULL, " \t");
            while (list && *list && g_n_osr_replay_frames < OSR_REPLAY_FRAMES_CAP) {
                char *end = NULL;
                unsigned v = (unsigned)strtoul(list, &end, 10);
                if (end == list) break;
                g_osr_replay_frames[g_n_osr_replay_frames++] = v;
                list = end;
                while (*list == ',' || *list == ' ') list++;
            }
        }
        tok = strtok(NULL, " \t");
    }
}

static void log_line(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[opensummoners] ");
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    fflush(stderr);
    va_end(ap);
}

/* TAS anchor — a named alignment point emitted at a scene/phase boundary.
 * Under the lockstep clock the port and retail run 1 update/present and march
 * tick-for-tick WITHIN a scene, but the flip count at which a boundary is
 * reached differs across the two binaries (boot skew, per-scene load cost).
 * An anchor records (name, flip, RNG state) so the side-by-side trace diff
 * (tools/tas_diff.py) can offset retail's flip axis onto the port's at each
 * boundary, and so a mismatched RNG state pinpoints an unaccounted rand()
 * consumer.  The retail counterpart is the Frida agent's {kind:'anchor'} send.
 * Format is fixed + grep-friendly: `anchor: <name> flip=<N> rng=0x<hex>`. */
static void emit_anchor(const char *name)
{
    log_line("anchor: %s flip=%u rng=0x%08lx",
             name, g_present_frame, (unsigned long)rng_peek_state());
    osr_emit_anchor(name, g_present_frame, g_sim_tick_count, rng_peek_state());
}
