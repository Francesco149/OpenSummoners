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
#include "color_grade.h"      /* the in-game palette color-grade LUT (0x417c40) */
#include "camera_follow.h"    /* the in-game camera easer (0x43d1d0) + setters (0x439690) */
#include "letterbox.h"        /* the establishing-shot cinematic letterbox (0x48c150 slice) */
#include "actor_spawn.h"      /* the town CHARACTER-band spawn (0x58d460 -> 0x431e30) */
#include "actor_render.h"     /* actor_render_static (the 0x491ae0 default arm) */
#include "particle.h"         /* the fountain spray (0x13e0 band / 0x46e510 / 0x493480) */

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

/* The particle band (0x13e0 DEVICE pool / 0x493480 render) — the FOUNTAIN SPRAY.
 * The fountain prop 0x112e5 (a CHARACTER in g_actors) emits one 0x18708 water
 * droplet per sim-tick; particle_pool_step applies gravity/fade.  engine-quirk
 * #87; findings "The FOUNTAIN SPRAY".  g_fountain_cx/cy is the emitter's
 * anchor-center; g_fountain_counter is the +0x5c %3 velocity-cycle state. */
static particle_pool    g_fountain_pp;
static int              g_fountain_loaded;
static int32_t          g_fountain_cx, g_fountain_cy;
static int              g_fountain_counter;
/* PORT-DEBT(fountain-anchor): the 0x557370 mode-1 anchor is parent
 * render-state +0xc/2.  The trace (0x18708 fresh particles) pins the fountain's
 * true value at +1405 (render-state +0xc ~= 2810); it is NOT the prop's display-
 * cel width (that measures +1700), so +0xc is a distinct field whose source is
 * still un-RE'd.  We keep the calibrated +1245 (USER-confirmed) until +0xc's
 * setter is identified.  (The SKY emitter needs none — see below.) */
#define FOUNTAIN_EMIT_X_OFF 1245

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

/* The establishing-shot cinematic letterbox bar heights (0x48c150's
 * in_ECX+0x44 / +0x48).  Set to LETTERBOX_INTRO_BAR (64) for the opening-town
 * intro = the quirk-#74 letterbox; 0 = no bars.
 * PORT-DEBT(ingame-letterbox): retail's heights are written by the unported
 * 0x5a00c0 cutscene script onto the scene-controller object; the port drives a
 * constant during the establishing shot (parallel to the camera-pan trigger
 * stand-in).  The grid-fill geometry itself is bit-exact RE'd (letterbox.c). */
static int            g_letterbox_top;
static int            g_letterbox_bottom;

/* Flips the camera holds at the spawn origin before the scripted pan begins.
 * MEASURED from a retail field-spec trace (--seed-pin --lockstep): Flip 1616 is
 * still HOLD (tgt=128000/cap=0), Flip 1617 = PAN (tgt=12800/cap=300), and
 * game_enter@1433 → 1617-1433 = 184.  Even (so the trigger Flip is also a sim
 * tick — see game_camera_step).  Synthetic stand-in for the cutscene-script
 * trigger source; see PORT-DEBT(ingame-camera-pan) above. */
#define GAME_CAMERA_HOLD_FRAMES 184u

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
    resolve_launch_path(g_call_trace_path_buf, sizeof g_call_trace_path_buf,
                        "--call-trace");

    /* Open the port-side call trace (no-op unless --call-trace given).
     * Done before any traced function (boot DDraw path) can run. */
    call_trace_init_from_cli(g_call_trace_path,
                             g_call_trace_frames, g_n_call_trace_frames);

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
    shutdown_ddraw();
    if (g_sotesd != NULL) {
        FreeLibrary(g_sotesd);
        g_sotesd = NULL;
    }
    call_trace_shutdown();
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
     * color-keying is by index, so grading the colors is key-safe. */
    if (g_color_grade_on && src == 8)
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
        snprintf(path, sizeof path, "%s/port_frame_%05u.bmp",
                 g_capture_dir, flip_frame);
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
        rng_srand(seed);
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
    if (g_effects_loaded)
        for (int i = 0; i < g_effects.count; i++)
            effect_emitted += actor_render_static(&g_effects.actors[i],
                                                  &g_effects.states[i],
                                                  /*flip_table=*/g_actor_flip_table, pool,
                                                  game_sprite_resolve, NULL);

    if (g_actors_loaded)
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
    int particle_emitted = particle_pool_render(&g_fountain_pp, pool,
                                                game_sprite_resolve, NULL);

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

/* The letterbox cel is main sprite-pool slot 41 (PE resource 0x583, 64x4,
 * opaque) — registered by ar_register_main_sprites (extras[] idx 41).  The
 * engine binds it via FUN_00418470(0) (the plain frame getter, NO 0x417c40
 * grade) before the FUN_005b9a40 tile blits, so the port resolves the fixed
 * slot directly (frame 0) and blits it whole — same primitive the parallax
 * far-plane uses, minus the 24bpp grade stamp. */
#define LETTERBOX_BANK_SLOT 41

/* letterbox_blit_fn — draw one letterbox cel at screen (x,y) (FUN_005b9a40). */
static void game_letterbox_blit(void *ctx, int x, int y)
{
    (void)ctx;
    if (g_zdd == NULL || g_zdd->primary_obj == NULL) return;
    void *cel = ar_sprite_slot_frame(&g_ar_sprite_slots[LETTERBOX_BANK_SLOT], 0);
    if (cel == NULL) return;
    zdd_object_blt_onto((zdd_object *)cel, g_zdd->primary_obj, x, y);
}

