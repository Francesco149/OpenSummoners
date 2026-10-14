/* rhost — in-process retail host for Fortune Summoners (sotes.exe).
 * Runs the user's own Steamless-unpacked sotes.exe inside this process, deterministically:
 * flip-locked virtual clock, fixed-address heap arena, pinned seed, scripted input through a
 * fake DirectInput7 keyboard, virtual BGM/SE playback, hermetic file overlay.
 * Modeled on openrecet's tools/rhost (2026-10-13 methodology reset). See README.md. */
#pragma once
#include "retail_map.h"
#include <stdarg.h>

#define G32(va) (*(volatile uint32_t *)(uintptr_t)(va))
#define G16(va) (*(volatile uint16_t *)(uintptr_t)(va))
#define G8(va)  (*(volatile uint8_t *)(uintptr_t)(va))

/* retail image layout (sotes.unpacked.exe, ImageBase 0x400000, relocs stripped).
 * Set from the PE headers at map time (rhost.c), so every edition works. */
extern uint32_t RT_TEXT_LO, RT_TEXT_SZ, RT_DATA_LO, RT_DATA_HI;
#define RT_IMG_LO 0x400000u
extern uint32_t RT_IMG_HI;

/* ---- per-edition engine VAs (rhost.c: g_ed) ----
 * Every VA is the edition's own; SE values come from the IDA crosswalk
 * (tools/ida/crosswalk.py) and are only filled once verified. 0 = unknown/unsupported. */
typedef struct {
    const char *tag;            /* "enold", "ense-en", ... */
    uint32_t image_size;        /* SizeOfImage — the identity check */
    uint32_t entry;             /* AddressOfEntryPoint VA */
    uint32_t flip;              /* zdd present (__thiscall this=zdd, HWND) — frame boundary */
    uint32_t easer;             /* scene easer = the port's sim tick (g_sim_tick_count) */
    uint32_t srand;             /* srand(time(0)) in WinMain — pinned to --seed */
    uint32_t rand;              /* MSVC rand (LCG) */
    uint32_t rng_seed;          /* DAT_ LCG state word */
    uint32_t state_ptr;         /* DAT_ game-state struct pointer (pump: +0x1c wait flag) */
    uint32_t active;            /* DAT_ "app active" (WM_ACTIVATEAPP wParam) */
    uint32_t dlgproc;           /* launcher DLGPROC (template 0x2711) */
    uint32_t launch_flag;       /* DAT_ set 1 by the launcher's Launch button */
    uint32_t cfg_screen;        /* DAT_ LOWORD screen mode (3 windowed) | HIWORD VRAM */
    uint32_t cfg_sound;         /* DAT_ LOWORD sound | HIWORD checkbox 10024 */
    uint32_t zdd;               /* DAT_ zdd object pointer (backbuffer for shots) */
    uint32_t wndproc;           /* CLASS_LIZSOFT_SOTES WndProc (WM_ACTIVATEAPP target) */
    uint32_t hwnd_slot;         /* DAT_ game HWND (always masked: a kernel handle value) */
    uint32_t pump_wait_off;     /* state_ptr + this = the pump's WM_TIMER wait flag (EN-old 0x1c, SE 0x20) */
} rh_edition_t;
extern const rh_edition_t *g_ed;

typedef struct {
    const char *exe, *gamedir, *rundir, *edition;
    uint32_t frames;            /* stop after this many flips (0 = trace end + tail) */
    uint32_t seed; int has_seed;
    const char *trace;          /* input trace (.jsonl: {"frame":N,"keys":[sc..]} levels) */
    const char *record;         /* realtime human play: write a flat trace */
    uint32_t dump_at[16]; int n_dump;
    uint32_t shot_at[64]; int n_shot; uint32_t shot_every;
    uint32_t hash_every;        /* log state hash every N frames (0 = only at end) */
    uint32_t log_every;         /* status line every N frames */
    uint32_t tail;              /* frames after the last trace entry */
    int realtime;               /* pace flips at 60 Hz wall clock (human play) */
    int quiet;                  /* window off-screen, never activated, no taskbar button */
    int audible;                /* real BGM/SE output (default: only in --record) */
    int silent;                 /* force audible off (also in --record) */
    int no_arena, no_overlay, no_uninit_fix, perturb_dlls;
    int log_files, log_heap, log_ext, log_media, log_input;
    const char *coverage;
    uint32_t stall_ms;
    const char *hooks;          /* hook spec, see rh_hooks.c */
    uint32_t watch[16]; int n_watch;   /* dwords appended to each status line */
    uint32_t step_ms;           /* virtual ms per flip (16 = the engines' 0x10 update slice) */
    uint32_t creep_after;       /* GetTickCount calls without a flip before the clock creeps */
    uint32_t stall_wall_ms;     /* watchdog: no Flip for this long (wall) → RESULT status=stalled */
    uint32_t trap_max;          /* --trap: log the first N hits (default 50) */
    uint32_t scrub_kb;          /* zero this much dead stack after each external call (0 = off) */
} rh_opts_t;