/* Load the room's map-data DATA resource from the original sotes.exe and build
 * the town backdrop scene.  Mirrors retail FUN_00587970: FindResourceA(EXE,
 * scene&0xffff, "DATA") + LoadResource + LockResource, then the parse + decode
 * (here town_render_load).  The opening town (map 0x3f2 → room 210110) is scene
 * 1022.  Returns 1 on success.  On any failure the scene stays unloaded and
 * game_render shows the faithful black map-load frame. */
static int load_town_scene(uint16_t scene)
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

    if (town_render_load(&g_town, bytes, (size_t)len, game_bank_dims, NULL) != 0) {
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
    CALL_TRACE_I32("advanced", advanced); /* port-side: actors stepped this tick */
    CALL_TRACE_END();

    /* The PARTICLE band.  The FOUNTAIN emitter (0x112e5 / 0x54f980:218) spawns one
     * 0x18708 water droplet this sim-tick; the SKY emitters (0x112e2 /
     * 0x54f980:150) each spawn one 0x18704 ambient particle every 6th tick.  Both
     * draw the shared LCG for their jitter/velocity.  particle_pool_step then steps
     * every active particle (gravity/integrate/fade/expire, dispatched by code).
     * RNG-driven, so frame-exact alignment with retail is Phase 2
     * (PORT-DEBT(fountain-rng-phase)); the physics here are faithful. */
    if (g_fountain_loaded)
        particle_fountain_emit(&g_fountain_pp, g_fountain_cx, g_fountain_cy,
                               &g_fountain_counter);
    for (int i = 0; i < g_sky_emit_count; i++)
        particle_sky_emit(&g_fountain_pp, g_sky_cx[i], g_sky_cy[i],
                          &g_sky_counter[i]);
    particle_pool_step(&g_fountain_pp);
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
            if (g_actors_loaded && (g_game_camera_hold & 1u) == 0u)
                game_actor_update();
            game_camera_step();
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
                            game_actor_walk, NULL, &deferred);
        /* 0x48c150:124-162 — the cinematic letterbox tiled ON TOP of the
         * backdrop (after the present pass), during the establishing shot. */
        letterbox_render(g_letterbox_top, g_letterbox_bottom,
                         game_letterbox_blit, NULL);
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
    rng_srand(game_rng_seed());

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
    load_town_scene(/*scene=*/1022);

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
        int en = actor_spawn_effect_from_map(&g_effects, &g_town.map);
        g_effects_loaded = (en > 0);
        /* Fill the mirror/flip table so the facing==3 townsfolk pick the mirrored
         * cel (frame_base + flip).  Faithful to retail's DAT_008a8440 global. */
        int fn = actor_spawn_effect_fill_flip_table(g_actor_flip_table,
                                                    AR_SPRITE_SLOT_COUNT);
        log_line("enter_game: actor_spawn_effect_from_map -> %d EFFECT townsfolk "
                 "(standing villagers, DATA 1022; %d flip-table banks; 0xe29a "
                 "wanderers deferred)", en, fn);

        /* Arm the PARTICLE band (Chip 3+).  Two emitters, both CHARACTER props
         * already in g_actors, both feeding the shared +0x13e0 pool (g_fountain_pp):
         *   - the FOUNTAIN 0x112e5 (bank 0x16c frame 36) emits 0x18708 water every
         *     primary sim-tick (engine-quirk #87, "The FOUNTAIN SPRAY").
         *   - the SKY emitters 0x112e2 emit 0x18704 ambient particles every 6th
         *     sim-tick ("The SKY-AMBIENT particles").
         * Find each, cache its anchor-center, reset its counter. */
        particle_pool_reset(&g_fountain_pp);
        g_fountain_loaded = 0;
        g_fountain_counter = 0;
        g_sky_emit_count = 0;
        for (int i = 0; i < g_actors.count; i++) {
            uint32_t code = g_actors.actors[i].code;
            if (code == 0x112e5u && !g_fountain_loaded) {
                /* fountain prop: anchor +0xc/2 (PORT-DEBT, calibrated +1245). */
                g_fountain_cx = g_actors.states[i].world_x + FOUNTAIN_EMIT_X_OFF;
                g_fountain_cy = g_actors.states[i].world_y;
                g_fountain_loaded = 1;
            } else if (code == 0x112e2u && g_sky_emit_count < SKY_EMIT_MAX) {
                /* invisible trigger: +0xc==0 -> anchor 0 -> the prop's world pos. */
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
        if (g_input_trace_active)
            input_trace_replay(&g_input_trace, g_present_frame,
                               &g_prologue_drive.input, now);
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
        if (g_input_trace_active)
            input_trace_replay(&g_input_trace, g_present_frame,
                               &g_game_drive.input, now);
        (void)game_drive_step(&g_game_drive, now);   /* GAME_RUNNING until ported */
    } else if (g_newgame_active) {
        /* The new-game config scene: one newgame_drive_step per presented frame
         * (no pace machine — frame_limiter gates the rate).  Inject any due
         * replay events into the drive's ring before it polls (keyed on the
         * Flip count drive_present bumps), then act on the outcome. */
        uint32_t now = GetTickCount();
        if (g_input_trace_active)
            input_trace_replay(&g_input_trace, g_present_frame,
                               &g_newgame_drive.input, now);
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
            if (g_input_trace_active)
                input_trace_replay(&g_input_trace, g_present_frame,
                                   &g_drive.input, now);
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
}