extern rh_opts_t O;
extern volatile uint32_t g_frame;      /* flips completed */
extern volatile uint32_t g_ticks;      /* easer (sim tick) calls */
extern FILE *g_log;
void rh_log(const char *fmt, ...);
void rh_fatal(const char *fmt, ...);
void rh_finish(const char *why);   /* end the run now: hash, dumps, RESULT, ExitProcess */
uint32_t rh_vnow(void);            /* virtual GetTickCount (ms) */
void iat_override(const char *name, void *fn);
int  iat_try(const char *name, void *fn);
void *iat_lookup(const char *name);
void *iat_override_ord(const char *dll, int ord, void *fn);
int  redirect_calls(uint32_t va, void *hook);
int  redirect_calls_chain(uint32_t va, void *hook);   /* + tails of rhost's own stubs (port hooks) */
void *rh_fixed_alloc(size_t n);   /* host objects retail may point at: fixed VA (0x5f080000..) */

/* rh_heap.c */
int  rh_heap_init(void);
void rh_heap_install(void);
extern int rh_heap_zero_alloc;   /* uninit-fix site D: zero every arena block handed out */
int  rh_heap_ranges(uint32_t (*out)[2], int max);
void rh_heap_stats(char *buf, size_t n);
int  rh_is_det_addr(uint32_t v);   /* image / host / arena: deterministic address space */
void *rh_heap_meta_save(void);
void rh_heap_meta_restore(void *m);
void rh_heap_meta_free(void *m);

/* rh_files.c */
void rh_files_install(void);

/* rh_gdi.c — fake GDI handle numbering */
void rh_gdi_install(void);
long rh_gdi_live(void);
void *rh_gdi_real(void *h);   /* fake → real (pass-through for others) */

/* rh_handles.c */
void rh_handles_install(void);
long rh_handles_live(void);

/* rh_state.c */
typedef struct { uint64_t full, semantic; uint32_t masked, ranges, bytes; } rh_hash_t;
void rh_state_hash(rh_hash_t *h);
int  rh_state_dump(const char *path);
void rh_state_volatile_add(uint32_t va);
void rh_state_ext_report(void);
int  rh_shadow(uint32_t va, const char *name, void (*port)(void), char *why, size_t whyn);
/* closure form: retail(ctx), then port(ctx) on the identical pre-state (retail .data + heap arena +
 * allocator metadata + virtual clock + the dead stack below SP); every state dword compared, retail's
 * post-state kept. 0 same, 1 differ (`why` = count + the first differing dwords), -1 not compared. */
int  rh_shadow_ex(const char *name, void (*retail)(void *), void (*port)(void *), void *ctx, char *why, size_t whyn);
void rh_clock_save(uint32_t s[3]);
int  rh_in_shadow(void);                   /* inside an rh_shadow_ex call (nested hooks pass through) */
/* the PORT phase of a shadow must not repeat an external effect retail already performed (free a
 * DirectDraw surface, release a DirectSound buffer, DeleteObject, re-poll DirectInput): the guarded
 * engine-call stubs ask rh_shadow_port_phase(), skip the call and rh_shadow_abort() — the shadow then
 * keeps retail's result and reports the call as skipped (not compared) */
int  rh_shadow_port_phase(void);
void rh_shadow_abort(const char *what);
void *rh_media_save(void);                 /* the fake DirectSound pool (rh_media.c) */
void rh_media_restore(const void *m);
long rh_media_diff(const void *m);
void rh_media_snap_free(void *m);
void rh_clock_restore(const uint32_t s[3]);

/* rh_trace.c — input script / recorder (DIK scancode levels per flip) */
void rh_trace_init(void);
const uint8_t *rh_trace_keys(uint32_t frame);   /* 256-byte DIK state for this frame */
uint32_t rh_trace_last_frame(void);
void rh_trace_record(uint32_t frame, const uint8_t keys[256]);
void rh_trace_finish(void);
const char *rh_trace_status(void);

/* rh_dinput.c — fake DirectInput7 (keyboard fed by rh_trace; no joysticks) */
void rh_dinput_install(void);
void rh_dinput_frame(uint32_t frame);   /* called at each flip: latch the next frame's keys */

/* rh_media.c — DirectShow BGM on a virtual clock + fake/real DirectSound */
void rh_media_install(void);
void rh_media_report(void);

/* rh_scrub.c — zero system-DLL stack garbage after every external call */
void rh_scrub_install(void);
void rh_scrub_iat(void);
long rh_scrub_thunks(void);
void rh_scrub_wrap_vtbl(void *obj, int nslots, const char *what);   /* COM: every slot via a scrub thunk */
void rh_scrub_here(void);           /* zero the dead stack below the caller */
uint64_t rh_scrub_pages(void);

/* rh_shot.c */
void rh_shot(const char *tag);

/* rh_hooks.c */
void rh_hooks_install(void);
void rh_hooks_report(void);
