// tools/frida/opensummoners-agent.js
//
// Frida agent for the harness against the retail sotes.exe.
//
// Same shape and conventions as the OpenMare / openrecet siblings: one
// module-level `init({...})` RPC sets every behavior, and the agent emits
// well-known `kind` events over send() that the Python driver consumes.
//
// Phase A — boot-level hooks (no engine-specific addresses yet):
//   * MessageBoxA/W            → redirect to stderr-equivalent send() and
//                                auto-IDOK so modal popups never block
//   * ShowWindow + SetWindowPos→ optionally rewrite to SW_HIDE
//   * GetMessage/PeekMessage   → tick counter (proxy for "engine ran a frame")
//   * Sleep (kernel32 + winmm) → no-op when turbo enabled (engine-loop speedup)
//   * timeGetTime              → virtualised clock when turbo enabled
//                                (the *simulation* clock, not just the loop)
//   * DirectSoundCreate/8 etc. → silent-audio plumb-through hook (volume clamp)
//
// As Ghidra resolves the engine's clock, frame-end, audio init, etc., we
// extend ADDR + install hooks behind init({...}) flags.
//
// Address convention: vendor/unpacked/sotes.unpacked.exe is a clean 32-bit
// PE, preferred ImageBase 0x00400000.  Windows loads it at its preferred
// base on modern systems (no /DYNAMICBASE in this era's MSVC builds), but
// we recompute the runtime base on attach and translate via `rva()` for
// safety.
//
// Message protocol (stringified JSON via send()):
//   {kind:"ready",        module, base, fridaVersion}
//   {kind:"log",          msg}
//   {kind:"error",        where, msg}
//   {kind:"messagebox",   api, caption, body, type}        // redirected popup
//   {kind:"hwnd_owned",   hwnd, source}                    // CreateWindowEx return
//   {kind:"hide_force",   api, hwnd, original}             // a show-path call was hidden
//   {kind:"msg",          source:"GetMessage"|"PeekMessage", count}
//   {kind:"turbo_tick",   virtual_ms}                      // periodic, every N frames
//   {kind:"silent_audio", api}                             // an audio-volume hook fired

'use strict';

// ─── module identity ────────────────────────────────────────────────────

const MODULE_NAME = 'sotes.exe';
const IMAGE_BASE  = ptr('0x00400000');

// Address table.  Filled in as Ghidra analysis resolves them.  For Phase A
// we don't need any engine-internal addresses — every hook lives in
// Win32 API land — so this stays empty.  See OpenMare's agent for the
// shape this grows into.
const ADDR = {};

// ─── runtime state ──────────────────────────────────────────────────────

let g_base = null;
let g_start_real_ms = Date.now();

// Window-hide.  When true, every show-path API call (ShowWindow,
// ShowWindowAsync, SetWindowPos w/ SWP_SHOWWINDOW) targeting a
// process-owned HWND gets rewritten to a hide.  "Process-owned" =
// returned from a CreateWindowEx{A,W} we observed — so any future
// system-error popup that takes a different kernel path still surfaces.
let g_hide_window  = false;
let g_owned_hwnds  = {};   // key = hwnd hex string, val = {source}

// Turbo: Sleep → 0 ms (loop runs as fast as it can) + virtualised
// timeGetTime returns a counter we bump per frame_limiter-equivalent
// (here, per PeekMessage iteration), making the engine *simulation*
// itself run faster, not just the loop body iterate at infinite Hz.
let g_turbo_enabled     = false;
let g_turbo_step_ms     = 17;     // ~60 Hz; tweak once we know cadence
let g_virtual_now_ms    = 0;
let g_clock_virtualised = false;
let g_main_thread_id    = -1;

// ─── lockstep clock (TAS determinism, deterministic 1 update / present) ───
// Default turbo advances the virtual GetTickCount by g_turbo_step_ms on
// EVERY main-thread call.  With several GetTickCount calls per presented
// frame, the engine's GetTickCount-delta pace machine (FUN_005b1030 +
// every scene loop's 3-state budget walk) banks several 16 ms update
// slices per Flip, so retail renders only ~1 of every N update ticks.
// The port renders EVERY update (fixed-timestep, 1 update / present), so
// the two frame streams cannot be diffed frame-for-frame: retail's
// presented frames are a 1/N subsample of the update stream.
//
// Lockstep mode makes retail render exactly one update per present, like
// the port: the virtual clock is FROZEN between Flips and advanced by
// exactly one update quantum (g_lockstep_step_ms, default 0x10 = 16 ms)
// in the Flip hook.  So each presented frame banks exactly one 16 ms
// slice → one update → matching the port's cadence.  Combined with the
// pinned RNG seed, the two runs then march tick-for-tick and a captured
// retail flip k aligns to a port flip (after the per-scene anchor offset).
//
// Livelock guard: the engine has GetTickCount busy-waits that need the
// clock to advance to make progress (asset-load settles, the pace
// machine's non-rendering drain iterations).  Two protections:
//   (1) lockstep only ARMS at the first Flip — boot + initial asset load
//       run on the ordinary per-call turbo clock (g_lock_armed latch).
//   (2) STALL-BREAKER creep: the clock is a pure per-Flip freeze during
//       normal scene loops (so the budget banks EXACTLY one 16 ms slice
//       per present → exactly 1 update / present → frame-for-frame match
//       with the port).  Only after a long flipless burst of GetTickCount
//       calls — i.e. a genuine busy-wait that is NOT advancing toward a
//       present (asset-load settle, mode-set spin) — does the clock start
//       creeping by g_lockstep_epsilon_ms per call to let that wait
//       finish.  The creep stops as soon as a Flip resets the counter, so
//       it never pollutes the steady-state update budget.  This keeps the
//       1:1 cadence exact while still being hang-proof across scene loads.
let g_lockstep            = false;
let g_lockstep_step_ms    = 0x10;  // one update quantum banked per Flip
let g_lockstep_epsilon_ms = 1;     // creep per call ONCE a stall is detected
let g_lockstep_stall_calls = 2000; // flipless GetTickCount calls → "stalled"
let g_lock_clock_ms       = 0;     // the frozen virtual clock (lockstep)
let g_lock_armed          = false; // becomes true at the first Flip
let g_lock_calls_since_flip = 0;   // reset each Flip; trips the stall-breaker

// Silent audio.  Three layers we know we need:
//  - DirectSound buffer SetVolume → clamp to -10000 (DSBVOLUME_MIN)
//  - waveOutSetVolume → clamp to 0
//  - waveOutOpen → succeeds but the result is muted by the above
// The DirectSound COM layer is best hooked once we see the engine call
// CoCreateInstance(IID_IDirectSound{,8}) but for Phase A we get most of
// the way there by silencing the WAVE_MAPPER fallback.
let g_silent_audio_enabled = false;

// MessageBox redirect.  Same intent as src/dev_hooks.c — a blocked modal
// invisibly stalls the harness.  Frida hooks the user32 prologue from
// the outside.
let g_msgbox_redirect = true;

// Launcher-dialog auto-handling.  sotes.exe shows a #32770 modal
// dialog at startup ("Fortune Summoners Ver1.2 - Product Ver. -") with
// graphics-settings controls and a Launch button.  The harness needs
// to bypass it without user interaction; all three default on.
//
//   force_windowed     — ensures Windowed Mode is selected so the engine
//                        never takes over the desktop in fullscreen even
//                        with the window hidden (DirectDraw exclusive
//                        fullscreen would still grab the monitor).
//   auto_disable_sound — ticks "Disable Sound" so the engine skips audio
//                        init entirely (cleaner than our silent_audio
//                        clamps because nothing gets allocated).
//   auto_click_launch  — clicks the Launch button to dismiss the dialog.
let g_force_windowed     = true;
let g_auto_disable_sound = true;
let g_auto_click_launch  = true;

// Manual frame counter.  Bumps on every PeekMessage iteration that
// drains a message — close enough for Phase A bootstrapping until we
// have a real "end-of-frame" anchor from Ghidra.
let g_manual_frame_counter = 0;
let g_msg_event_count = 0;

// Periodic telemetry pacing — emit a tick every N main-loop spins so
// the Python driver knows the agent is alive without flooding the
// channel.
const TICK_EVERY = 250;

// Gate for GetTickCount virtualization.  The engine has several
// pre-pump init busy-waits (asset loader progress polls, DDraw mode
// settle, DInput device enum) that livelock if the clock jumps
// forward 17 ms per call instead of with real wall-clock.  Flip this
// to true once we see the first PeekMessage call from main thread —
// that's the engine entering steady-state per-scene loops.
let g_pump_entered = false;

// Periodic window-enumeration pass.  Catches *any* HWND owned by our
// process regardless of which API created it (CreateWindowEx*,
// DialogBox*, internal user32 path, …).  Emits hwnd_seen events with
// class name + title + visibility so the Python driver can decide
// what to interact with (auto-click Launch, tick Disable-Sound, etc.).
//
// The launcher dialog is a Win32 #32770 modal created via
// DialogBoxParamA reading a template from PE resources — it bypasses
// the public user32!CreateWindowEx*, user32!ShowWindow, and
// user32!SetWindowPos exports entirely (dialog manager uses internal
// _xxxCreateDialog and friends).  So our prologue hooks DO NOT catch
// it; only this periodic scan does.  To keep the on-screen flash
// imperceptible we scan very fast (every 8 ms) until we see the main
// game window (class CLASS_LIZSOFT_SOTES); after that, back off to
// 200 ms.
const WINDOW_SCAN_FAST_MS = 8;
const WINDOW_SCAN_SLOW_MS = 200;
let   g_window_scan_started = false;
let   g_window_scan_handle  = null;
let   g_main_game_window_seen = false;
let   g_activateapp_posts = 0;
const g_seen_hwnd_keys = {};   // dedupe: emit once per (hwnd, title) pair

// ─── structural-parity harness: call-trace + mem-watch ───────────────────
//
// Counterpart of the port-side src/call_trace.c emitter.  Together with
// tools/call_trace_diff.py they let us diff which engine functions retail
// called per frame vs. which our port reached.
//
// Frame anchor = the DirectDraw Flip dispatcher (FUN_005b8fc0).  The
// engine calls it once per presented frame; we hook its entry, flush the
// per-frame call-trace / mem-watch batch tagged with the frame that just
// ended, then bump.  The port bumps g_frame_counter once per zdd_present,
// so the two frame axes are directly comparable (modulo boot/load skew,
// which call_trace_diff's --align-on-first absorbs).  This is the
// DirectDraw analog of openrecet's D3D-Present anchor.
const FLIP_VA       = 0x5b8fc0;
let   g_flip_frame  = 0;
let   g_flip_hooked = false;

// call_trace mode (Interceptor.attach onEnter over a vetted VA list).
let   g_call_trace_enabled = false;
let   g_call_trace_frames  = null;   // Set<int> or null (null = every frame)
let   g_call_trace_vas     = [];     // [int] Ghidra VAs to hook
let   g_call_trace_hooked  = false;
let   g_call_trace_buffer  = [];
let   g_call_trace_n_ok    = 0;
let   g_call_trace_n_fail  = 0;
// Per-frame flush keeps steady-state batches small, but the pre-Flip
// boot window (hundreds of init calls with the full VA set hooked) can
// dump a huge batch into one send().  Frida's transport tops out around
// 128MiB per blob; force a flush when the buffer crosses this so each
// message stays bounded.
const CALL_TRACE_FLUSH_AT = 20000;

// mem_watch mode (MemoryAccessMonitor over Ghidra-VA regions).
let   g_mem_watch_enabled  = false;
let   g_mem_watch_regions  = [];   // [{va,size,label,access}]
let   g_mem_watch_ranges   = [];   // cached [{base:NativePointer,size}] for re-arm
let   g_mem_watch_buffer   = [];
let   g_mem_watch_n        = 0;    // in-region hits recorded this session
let   g_mem_watch_neighbor = 0;    // page-neighbor traps skipped (precise mode)
let   g_mem_watch_rearm    = 0;    // re-arm count (budget against the cap)
let   g_mem_watch_precise  = true;
const MEM_WATCH_FLUSH_AT   = 256;
const MEM_WATCH_MAX_HITS   = 4000;
const MEM_WATCH_REARM_CAP  = 200000;

// ─── frame capture (DDraw surface → 24bpp BMP) ───────────────────────────
//
// The DirectDraw analog of openrecet's D3D back-buffer readback.  On a
// whitelisted Flip frame we GetDC the surface, BitBlt it into a 24bpp
// top-down DIB (GDI does the RGB565→BGR conversion for free), read the
// bits, and send() them to the driver, which writes frame_NNNNN.bmp.
//
// Runs INLINE on the engine thread (Interceptor onEnter), so there is no
// cross-thread DDraw hazard — the surface isn't locked by anyone else at
// that instant.
//
// Surface source (pinned by r2 disasm of the render path + GetDC wrapper
// FUN_005b94e0):
//   obj      = *(0x8a93cc)              ; screen object  (mov ecx,[0x8a93cc])
//   paintCtx = *(obj + 0x16c)           ; engine paint_ctx wrapper
//   surface  = *(paintCtx + 0x2c)       ; the real IDirectDrawSurface7
//                                         (FUN_005b94e0: mov eax,[ecx+0x2c];
//                                          call [[eax]+0x44] = GetDC)
// This is the render-target surface the engine GetDC's, draws into, then
// Flips — so reading it at Flip onEnter yields the freshly-drawn frame
// (no back-buffer hop needed).
const GOD_SCREEN_VA       = 0x8a93cc;   // DAT_008a93cc → screen object
const SCREEN_PAINTCTX_OFF = 0x16c;      // screen->[0x16c] = paint_ctx wrapper
const PAINTCTX_SURFACE_OFF = 0x2c;      // paint_ctx->[0x2c] = LPDIRECTDRAWSURFACE7

// IDirectDrawSurface7 vtable indices (verified: Flip=11 ⇒ byte 0x2c, see
// zdd_surface_flip).  Standard DX7 surface vtable order.
const V_SURF_FLIP            = 11;
const V_SURF_GETATTACHED     = 12;
const V_SURF_GETDC           = 17;
const V_SURF_GETSURFACEDESC  = 22;
const V_SURF_RELEASEDC       = 26;

const DDSCAPS_BACKBUFFER  = 0x00000004;
const DDSD2_SIZE          = 124;        // sizeof(DDSURFACEDESC2)
const DDSD2_DWHEIGHT_OFF  = 8;
const DDSD2_DWWIDTH_OFF   = 12;
const SRCCOPY             = 0x00CC0020;
const DIB_RGB_COLORS      = 0;

let g_capture_enabled = false;
let g_capture_frames  = null;   // Set<int> or null (capture every Flip frame)
let g_capture_n       = 0;
let g_capture_diag_done = false;   // one-shot surface-acquisition diagnostic

// ─── deterministic input injection ───────────────────────────────────────
//
// On a hidden, unfocused window DInput produces no events, so the engine's
// input ring is ours to fill.  We resolve the input-manager by hooking the
// poll consumer FUN_0043c110 (ecx = this = manager), then inject synthetic
// event records into its ring so a scripted trace replays as real presses.
//
// Ring layout (docs/findings/input.md): 64 dword slots at mgr+0x0c..mgr+0x108,
// each a pointer to a 3-dword record {id@0, ts@4, flag@8}.  The poll scans
// newest-first (slot 63 → 0), matches id within a 100 ms GetTickCount window,
// and consumes on read (zeroes rec.id).  Newest event = highest slot index.
//
// Trace schema (one JSON object per line): {"frame": N, "ids": [2, 36]}.
// Each line injects those button ids as one fresh press apiece at the first
// poll at-or-after Flip frame N.  Discrete-event model (not a held mask):
// menu nav wants single presses.  Button ids: 1 up, 2 down, 3 left, 4 right,
// 0x24 select/commit, 0x22 abort.
const INPUT_POLL_VA       = 0x43c110;   // FUN_0043c110 input_poll_consume
const RING_BASE_OFF       = 0x0c;       // mgr+0x0c = ring slot 0
const RING_SLOTS          = 64;
const REC_ID_OFF          = 0x00;
const REC_TS_OFF          = 0x04;
const REC_FLAG_OFF        = 0x08;
const REC_SIZE            = 0x0c;
const INJECT_POOL_N       = 32;         // reusable record structs

let g_inject_enabled  = false;
let g_inject_trace    = [];     // [{frame, ids:[...]}] sorted by frame
let g_inject_i        = 0;      // cursor into g_inject_trace
let g_input_mgr       = null;   // resolved at first poll
let g_inject_pool     = null;   // Memory.alloc(INJECT_POOL_N * REC_SIZE)
let g_inject_pool_i   = 0;      // round-robin record allocator
let g_inject_hooked   = false;
let g_inject_n        = 0;      // records injected this session
let g_GetTickCount    = null;
let g_poll_dbg_lo     = 0;      // focused poll-debug frame window [lo,hi]
let g_poll_dbg_hi     = 0;
let g_gdi_ready       = false;
let g_gdi = {};                 // resolved GDI/user32 NativeFunctions

// ── cursor-level probe (R1 investigation) ────────────────────────────────
// Hook the menu-cursor draw wrapper FUN_0056c470 (__thiscall) and log the
// per-frame level_num it is actually called with.  The port drives this to
// idx 19 (full add); retail's value is the path-dependent [esp+0x20] that
// couldn't be pinned statically.  parity-ledger R1: the port cursor is
// uniformly over-bright, so we need retail's real level_num to match it.
const CURSOR_DRAW_VA      = 0x56c470;
let   g_cursor_probe      = false;
let   g_cursor_probe_hooked = false;

// ── intro-fade probe (LOGO/SPARKLE wiring verification, ckpt 30) ──────────
// Hook the fade→alpha ramp helper FUN_00448c80 (__thiscall ecx=0x8a6b60),
// called everywhere as 0x448c80(value, divisor).  In phases 0..4 it is
// invoked once per flip with (intro_fade, 1000) — the studio/title logo fade
// — so the FIRST call per flip gives retail's logo fade at that flip.  We log
// {frame,value,div} for that first call so a port frame can be matched to a
// retail golden at an equal fade and diffed (the R1 method, now for the
// logos).  Cheap: skipped after the first call of each flip.
const FADE_RAMP_VA        = 0x448c80;
let   g_fade_probe        = false;
let   g_fade_probe_hooked = false;
let   g_fade_last_flip    = -1;

// ─── RNG seed pin (phase-7 sparkle parity, ckpt 31) ──────────────────────
// Retail seeds srand(time()) at boot (FUN_00562210 @0x56227a), so the engine
// LCG (FUN_005bf505, seed DAT_008a4f94) — and every random effect through it,
// the phase-7 sparkle spawn FUN_0056c070 first among them — is wall-clock
// dependent and not reproducible.  To diff the port's twinkles bit-for-bit we
// pin the seed: write a fixed value into DAT_008a4f94 immediately before the
// FIRST 0x56c070 spawn, so retail enters phase-7 spawning from the same LCG
// state the port boots with (the port makes no rand() calls before then).
// One-shot — re-pinning per spawn would make every particle identical.  Any
// divergence that survives this pin is an unaccounted rand() consumer firing
// between spawns on one side: a real ordering bug to chase, not RNG noise.
const SPAWN_VA            = 0x56c070;   // FUN_0056c070 (sparkle spawn)
const SEED_VA             = 0x8a4f94;   // DAT_008a4f94 (LCG seed word)
const RAND_VA             = 0x5bf505;   // FUN_005bf505 (MSVC rand)

// ─── rand() call-site probe (TAS RNG-desync diagnosis) ───────────────────
// Tally every rand() call between two scene anchors by CALLER return address,
// so an unaccounted rand consumer (a transition/animation the port skips)
// can be located by VA.  Armed by the newgame_enter scene anchor, flushed +
// disarmed at prologue_enter.  Off unless --rand-probe.
let g_rand_probe        = false;
let g_rand_window_active = false;
let g_rand_callers      = {};   // caller-VA (hex) -> count
let g_rand_total        = 0;
let g_rand_hooked       = false;
let   g_seed_pin          = false;
let   g_seed_value        = 0x4f5347;   // OSS_RNG_DEFAULT_SEED (matches port)
let   g_seed_pinned       = false;      // one-shot latch

// ─── intro-pace probe (parity residual R3) ──────────────────────────────
// Timestamps Flips so the flip-RATE can be measured: it discriminates
// "retail rushes vs port" between two hypotheses for the flip-index gap —
//   (A) render artifact: retail reaches the menu in ~the same WALL-CLOCK
//       time as the port (~60 Hz phase updates) but emits many *duplicate*
//       flips per update (high render throughput), OR
//   (B) extra per-phase delays: retail genuinely takes ~Nx longer.
// (A) ⇒ high flips/sec; (B) ⇒ ~60 flips/sec.  We emit a `pace_sample`
// every g_pace_every flips with the elapsed wall-clock since the first
// flip; combined with the cursor probe's menu-onset flip this is the whole
// test.  Use with --no-turbo (real clock).
let   g_pace_probe        = false;
let   g_pace_every        = 30;
let   g_pace_t0           = 0;     // Date.now() at the first flip

// ─── GDI text probe (text-renderer parity, ckpt 36 part 2) ───────────────
// Hook gdi32!TextOutA / ExtTextOutA — the per-glyph primitive the dynamic-
// text renderer (glyph_row_draw, 0x48e860) emits, one call per glyph.  For
// each draw we record (x, y, the c glyph bytes, text colour, bk mode) plus
// the LOGFONTA of the font currently selected into the DC (GetCurrentObject
// OBJ_FONT + GetObjectA).  This is the retail GROUND TRUTH for the GDI text
// path: which of the 8 ar_register_fonts HFONTs a menu selects, the text/
// shadow colours, and — from the x advance between consecutive glyphs — the
// real per-byte advance the port hard-codes as 7 (14 per SJIS pair).  Dedup
// by (x|y|bytes|colour|bkmode|font) so a menu redrawn every frame yields its
// distinct glyph-draw set once, not thousands of identical dupes.
const OBJ_FONT            = 6;
let   g_textout_probe     = false;
let   g_textout_hooked    = false;
let   g_textout_lo        = 0;     // flip-window [lo,hi]: record only within it
let   g_textout_hi        = 0x7fffffff;  // (avoids flooding on intro/debug text)
let   g_textout_seen      = {};    // dedup key -> first flip frame seen
let   g_textout_n         = 0;     // distinct draws emitted
let   g_textout_calls     = 0;     // total TextOut* calls observed
const TEXTOUT_CAP         = 4000;  // distinct-draw emit cap (safety valve)
let   g_textout_q         = {};    // resolved GDI query NativeFunctions
let   g_textout_lfbuf     = null;  // Memory.alloc LOGFONTA scratch (60 bytes)

// ─── box-widget render probe (new-game config chrome, ckpt 40) ───────────
// Hook FUN_0048d940 (the sprite-cell node render) — the path the box widget's
// bordered-panel slices blit through (cream fill + bevel edges + ornate gold
// corners, all sprite frames from the box-art bank at scene+0xb8c).  For each
// call, gated to a flip window + deduped by node|frame, we reconstruct exactly
// as 0x48d940 does: the box-art bank's PE resource id (node+0x28 -> slot,
// slot+0x40), the selected sprite frame (base node+0x2c + frame-list
// node+0x2e[node+0x72]), the resolved frame record's dst offset (+0xc/+0x10) +
// size (+0x14/+0x18), and the on-screen dst rect (the type-1/2 position math).
// This is the GROUND TRUTH for the box composition the static build (0x411940
// + 40f3e0/411ec0) lays out: which resource, which frame ids, where each slice
// lands — the one thing not statically greppable (the bank field is set by an
// embedded-subobject ctor, never written as a literal offset in the corpus).
const BOX_RENDER_VA       = 0x48d940;
let   g_box_probe         = false;
let   g_box_hooked        = false;
let   g_box_lo            = 0;     // flip-window [lo,hi]: record only within it
let   g_box_hi            = 0x7fffffff;
let   g_box_seen          = {};    // dedup key (node|frameSel) -> first flip
let   g_box_n             = 0;     // distinct cells emitted
const BOX_CAP             = 4000;  // safety valve

// ─── helpers ────────────────────────────────────────────────────────────

function rva(va) {
    if (g_base === null) {
        throw new Error('rva() called before module base resolved');
    }
    return g_base.add(ptr(va).sub(IMAGE_BASE));
}

function logmsg(msg) {
    send({ kind: 'log', msg: msg });
}

function err(where, msg) {
    send({ kind: 'error', where: where, msg: msg });
}

// Frida 17 removed the static Memory.readUtf8String/Memory.readUtf16String
// in favor of method-on-NativePointer.  Stay on the method form
// everywhere so we don't trip the API mismatch again.
function readAStringOrNull(p) {
    if (p === null || p.isNull()) return '';
    try { return p.readUtf8String(); }
    catch (e) { return '(unreadable A:' + p + ')'; }
}

function readWStringOrNull(p) {
    if (p === null || p.isNull()) return '';
    try { return p.readUtf16String(); }
    catch (e) { return '(unreadable W:' + p + ')'; }
}

// Milliseconds since the agent started (matches openrecet's `ts` field).
function nowMs() { return Date.now() - g_start_real_ms; }

// Map a live NativePointer into the main module back to a Ghidra VA
// (module-relative offset + IMAGE_BASE).  Returns a JS number.
function toGhidraVa(p) {
    if (g_base === null || p === null || p.isNull()) return 0;
    try { return p.sub(g_base).add(IMAGE_BASE).toUInt32(); }
    catch (e) { return 0; }
}

// Module-relative offset of a return address — matches the port-side
// emitter's ret_va convention (add IMAGE_BASE for the caller's Ghidra VA).
function traceRetVa(p) {
    if (g_base === null || p === null || p.isNull()) return 0;
    try { return p.sub(g_base).toUInt32(); }
    catch (e) { return 0; }
}

// ─── installers ─────────────────────────────────────────────────────────

// Resolve an export from a (possibly-not-yet-loaded) module.  Frida 17.x
// moved away from the static `Module.findExportByName(modName, exp)` API
// in favor of instance methods on a Module object.  We additionally
// fall back to Module.load() when the DLL hasn't been imported yet (so a
// hook can install before the engine has called e.g. CoCreateInstance
// that pulls dsound.dll in).
function resolveExport(modName, exportName) {
    let mod = Process.findModuleByName(modName);
    if (!mod) {
        try { mod = Module.load(modName); }
        catch (e) { return null; }
    }
    if (!mod) return null;
    if (typeof mod.findExportByName === 'function') {
        return mod.findExportByName(exportName);
    }
    if (typeof mod.getExportByName === 'function') {
        try { return mod.getExportByName(exportName); }
        catch (e) { return null; }
    }
    return null;
}

function installMessageBoxRedirect() {
    if (!g_msgbox_redirect) return;
    const user32 = resolveExport('user32.dll', 'MessageBoxA');
    const user32W = resolveExport('user32.dll', 'MessageBoxW');

    if (user32) {
        Interceptor.replace(user32, new NativeCallback(function (hwnd, text, caption, type) {
            const c = readAStringOrNull(caption);
            const b = readAStringOrNull(text);
            send({ kind: 'messagebox', api: 'MessageBoxA',
                   caption: c, body: b, type: type >>> 0 });
            return 1; // IDOK
        }, 'int', ['pointer', 'pointer', 'pointer', 'uint']));
        logmsg('Interceptor.replace MessageBoxA');
    }
    if (user32W) {
        Interceptor.replace(user32W, new NativeCallback(function (hwnd, text, caption, type) {
            const c = readWStringOrNull(caption);
            const b = readWStringOrNull(text);
            send({ kind: 'messagebox', api: 'MessageBoxW',
                   caption: c, body: b, type: type >>> 0 });
            return 1; // IDOK
        }, 'int', ['pointer', 'pointer', 'pointer', 'uint']));
        logmsg('Interceptor.replace MessageBoxW');
    }
}

function installHwndOwnershipTracking() {
    // Track every CreateWindowEx{A,W} return so we know which HWNDs are
    // ours and only force-hide those.
    //
    // We hook BOTH onEnter and onLeave:
    //   onEnter — when hide-window is on, strip WS_VISIBLE (0x10000000)
    //     from the dwStyle arg BEFORE the call proceeds.  This is the
    //     only way to avoid a brief on-screen flash for windows that
    //     are created visible (the launcher dialog #32770 is one such);
    //     by the time onLeave fires the user has already seen the window.
    //   onLeave — record the HWND so the rest of the agent (ShowWindow
    //     hook, SetWindowPos hook, periodic scan) knows it's ours.
    //
    // WS_VISIBLE = 0x10000000.  arg index map for CreateWindowEx{A,W}:
    //   [0]=dwExStyle [1]=lpClassName [2]=lpWindowName [3]=dwStyle …
    //
    // We also capture the class name in onEnter so the onLeave message
    // can emit it (debugging aid + helps the human map HWND -> dialog).
    const WS_VISIBLE = 0x10000000;

    ['CreateWindowExA', 'CreateWindowExW'].forEach(function (name) {
        const p = resolveExport('user32.dll', name);
        if (!p) return;
        const isW = name.endsWith('W');
        Interceptor.attach(p, {
            onEnter: function (args) {
                if (g_hide_window) {
                    const style = args[3].toInt32() >>> 0;
                    if (style & WS_VISIBLE) {
                        args[3] = ptr(style & ~WS_VISIBLE);
                        this._stripped_visible = true;
                    }
                }
                // Stash class-name pointer for the onLeave logger.
                try {
                    this._cls = args[1].isNull()
                        ? ''
                        : (isW ? args[1].readUtf16String()
                               : args[1].readUtf8String()) || '';
                } catch (e) { this._cls = '(?)'; }
            },
            onLeave: function (retval) {
                if (retval.isNull()) return;
                const key = retval.toString();
                g_owned_hwnds[key] = { source: name };
                send({ kind: 'hwnd_owned', hwnd: key, source: name,
                       cls: this._cls,
                       ws_visible_stripped: !!this._stripped_visible });
            }
        });
        logmsg('attached ' + name + ' (strip WS_VISIBLE + track return)');
    });
}

function isOwned(hwndPtr) {
    if (!hwndPtr || hwndPtr.isNull()) return false;
    return g_owned_hwnds[hwndPtr.toString()] !== undefined;
}

function installHideWindowHook() {
    if (!g_hide_window) return;

    const show = resolveExport('user32.dll', 'ShowWindow');
    if (show) {
        Interceptor.attach(show, {
            onEnter: function (args) {
                if (!isOwned(args[0])) return;
                const orig = args[1].toInt32();
                if (orig !== 0 /* SW_HIDE */) {
                    send({ kind: 'hide_force', api: 'ShowWindow',
                           hwnd: args[0].toString(), original: orig });
                    args[1] = ptr(0);
                }
            }
        });
        logmsg('attached ShowWindow (hide-force)');
    }

    const showAsync = resolveExport('user32.dll', 'ShowWindowAsync');
    if (showAsync) {
        Interceptor.attach(showAsync, {
            onEnter: function (args) {
                if (!isOwned(args[0])) return;
                const orig = args[1].toInt32();
                if (orig !== 0) {
                    send({ kind: 'hide_force', api: 'ShowWindowAsync',
                           hwnd: args[0].toString(), original: orig });
                    args[1] = ptr(0);
                }
            }
        });
        logmsg('attached ShowWindowAsync (hide-force)');
    }

    // SetWindowPos with SWP_SHOWWINDOW (0x0040) needs the flag stripped.
    const swp = resolveExport('user32.dll', 'SetWindowPos');
    if (swp) {
        Interceptor.attach(swp, {
            onEnter: function (args) {
                if (!isOwned(args[0])) return;
                const flags = args[6].toInt32();
                if (flags & 0x0040 /* SWP_SHOWWINDOW */) {
                    send({ kind: 'hide_force', api: 'SetWindowPos',
                           hwnd: args[0].toString(), original: flags });
                    args[6] = ptr(flags & ~0x0040);
                }
            }
        });
        logmsg('attached SetWindowPos (strip SWP_SHOWWINDOW)');
    }
}

function installMessagePumpCounter() {
    // Tick the manual frame counter on PeekMessage/GetMessage.  This is a
    // coarse proxy for "the engine ran a frame" until we have a real
    // end-of-frame anchor.
    //
    // Side effect: flip g_pump_entered on the first non-zero return so
    // the GetTickCount/WaitMessage virtualization gates open.  We use
    // PeekMessage's call entry (regardless of return value) as the
    // gate because the very first pump invocation might have zero
    // messages but we still want clock virt active from then on.
    ['PeekMessageA', 'PeekMessageW', 'GetMessageA', 'GetMessageW'].forEach(function (name) {
        const p = resolveExport('user32.dll', name);
        if (!p) return;
        Interceptor.attach(p, {
            onEnter: function () {
                if (!g_pump_entered) {
                    g_pump_entered = true;
                    send({ kind: 'log',
                           msg: 'pump entered (' + name + ') — clock virt active' });
                }
            },
            onLeave: function (retval) {
                // PeekMessage returns BOOL: 0 = no message.  Only count
                // non-zero returns so we approximate "real frames done".
                if (retval.toInt32() === 0) return;
                g_msg_event_count++;
                if (g_msg_event_count % TICK_EVERY === 0) {
                    send({ kind: 'msg', source: name, count: g_msg_event_count });
                }
            }
        });
    });
    logmsg('attached message-pump counter');
}

function installTurboHooks() {
    if (!g_turbo_enabled) return;

    // Sleep → Sleep(0).  Yields the timeslice but doesn't actually
    // wait.  We can't true-noop Sleep because a busy main thread that
    // never yields will starve any background thread (audio mixer,
    // asset loader I/O) that needs CPU to make progress — and the
    // main thread is often polling a flag set by exactly such a
    // thread.  Sleep(0) keeps the engine fast without livelock.
    const sleep = resolveExport('kernel32.dll', 'Sleep');
    if (sleep) {
        const realSleep = new NativeFunction(sleep, 'void', ['uint']);
        Interceptor.replace(sleep, new NativeCallback(function (ms) {
            realSleep(0);
        }, 'void', ['uint']));
        logmsg('Interceptor.replace Sleep (yield via Sleep(0))');
    }

    // Virtualised timeGetTime.  Kept for cross-project compatibility but
    // Fortune Summoners does NOT import timeGetTime — see
    // docs/findings/winmain-and-bootstrap.md.  GetTickCount below is the
    // real cadence source for this engine.
    const tgt = resolveExport('winmm.dll', 'timeGetTime');
    if (tgt) {
        const realTimeGetTime = new NativeFunction(tgt, 'uint32', []);
        Interceptor.replace(tgt, new NativeCallback(function () {
            const tid = Process.getCurrentThreadId();
            if (g_main_thread_id === -1) {
                g_main_thread_id = tid;
                logmsg('main thread captured: tid=' + tid);
            }
            if (g_clock_virtualised && tid === g_main_thread_id) {
                g_virtual_now_ms += g_turbo_step_ms;
                if ((g_virtual_now_ms / g_turbo_step_ms) % TICK_EVERY === 0) {
                    send({ kind: 'turbo_tick', virtual_ms: g_virtual_now_ms });
                }
                return g_virtual_now_ms;
            }
            return realTimeGetTime();
        }, 'uint32', []));
        g_clock_virtualised = true;
        logmsg('Interceptor.replace timeGetTime (virtual)');
    }

    // Virtualised GetTickCount — the actual cadence source.  Fortune
    // Summoners' frame limiter (FUN_005b1030) and ~30 scene functions
    // call GetTickCount to decide whether enough time has passed since
    // the last tick.  Pin to the main thread so background threads
    // (audio mixer, asset loader I/O) still see real wall-clock.
    //
    // Gate: only virtualise AFTER the main game window appears.  The
    // engine's pre-window init has its own busy-wait loops keyed off
    // GetTickCount (e.g., asset loader progress polls, DDraw mode-set
    // settling waits) that livelock if the clock advances 17 ms per
    // call instead of monotonically with real wall-clock.
    const gtc = resolveExport('kernel32.dll', 'GetTickCount');
    if (gtc) {
        const realGetTickCount = new NativeFunction(gtc, 'uint32', []);
        Interceptor.replace(gtc, new NativeCallback(function () {
            const tid = Process.getCurrentThreadId();
            if (g_main_thread_id === -1) {
                g_main_thread_id = tid;
                logmsg('main thread captured (via GetTickCount): tid=' + tid);
            }
            // Virtualise only after the pump has been entered.
            // Pre-pump init has its own busy-waits keyed off
            // GetTickCount; those livelock if the clock advances 17 ms
            // per call instead of with real wall-clock.
            if (g_clock_virtualised && tid === g_main_thread_id &&
                g_pump_entered) {
                // Lockstep: once armed (first Flip seen) the clock is
                // frozen between Flips — return g_lock_clock_ms, advanced
                // only by the Flip hook — plus a tiny per-call creep so
                // busy-waits cannot hang.  Before arming, fall through to
                // the ordinary per-call turbo clock (boot/load safe).
                if (g_lockstep && g_lock_armed) {
                    // Pure freeze during normal play; creep only once a
                    // flipless busy-wait has clearly stalled (load settle).
                    if (++g_lock_calls_since_flip > g_lockstep_stall_calls)
                        g_lock_clock_ms += g_lockstep_epsilon_ms;
                    return g_lock_clock_ms;
                }
                g_virtual_now_ms += g_turbo_step_ms;
                if ((g_virtual_now_ms / g_turbo_step_ms) % TICK_EVERY === 0) {
                    send({ kind: 'turbo_tick', virtual_ms: g_virtual_now_ms });
                }
                return g_virtual_now_ms;
            }
            return realGetTickCount();
        }, 'uint32', []));
        g_clock_virtualised = true;
        logmsg('Interceptor.replace GetTickCount (virtual after pump enters)');
    }

    // Stub WaitMessage so the pump never blocks.  FUN_005b1030 calls
    // WaitMessage to yield until the 10 ms SetTimer fires; with the
    // virtualised clock above, real time has stopped and the timer
    // would never deliver, so WaitMessage would hang forever.
    // Returning immediately turns the pump into a busy-spin which is
    // what we want for turbo.
    //
    // Pin to the main thread for the same reason as the clock hooks:
    // background threads (audio mixer, asset loader) may use
    // WaitMessage to wait on real OS events, and they need real OS
    // semantics to avoid livelock.
    const wait = resolveExport('user32.dll', 'WaitMessage');
    if (wait) {
        const realWaitMessage = new NativeFunction(wait, 'int', []);
        Interceptor.replace(wait, new NativeCallback(function () {
            const tid = Process.getCurrentThreadId();
            if (g_main_thread_id !== -1 && tid === g_main_thread_id) {
                return 1;
            }
            return realWaitMessage();
        }, 'int', []));
        logmsg('Interceptor.replace WaitMessage (main thread → no-block)');
    }
}

// Periodic enumeration of every window owned by the current process.
// This is our truth-source for "what's actually on screen", independent
// of which user32 API created the window.  It also doubles as the
// launcher-dialog detector for the auto-click task (find a button
// labeled "Launch" → BM_CLICK; find a checkbox labeled "Disable sound" →
// BM_SETCHECK; etc).
// Bypass the engine's launcher dialog by wrapping its DLGPROC.
//
// Ghidra finding (sotes.unpacked.exe): the launcher is a single call
//     DialogBoxParamA(hInst, MAKEINTRESOURCE(0x2711), NULL, dlgProc, 0)
// where dlgProc is at VA 0x004013c0 (resource ID 0x2711 = 10001 is the
// dialog template).  The dialog manager:
//   1. CreateWindowEx (internal) the dialog and its children, invisible
//      so far (no WS_VISIBLE in template style)
//   2. SendMessage WM_INITDIALOG → dlgProc.  The engine's handler loads
//      the saved Screen Mode / Graphics Quality / VRAM Use / Disable
//      Sound selections from gl.cfg (or wherever) and BM_SETCHECKs the
//      matching controls
//   3. Add WS_VISIBLE + ShowWindow + start modal message loop ← this
//      is where the flash happens, BEFORE step 2 returns control
//
// If we synchronously SendMessage(launchBtn, BM_CLICK) inside the
// engine's WM_INITDIALOG (after calling the original handler), the
// engine's IDOK handler fires NOW — it reads the now-properly-checked
// control states, commits them, and calls EndDialog.  The dialog
// manager sees EndDialog has been called and skips step 3 entirely.
// No paint, no flash.
//
// Bonus: this is the same code path a real user click takes, so the
// engine's settings persistence (gl.cfg write) runs exactly as it
// would for a manually-launched session.
const g_wrapped_dlgprocs = {};       // dedupe wrapper per original-proc address
let   g_dialog_intercepted = false;  // first DialogBoxParamA wins

function getOrCreateDlgWrapper(originalPtr, LAUNCH_CTRL_ID) {
    const key = originalPtr.toString();
    if (g_wrapped_dlgprocs[key]) return g_wrapped_dlgprocs[key];

    // INT_PTR CALLBACK DLGPROC(HWND hwndDlg, UINT msg, WPARAM wp, LPARAM lp);
    // On Win32 (32-bit), INT_PTR is 32-bit and the calling convention
    // is __stdcall.
    const original = new NativeFunction(originalPtr,
        'int', ['pointer', 'uint32', 'pointer', 'pointer'], 'stdcall');

    const u32 = Process.findModuleByName('user32.dll');
    const sma = u32.findExportByName('SendMessageA');
    const gci = u32.findExportByName('GetDlgItem');
    if (!sma || !gci) {
        send({ kind: 'error', where: 'dlg_wrap',
               msg: 'SendMessageA / GetDlgItem missing' });
        return null;
    }
    const SendMessageA = new NativeFunction(sma, 'long',
        ['pointer', 'uint32', 'long', 'long'], 'stdcall');
    const GetDlgItem  = new NativeFunction(gci, 'pointer',
        ['pointer', 'int'], 'stdcall');

    const WM_INITDIALOG = 0x0110;
    const BM_SETCHECK   = 0x00F1;
    const BM_CLICK      = 0x00F5;
    const BST_CHECKED   = 0x0001;

    // Control IDs from docs/findings/engine-quirks.md §3 (captured
    // 2026-05-24 via the EnumChildWindows pass).
    const CTRL_WINDOWED      = 10020;
    const CTRL_DISABLE_SOUND = 10024;

    const wrapper = new NativeCallback(function (hDlg, msg, wParam, lParam) {
        try {
            if (msg === WM_INITDIALOG) {
                // Let the engine load saved settings from gl.cfg or
                // wherever — its WM_INITDIALOG handler BM_SETCHECKs the
                // appropriate radios.
                const ret = original(hDlg, msg, wParam, lParam);

                // Force our desired safety settings: windowed mode +
                // disable sound (matches the auto-handler we used to
                // run from the periodic scan).
                if (g_force_windowed) {
                    const winBtn = GetDlgItem(hDlg, CTRL_WINDOWED);
                    if (!winBtn.isNull())
                        SendMessageA(winBtn, BM_SETCHECK, BST_CHECKED, 0);
                }
                if (g_auto_disable_sound) {
                    const sndBtn = GetDlgItem(hDlg, CTRL_DISABLE_SOUND);
                    if (!sndBtn.isNull())
                        SendMessageA(sndBtn, BM_SETCHECK, BST_CHECKED, 0);
                }

                // Synchronously fire the Launch button click.  The
                // engine's WM_COMMAND handler reads the currently-
                // checked controls, persists them, and calls EndDialog.
                if (g_auto_click_launch) {
                    const launchBtn = GetDlgItem(hDlg, LAUNCH_CTRL_ID);
                    if (!launchBtn.isNull()) {
                        SendMessageA(launchBtn, BM_CLICK, 0, 0);
                        send({ kind: 'dialog_action',
                               action: 'wm_initdialog_click',
                               target: 'launch',
                               hwnd: hDlg.toString() });
                    } else {
                        send({ kind: 'error', where: 'dlg_wrap_initdialog',
                               msg: 'Launch button GetDlgItem returned NULL' });
                    }
                }
                return ret;
            }
            return original(hDlg, msg, wParam, lParam);
        } catch (e) {
            send({ kind: 'error', where: 'dlgproc_wrap', msg: '' + e });
            return 0;
        }
    }, 'int', ['pointer', 'uint32', 'pointer', 'pointer'], 'stdcall');

    g_wrapped_dlgprocs[key] = wrapper;
    return wrapper;
}

function installDialogBypass() {
    if (!g_auto_click_launch) return;
    const LAUNCH_CTRL_ID = 10003;  // see engine-quirks.md §3

    ['DialogBoxParamA', 'DialogBoxParamW',
     'DialogBoxIndirectParamA', 'DialogBoxIndirectParamW'].forEach(function (name) {
        const p = resolveExport('user32.dll', name);
        if (!p) return;
        Interceptor.attach(p, {
            onEnter: function (args) {
                if (g_dialog_intercepted) return;  // first dialog only
                // For DialogBoxParamA the DLGPROC is arg 3; same for all
                // four variants (HINSTANCE, template-or-ptr, parent,
                // DLGPROC, initParam).
                const originalProc = args[3];
                if (originalProc.isNull()) return;
                const wrapper = getOrCreateDlgWrapper(originalProc, LAUNCH_CTRL_ID);
                if (!wrapper) return;
                args[3] = wrapper;
                g_dialog_intercepted = true;
                send({ kind: 'log',
                       msg: 'wrapped DLGPROC for ' + name +
                            ' (original @ ' + originalProc + ')' });
            }
        });
        logmsg('attached ' + name + ' (DLGPROC wrap for Launch-click bypass)');
    });
}

function installPeriodicWindowScan() {
    if (g_window_scan_started) return;
    const u32 = Process.findModuleByName('user32.dll');
    if (!u32) { err('window_scan', 'user32.dll not loaded'); return; }

    const enumWindows        = u32.findExportByName('EnumWindows');
    const enumChildWindows   = u32.findExportByName('EnumChildWindows');
    const getWindowThreadPid = u32.findExportByName('GetWindowThreadProcessId');
    const getClassNameA      = u32.findExportByName('GetClassNameA');
    const getWindowTextA     = u32.findExportByName('GetWindowTextA');
    const isWindowVisible    = u32.findExportByName('IsWindowVisible');
    const postMessageA       = u32.findExportByName('PostMessageA');
    const sendMessageA       = u32.findExportByName('SendMessageA');
    const getDlgCtrlID       = u32.findExportByName('GetDlgCtrlID');
    if (!enumWindows || !enumChildWindows || !getWindowThreadPid ||
        !getClassNameA || !getWindowTextA || !isWindowVisible ||
        !postMessageA || !sendMessageA || !getDlgCtrlID) {
        err('window_scan', 'one of the required user32 exports is missing');
        return;
    }

    const EnumWindows = new NativeFunction(enumWindows,
        'int', ['pointer', 'long']);
    const EnumChildWindows = new NativeFunction(enumChildWindows,
        'int', ['pointer', 'pointer', 'long']);
    const GetWindowThreadProcessId = new NativeFunction(getWindowThreadPid,
        'uint32', ['pointer', 'pointer']);
    const GetClassNameA  = new NativeFunction(getClassNameA,
        'int', ['pointer', 'pointer', 'int']);
    const GetWindowTextA = new NativeFunction(getWindowTextA,
        'int', ['pointer', 'pointer', 'int']);
    const IsWindowVisible = new NativeFunction(isWindowVisible,
        'int', ['pointer']);
    const PostMessageA = new NativeFunction(postMessageA,
        'int', ['pointer', 'uint32', 'long', 'long']);
    const SendMessageA = new NativeFunction(sendMessageA,
        'long', ['pointer', 'uint32', 'long', 'long']);
    const GetDlgCtrlID = new NativeFunction(getDlgCtrlID,
        'int', ['pointer']);

    // Win32 message constants we use for auto-interaction with the
    // launcher dialog.  See MSDN; values are stable across Windows
    // versions.
    const WM_COMMAND   = 0x0111;
    const BM_CLICK     = 0x00F5;
    const BM_GETCHECK  = 0x00F0;
    const BM_SETCHECK  = 0x00F1;
    const BST_CHECKED  = 0x0001;
    const BST_UNCHECKED= 0x0000;
    const BN_CLICKED   = 0;

    const ourPid = Process.id;
    const pidBuf = Memory.alloc(4);
    const cnBuf  = Memory.alloc(256);
    const wtBuf  = Memory.alloc(512);

    // Child-window enumeration callback.  Called once per child of the
    // launcher dialog so we can find the Launch button and the
    // disable-sound checkbox by their label text.
    const dialogState = { launchClicked: false };

    const childCb = new NativeCallback(function (hwnd, lparam) {
        try {
            GetClassNameA(hwnd, cnBuf, 256);
            const cls = cnBuf.readUtf8String() || '';
            GetWindowTextA(hwnd, wtBuf, 512);
            const text = wtBuf.readUtf8String() || '';
            const ctrlId = GetDlgCtrlID(hwnd);

            send({ kind: 'dialog_child', hwnd: hwnd.toString(),
                   cls: cls, text: text, ctrlId: ctrlId });

            // Button heuristics: lowercase compare against known labels.
            // Fortune Summoners' launcher uses English labels in the EN
            // release ("Launch" / "Disable Sound" — TBD exact wording;
            // we'll tighten this once a run logs the actual text).
            const t = text.toLowerCase();

            if (g_force_windowed &&
                cls === 'Button' &&
                (t.indexOf('windowed') !== -1 ||
                 t.indexOf('window mode') !== -1)) {
                const cur = SendMessageA(hwnd, BM_GETCHECK, 0, 0);
                if (cur !== BST_CHECKED) {
                    SendMessageA(hwnd, BM_SETCHECK, BST_CHECKED, 0);
                    // Also send WM_COMMAND to the parent so the radio
                    // group's other buttons get un-checked properly.
                    // The Win32 docs are explicit that BM_SETCHECK on a
                    // BS_AUTORADIOBUTTON doesn't trigger the group
                    // exclusivity update — only WM_COMMAND does.
                    send({ kind: 'dialog_action', action: 'check',
                           target: 'windowed_mode', text: text,
                           hwnd: hwnd.toString() });
                }
            }

            if (g_auto_disable_sound &&
                cls === 'Button' &&
                (t.indexOf('disable sound') !== -1 ||
                 t.indexOf('no sound') !== -1 ||
                 t.indexOf('mute') !== -1)) {
                const cur = SendMessageA(hwnd, BM_GETCHECK, 0, 0);
                if (cur !== BST_CHECKED) {
                    SendMessageA(hwnd, BM_SETCHECK, BST_CHECKED, 0);
                    send({ kind: 'dialog_action', action: 'check',
                           target: 'disable_sound', text: text,
                           hwnd: hwnd.toString() });
                }
            }

            if (g_auto_click_launch && !dialogState.launchClicked &&
                cls === 'Button' &&
                (t.indexOf('launch') !== -1 ||
                 t.indexOf('start') !== -1 ||
                 t === 'ok' || t === '&ok' ||
                 t.indexOf('play') !== -1 ||
                 t.indexOf('開始') !== -1 || t.indexOf('起動') !== -1)) {
                // Use BM_CLICK so the button fires its own WM_COMMAND
                // notification chain to the dialog's WndProc.  This
                // mirrors a real user click — IsDialogMessage will
                // correctly notice and close the modal.
                PostMessageA(hwnd, BM_CLICK, 0, 0);
                dialogState.launchClicked = true;
                send({ kind: 'dialog_action', action: 'click',
                       target: 'launch', text: text,
                       hwnd: hwnd.toString() });
            }
        } catch (e) {
            send({ kind: 'error', where: 'child_cb', msg: '' + e });
        }
        return 1;  // continue
    }, 'int', ['pointer', 'long']);

    const cb = new NativeCallback(function (hwnd, lparam) {
        try {
            GetWindowThreadProcessId(hwnd, pidBuf);
            const pid = pidBuf.readU32();
            if (pid !== ourPid) return 1;  // continue

            GetClassNameA(hwnd, cnBuf, 256);
            const cls = cnBuf.readUtf8String() || '';
            GetWindowTextA(hwnd, wtBuf, 512);
            const title = wtBuf.readUtf8String() || '';
            const visible = IsWindowVisible(hwnd) !== 0;
            const key = hwnd.toString() + '|' + cls + '|' + title +
                        '|' + (visible ? '1' : '0');
            if (g_seen_hwnd_keys[key]) {
                // We've already logged this exact state.  Still drive
                // dialog auto-handling on every scan in case the user
                // dismissed our injected click somehow (or a different
                // dialog with the same key appears later).
            } else {
                g_seen_hwnd_keys[key] = true;
                send({ kind: 'hwnd_seen', hwnd: hwnd.toString(),
                       cls: cls, title: title, visible: visible });
            }

            // Launcher-dialog auto-handling.  The Win32 modal dialog
            // class is "#32770"; the engine's launcher uses it as
            // observed in the 2026-05-24 smoke run.  Drive auto-click
            // on every scan until the click is registered.
            if (cls === '#32770' && (g_auto_click_launch || g_auto_disable_sound)) {
                EnumChildWindows(hwnd, childCb, 0);
            }

            // Belt-and-braces: also force-hide if the user asked for it.
            // Combined hide: ShowWindow(SW_HIDE) handles the public case;
            // SetWindowPos with HWND_BOTTOM + SWP_NOACTIVATE + huge
            // negative position + 1x1 size shoves the window off-screen
            // in case ShowWindow alone doesn't take (some Win32 modal
            // dialog managers reassert visibility internally).  Together
            // these make the window imperceptible even if it briefly
            // exists on the desktop between our scans.
            if (g_hide_window && visible) {
                const u = Process.findModuleByName('user32.dll');
                if (u) {
                    const sw = u.findExportByName('ShowWindow');
                    if (sw) new NativeFunction(sw, 'int',
                        ['pointer', 'int'])(hwnd, 0);
                    const spp = u.findExportByName('SetWindowPos');
                    if (spp) new NativeFunction(spp, 'int',
                        ['pointer', 'pointer', 'int', 'int', 'int', 'int', 'uint32'])
                        (hwnd, ptr(1) /* HWND_BOTTOM */,
                         -32000, -32000, 1, 1,
                         0x10 /* SWP_NOACTIVATE */ |
                         0x80 /* SWP_HIDEWINDOW */);
                    send({ kind: 'hide_force', api: 'periodic-scan',
                           hwnd: hwnd.toString(), original: 1 });
                }
                g_owned_hwnds[hwnd.toString()] = { source: 'periodic-scan' };
            }

            // Detect main game window appearance → back off scan rate
            // AND post WM_ACTIVATEAPP(TRUE) so the engine's WndProc
            // (FUN_005b12e0) flips its DAT_008a952c "app is active"
            // flag.  Without that, FUN_005b1030's pump loop never
            // breaks out and the engine never advances past init —
            // hidden windows don't naturally receive WM_ACTIVATEAPP
            // from the OS when no real focus change happens.
            //
            // Post on EVERY scan iteration (not just first sight) for
            // a short retry window — the first post can race the
            // engine's pump entry, and a single missed post means
            // hangs until something else activates the app.
            if (cls === 'CLASS_LIZSOFT_SOTES') {
                if (!g_main_game_window_seen) {
                    g_main_game_window_seen = true;
                    if (g_window_scan_handle !== null) {
                        clearInterval(g_window_scan_handle);
                        g_window_scan_handle = setInterval(scan, WINDOW_SCAN_SLOW_MS);
                        logmsg('main window appeared — scan rate → ' +
                               WINDOW_SCAN_SLOW_MS + ' ms');
                    }
                }
                const WM_ACTIVATEAPP = 0x001C;
                const rc = PostMessageA(hwnd, WM_ACTIVATEAPP, 1, 0);
                if (g_activateapp_posts < 3) {
                    send({ kind: 'log',
                           msg: 'posted WM_ACTIVATEAPP(TRUE) → ' +
                                hwnd.toString() + ' rc=' + rc });
                    g_activateapp_posts++;
                }
            }
        } catch (e) {
            send({ kind: 'error', where: 'window_scan_cb', msg: '' + e });
        }
        return 1;  // continue
    }, 'int', ['pointer', 'long']);

    function scan() {
        try { EnumWindows(cb, 0); }
        catch (e) { send({ kind: 'error', where: 'EnumWindows', msg: '' + e }); }
    }
    g_window_scan_handle = setInterval(scan, WINDOW_SCAN_FAST_MS);
    g_window_scan_started = true;
    logmsg('installed periodic window scan (every ' + WINDOW_SCAN_FAST_MS + ' ms fast, → ' +
           WINDOW_SCAN_SLOW_MS + ' ms after main window)');
}

function installSilentAudioHooks() {
    if (!g_silent_audio_enabled) return;

    // waveOutSetVolume(hwo, dwVolume) — clamp the high/low 16-bit volume
    // words to 0.  Hooked via attach so the API still returns success
    // and the engine's "audio is ready" gates take the positive branch.
    const wosv = resolveExport('winmm.dll', 'waveOutSetVolume');
    if (wosv) {
        Interceptor.attach(wosv, {
            onEnter: function (args) { args[1] = ptr(0); },
            onLeave: function () {
                send({ kind: 'silent_audio', api: 'waveOutSetVolume' });
            }
        });
        logmsg('attached waveOutSetVolume (clamp 0)');
    }

    // DirectSound layer: we can hook DirectSoundCreate{,8} return to wrap
    // the IDirectSound interface, but that requires vtable surgery on the
    // returned COM pointer.  Save that for Phase B once we see what the
    // engine actually calls.  For now, the WAVE_MAPPER fallback above
    // handles the legacy code path most engines from this era use as a
    // backup when DirectSound init fails.
}

// ─── module-load wait ──────────────────────────────────────────────────

// ─── frame anchor + call-trace + mem-watch ───────────────────────────────

// Resolve the GDI + user32 entry points used by the BitBlt capture path.
// Lazy: only the capture path needs them, and gdi32/user32 are always
// loaded by the time the engine reaches a Flip.
function ensureGdiFns() {
    if (g_gdi_ready) return true;
    const gdi  = Process.findModuleByName('gdi32.dll');
    if (!gdi) { err('ensureGdiFns', 'gdi32.dll not loaded'); return false; }
    const E = function (mod, name, ret, args) {
        const p = mod.findExportByName(name);
        if (!p) throw new Error('missing export ' + name);
        return new NativeFunction(p, ret, args, 'stdcall');
    };
    try {
        g_gdi.CreateCompatibleDC = E(gdi, 'CreateCompatibleDC', 'pointer', ['pointer']);
        g_gdi.CreateDIBSection   = E(gdi, 'CreateDIBSection',   'pointer',
            ['pointer', 'pointer', 'uint32', 'pointer', 'pointer', 'uint32']);
        g_gdi.SelectObject       = E(gdi, 'SelectObject',       'pointer', ['pointer', 'pointer']);
        g_gdi.BitBlt             = E(gdi, 'BitBlt',             'int',
            ['pointer', 'int', 'int', 'int', 'int', 'pointer', 'int', 'int', 'uint32']);
        g_gdi.DeleteObject       = E(gdi, 'DeleteObject',       'int', ['pointer']);
        g_gdi.DeleteDC           = E(gdi, 'DeleteDC',           'int', ['pointer']);
    } catch (e) { err('ensureGdiFns', '' + e); return false; }
    g_gdi_ready = true;
    return true;
}

// vtable[idx] of a COM object pointer.
function vtSlot(obj, idx) {
    return obj.readPointer().add(idx * Process.pointerSize).readPointer();
}

// The render-target IDirectDrawSurface7 from the engine god object, or NULL.
function getPrimarySurface() {
    try {
        const obj = rva(GOD_SCREEN_VA).readPointer();
        if (obj.isNull()) return NULL;
        const paintCtx = obj.add(SCREEN_PAINTCTX_OFF).readPointer();
        if (paintCtx.isNull()) return NULL;
        return paintCtx.add(PAINTCTX_SURFACE_OFF).readPointer();
    } catch (e) { return NULL; }
}

// (w, h) of a surface via GetSurfaceDesc, or null.
function getSurfaceDims(surf) {
    try {
        const d = Memory.alloc(DDSD2_SIZE);
        d.writeU32(DDSD2_SIZE);   // dwSize — required or DDERR_INVALIDPARAMS
        const getDesc = new NativeFunction(
            vtSlot(surf, V_SURF_GETSURFACEDESC), 'uint32',
            ['pointer', 'pointer'], 'stdcall');
        const hr = getDesc(surf, d);
        if (hr !== 0) return null;
        return { w: d.add(DDSD2_DWWIDTH_OFF).readU32(),
                 h: d.add(DDSD2_DWHEIGHT_OFF).readU32() };
    } catch (e) { return null; }
}

// GetDC → BitBlt into a 24bpp top-down DIB → send the bits.  Whole thing
// runs on the engine thread at Flip onEnter.
// One-shot dump of the surface-acquisition chain so we can pin the right
// pointers + vtable indices from a single live run.
function captureDiag() {
    if (g_capture_diag_done) return;
    g_capture_diag_done = true;
    try {
        const obj = rva(GOD_SCREEN_VA).readPointer();
        let line = 'diag: *(0x8a93cc)=' + obj;
        if (!obj.isNull()) {
            const surf = obj.add(SCREEN_PRIMARY_OFF).readPointer();
            line += ' primary=*(obj+0x16c)=' + surf;
            if (!surf.isNull()) {
                const vt = surf.readPointer();
                line += ' vtbl=' + vt;
                // GetSurfaceDesc on primary
                try {
                    const d = Memory.alloc(DDSD2_SIZE); d.writeU32(DDSD2_SIZE);
                    const gd = new NativeFunction(vtSlot(surf, V_SURF_GETSURFACEDESC),
                        'uint32', ['pointer', 'pointer'], 'stdcall');
                    const hr = gd(surf, d);
                    line += ' GetDesc[22]hr=0x' + (hr >>> 0).toString(16) +
                            ' w=' + d.add(DDSD2_DWWIDTH_OFF).readU32() +
                            ' h=' + d.add(DDSD2_DWHEIGHT_OFF).readU32();
                } catch (e) { line += ' GetDesc[22]EXC=' + e; }
                // GetAttachedSurface (backbuffer)
                try {
                    const caps = Memory.alloc(16); caps.writeU32(DDSCAPS_BACKBUFFER);
                    const pp = Memory.alloc(4); pp.writePointer(NULL);
                    const ga = new NativeFunction(vtSlot(surf, V_SURF_GETATTACHED),
                        'uint32', ['pointer', 'pointer', 'pointer'], 'stdcall');
                    const hr = ga(surf, caps, pp);
                    line += ' GetAttached[12]hr=0x' + (hr >>> 0).toString(16) +
                            ' bb=' + pp.readPointer();
                } catch (e) { line += ' GetAttached[12]EXC=' + e; }
            }
        }
        logmsg(line);
    } catch (e) { err('captureDiag', '' + e); }
}

function captureFrame(frameNumber) {
    if (!ensureGdiFns()) return;
    const surf = getPrimarySurface();
    if (surf.isNull()) { err('captureFrame', 'no render surface yet'); return; }
    const fromBack = false;

    const dims = getSurfaceDims(surf);
    if (!dims || dims.w === 0 || dims.h === 0) {
        if (!g_capture_diag_done) captureDiag();
        err('captureFrame', 'bad surface dims @frame ' + frameNumber);
        return;
    }
    const w = dims.w, h = dims.h;

    // GetDC on the DDraw surface.
    const phdc = Memory.alloc(Process.pointerSize); phdc.writePointer(NULL);
    const getDC = new NativeFunction(vtSlot(surf, V_SURF_GETDC), 'uint32',
        ['pointer', 'pointer'], 'stdcall');
    let hr = getDC(surf, phdc);
    if (hr !== 0) { err('captureFrame/GetDC', 'hr=0x' + (hr >>> 0).toString(16)); return; }
    const srcDC = phdc.readPointer();

    let memDC = NULL, hbmp = NULL, oldObj = NULL;
    try {
        memDC = g_gdi.CreateCompatibleDC(srcDC);
        if (memDC.isNull()) { err('captureFrame', 'CreateCompatibleDC failed'); return; }

        // BITMAPINFOHEADER: 24bpp, negative height = top-down, BI_RGB.
        const bmi = Memory.alloc(40);
        bmi.writeU32(40);                       // biSize
        bmi.add(4).writeS32(w);                 // biWidth
        bmi.add(8).writeS32(-h);                // biHeight (top-down)
        bmi.add(12).writeU16(1);                // biPlanes
        bmi.add(14).writeU16(24);               // biBitCount
        bmi.add(16).writeU32(0);                // biCompression = BI_RGB
        const ppBits = Memory.alloc(Process.pointerSize); ppBits.writePointer(NULL);
        hbmp = g_gdi.CreateDIBSection(memDC, bmi, DIB_RGB_COLORS, ppBits, NULL, 0);
        if (hbmp.isNull()) { err('captureFrame', 'CreateDIBSection failed'); return; }
        oldObj = g_gdi.SelectObject(memDC, hbmp);

        if (g_gdi.BitBlt(memDC, 0, 0, w, h, srcDC, 0, 0, SRCCOPY) === 0) {
            err('captureFrame', 'BitBlt failed @frame ' + frameNumber);
            return;
        }

        const bits   = ppBits.readPointer();
        const stride  = (w * 3 + 3) & ~3;       // DIB rows are DWORD-aligned
        const total   = stride * h;
        const ab      = bits.readByteArray(total);
        send({ kind: 'frame', frame: frameNumber, w: w, h: h,
               stride: stride, bpp: 24, from_back: fromBack }, ab);
        g_capture_n++;
    } finally {
        if (!oldObj.isNull()) g_gdi.SelectObject(memDC, oldObj);
        if (!hbmp.isNull())   g_gdi.DeleteObject(hbmp);
        if (!memDC.isNull())  g_gdi.DeleteDC(memDC);
        const releaseDC = new NativeFunction(vtSlot(surf, V_SURF_RELEASEDC), 'uint32',
            ['pointer', 'pointer'], 'stdcall');
        try { releaseDC(surf, srcDC); } catch (e) { /* surface may be transient */ }
    }
}

function captureShouldEmit() {
    if (!g_capture_enabled) return false;
    if (g_capture_frames === null) return true;
    return g_capture_frames.has(g_flip_frame);
}

// ─── input injection ─────────────────────────────────────────────────────

function ensureTickFn() {
    if (g_GetTickCount) return true;
    const k = Process.findModuleByName('kernel32.dll');
    if (!k) return false;
    const p = k.findExportByName('GetTickCount');
    if (!p) return false;
    g_GetTickCount = new NativeFunction(p, 'uint32', [], 'stdcall');
    return true;
}

// Write one fresh press record for `id` into the newest free ring slot
// (top-down), so the engine's newest-first poll picks it up.  `ts` MUST be
// the engine's own per-frame `now` (the poll's first arg) — the poll tests
// (uint32_t)(now - rec.ts) <= 100, and the engine caches GetTickCount once
// per frame, so stamping with a *later* GetTickCount() underflows the window
// and the press is silently dropped.
function injectPress(id, slotIdx, ts) {
    const rec = g_inject_pool.add((g_inject_pool_i % INJECT_POOL_N) * REC_SIZE);
    g_inject_pool_i++;
    rec.add(REC_ID_OFF).writeU32(id >>> 0);
    rec.add(REC_TS_OFF).writeU32(ts >>> 0);
    rec.add(REC_FLAG_OFF).writeU32(1);
    const slotAddr = g_input_mgr.add(RING_BASE_OFF + slotIdx * 4);
    slotAddr.writePointer(rec);
}

// Inject every trace entry whose frame is at-or-before the current Flip
// frame, one record per id.  Fires each entry exactly once.  `engineNow`
// is the poll's cached tick (used verbatim as the record timestamp).
function injectDueEntries(engineNow) {
    if (!g_inject_enabled || g_input_mgr === null) return;
    while (g_inject_i < g_inject_trace.length &&
           g_flip_frame >= g_inject_trace[g_inject_i].frame) {
        const e = g_inject_trace[g_inject_i];
        const ids = e.ids || [];
        for (let j = 0; j < ids.length; j++) {
            // Fill the top slots (63, 62, …) so each is "newest".
            injectPress(ids[j], RING_SLOTS - 1 - j, engineNow);
            g_inject_n++;
        }
        send({ kind: 'inject', frame: g_flip_frame, trace_frame: e.frame,
               ids: ids, now: engineNow });
        g_inject_i++;
    }
}

function installInputInjection() {
    if (g_inject_hooked) return;
    g_inject_pool = Memory.alloc(INJECT_POOL_N * REC_SIZE);
    // Focused poll/latch debug window around the first scripted press —
    // only when explicitly requested (g_poll_dbg_hi already set by init).
    Interceptor.attach(rva(INPUT_POLL_VA), {
        onEnter: function (args) {
            // ecx = this = the *current scene's* input-manager (thiscall).
            // Track it every poll — a sub-scene (e.g. the new-game difficulty
            // menu) uses a DIFFERENT manager instance, so a once-cached mgr
            // would inject into the wrong ring (record never consumed).
            const mgr = this.context.ecx;
            if (g_input_mgr === null || !g_input_mgr.equals(mgr)) {
                g_input_mgr = mgr;
                send({ kind: 'input_mgr_resolved', mgr: mgr.toString() });
            }
            // __thiscall(now, button_id): stack = [retaddr][now][button_id].
            // Read the engine's cached per-frame tick for the record ts.
            let engineNow = 0, btnId = -1;
            try { engineNow = this.context.esp.add(4).readU32(); } catch (e) {}
            try { btnId = this.context.esp.add(8).readU32(); } catch (e) {}
            if (engineNow === 0) engineNow = ensureTickFn() ? g_GetTickCount() : 0;
            // Focused debug window around the scripted press.
            if (g_poll_dbg_hi > 0 && g_flip_frame >= g_poll_dbg_lo &&
                g_flip_frame <= g_poll_dbg_hi) {
                let s63 = 0;
                try {
                    const p = g_input_mgr.add(RING_BASE_OFF + 63 * 4).readPointer();
                    if (!p.isNull()) s63 = p.add(REC_ID_OFF).readU32();
                } catch (e) {}
                send({ kind: 'poll_dbg', frame: g_flip_frame, btn: btnId,
                       now: engineNow, slot63_id: s63 });
            }
            try { injectDueEntries(engineNow); }
            catch (e) { err('injectDueEntries', e.message); }
        },
    });
    // Debug: watch the menu latch gate (FUN_0043ce50, __thiscall(dir, now);
    // ecx = menu_ctrl).  menu_ctrl+0 = sub; sub+0x04 = enabled; sub+0x54 =
    // ready (must == 1000 to latch).  Logs in the same focused window.
    try {
        Interceptor.attach(rva(0x43ce50), {
            onEnter: function () {
                if (g_poll_dbg_hi <= 0 || g_flip_frame < g_poll_dbg_lo ||
                    g_flip_frame > g_poll_dbg_hi) return;
                let dir = -1, ready = -1, enabled = -1;
                try { dir = this.context.esp.add(4).readU32(); } catch (e) {}
                try {
                    const sub = this.context.ecx.readPointer();
                    if (!sub.isNull()) {
                        enabled = sub.add(0x04).readU32();
                        ready   = sub.add(0x54).readS32();
                    }
                } catch (e) {}
                send({ kind: 'latch_dbg', frame: g_flip_frame, dir: dir,
                       ready: ready, enabled: enabled });
            },
        });
    } catch (e) { err('latch_dbg_hook', '' + e); }

    g_inject_hooked = true;
    logmsg('input injection hooked @ FUN_0043c110 (' +
           g_inject_trace.length + ' trace entries)');
}

// Hook the DDraw Flip dispatcher (FUN_005b8fc0) so its entry is the
// per-frame boundary: flush the batches accumulated during the frame that
// is ending (tagged with that frame number), THEN bump.  Same
// flush-before-bump invariant openrecet uses on D3D Present.
function installFlipFrameHook() {
    if (g_flip_hooked) return;
    Interceptor.attach(rva(FLIP_VA), {
        onEnter: function () {
            // Lockstep clock: bank exactly one update quantum per present.
            // Arm on the first Flip so boot/asset-load ran on the per-call
            // turbo clock; seed continuity from g_virtual_now_ms so the
            // scene's pace anchors see no discontinuity.
            if (g_lockstep) {
                if (!g_lock_armed) {
                    g_lock_armed   = true;
                    g_lock_clock_ms = g_virtual_now_ms;
                    send({kind: 'lockstep_armed', frame: g_flip_frame,
                          clock_ms: g_lock_clock_ms,
                          step_ms: g_lockstep_step_ms,
                          epsilon_ms: g_lockstep_epsilon_ms});
                }
                g_lock_clock_ms += g_lockstep_step_ms;
                g_lock_calls_since_flip = 0;   // reset the stall-breaker
            }
            if (g_call_trace_enabled) {
                try { callTraceFlush(g_flip_frame); }
                catch (e) { err('flip.callTraceFlush', e.message); }
            }
            if (g_mem_watch_enabled) {
                try { memWatchFlush(g_flip_frame); }
                catch (e) { err('flip.memWatchFlush', e.message); }
            }
            if (captureShouldEmit()) {
                try { captureFrame(g_flip_frame); }
                catch (e) { err('flip.captureFrame', e.message); }
            }
            if (g_pace_probe) {
                if (g_pace_t0 === 0) {
                    g_pace_t0 = Date.now();
                    send({kind: 'pace_sample', frame: g_flip_frame, ms: 0});
                } else if (g_flip_frame % g_pace_every === 0) {
                    send({kind: 'pace_sample', frame: g_flip_frame,
                          ms: Date.now() - g_pace_t0});
                }
            }
            g_flip_frame++;
        },
    });
    g_flip_hooked = true;
    logmsg('flip frame anchor installed @ FUN_005b8fc0');
    send({kind: 'flip_hook_ready', va: FLIP_VA});
}

function callTraceShouldEmit() {
    if (!g_call_trace_enabled) return false;
    if (g_call_trace_frames === null) return true;
    return g_call_trace_frames.has(g_flip_frame);
}

function callTraceFlush(frameNumber) {
    if (g_call_trace_buffer.length === 0) return;
    const events = g_call_trace_buffer;
    g_call_trace_buffer = [];
    send({kind:   'call_trace_batch',
          frame:  frameNumber,
          count:  events.length,
          events: events});
}

function installCallTraceHooks(vasArray) {
    if (g_call_trace_hooked) return;
    // Per-iteration let-binding so each closure reports its own VA.
    for (let i = 0; i < vasArray.length; i++) {
        const va = vasArray[i] | 0;
        try {
            Interceptor.attach(rva(va), {
                onEnter: function () {
                    if (!callTraceShouldEmit()) return;
                    g_call_trace_buffer.push({
                        va:     va,
                        ret_va: traceRetVa(this.returnAddress),
                        ts:     nowMs(),
                        thr:    this.threadId,
                    });
                    if (g_call_trace_buffer.length >= CALL_TRACE_FLUSH_AT) {
                        callTraceFlush(g_flip_frame);
                    }
                },
            });
            g_call_trace_n_ok++;
        } catch (e) {
            // Frida couldn't trampoline this VA (unsupported prefix, a
            // prior hook on the same byte, etc.).  Counted so the driver
            // can spot a degraded run; the bisect tool carves crashers
            // out of the VA list separately.
            g_call_trace_n_fail++;
        }
    }
    g_call_trace_hooked = true;
    logmsg('call_trace: hooked ' + g_call_trace_n_ok + ' VAs (' +
           g_call_trace_n_fail + ' failed) of ' + vasArray.length);
    send({kind:   'call_trace_hooked',
          n_ok:   g_call_trace_n_ok,
          n_fail: g_call_trace_n_fail,
          n_req:  vasArray.length});
}

// Hook FUN_0056c470 (the menu-cursor draw) and report the level_num it is
// invoked with, per Flip frame.  __thiscall: `this` in ECX, explicit params
// on the stack (args[0]=dest, args[1]=sprite, args[2]=level_num,
// args[3]=level_div=0x4b0, args[4]=x, args[5]=y).  We dump the first 8 stack
// slots raw on the first hit so the layout can be confirmed against the
// known constants (level_div==0x4b0, x==0, y==0x10+row*0x20), then emit a
// compact {frame,num,div,x,y} per call.  Cheap: one call per drawn frame.
function installCursorProbe() {
    if (g_cursor_probe_hooked) return;
    let first = true;
    Interceptor.attach(rva(CURSOR_DRAW_VA), {
        onEnter: function (args) {
            const slots = [];
            for (let i = 0; i < 8; i++) {
                try { slots.push(args[i].toInt32()); }
                catch (e) { slots.push(null); }
            }
            let ecx = 0;
            try { ecx = this.context.ecx.toUInt32(); } catch (e) {}
            if (first) {
                first = false;
                send({kind: 'cursor_probe_first', frame: g_flip_frame,
                      ecx: ecx, slots: slots,
                      ms: (g_pace_t0 ? Date.now() - g_pace_t0 : null),
                      ret_va: traceRetVa(this.returnAddress)});
            }
            // Identify level_div by its known constant 0x4b0; level_num is the
            // slot immediately before it.  Fall back to the documented layout.
            let div = slots[3], num = slots[2], x = slots[4], y = slots[5];
            const k = slots.indexOf(0x4b0);
            if (k >= 1) { div = slots[k]; num = slots[k - 1];
                          x = slots[k + 1]; y = slots[k + 2]; }
            send({kind: 'cursor_level', frame: g_flip_frame,
                  num: num, div: div, x: x, y: y});
        },
    });
    g_cursor_probe_hooked = true;
    logmsg('cursor probe installed @ FUN_0056c470');
}

// Hook FUN_00448c80 (the fade→alpha ramp) and report the first (value, div)
// it is called with each Flip frame.  __thiscall ecx=0x8a6b60; stack args
// args[0]=value (= the intro fade in phases 0..4), args[1]=divisor (1000).
function installFadeProbe() {
    if (g_fade_probe_hooked) return;
    let first = true;
    Interceptor.attach(rva(FADE_RAMP_VA), {
        onEnter: function (args) {
            if (g_flip_frame === g_fade_last_flip) return;   // first call/flip only
            g_fade_last_flip = g_flip_frame;
            let value = null, div = null;
            try { value = args[0].toInt32(); } catch (e) {}
            try { div   = args[1].toInt32(); } catch (e) {}
            if (first) {
                first = false;
                let ecx = 0;
                try { ecx = this.context.ecx.toUInt32(); } catch (e) {}
                send({kind: 'fade_probe_first', frame: g_flip_frame,
                      ecx: ecx, value: value, div: div,
                      ret_va: traceRetVa(this.returnAddress)});
            }
            send({kind: 'fade_level', frame: g_flip_frame,
                  value: value, div: div});
        },
    });
    g_fade_probe_hooked = true;
    logmsg('fade probe installed @ FUN_00448c80');
}

// Resolve the gdi32 query functions the text probe uses to read DC state
// (text colour, bk mode) and the selected font's LOGFONTA.  Allocates the
// 60-byte LOGFONTA scratch once.  Returns false if gdi32 is unavailable.
function ensureTextOutQueryFns() {
    if (g_textout_q.ready) return true;
    const gdi = Process.findModuleByName('gdi32.dll');
    if (!gdi) { err('textout_probe', 'gdi32.dll not loaded'); return false; }
    const E = function (name, ret, args) {
        const pp = gdi.findExportByName(name);
        if (!pp) throw new Error('missing export ' + name);
        return new NativeFunction(pp, ret, args, 'stdcall');
    };
    try {
        g_textout_q.GetTextColor     = E('GetTextColor',     'uint32',  ['pointer']);
        g_textout_q.GetBkMode        = E('GetBkMode',        'int',     ['pointer']);
        g_textout_q.GetCurrentObject = E('GetCurrentObject', 'pointer', ['pointer', 'uint32']);
        g_textout_q.GetObjectA       = E('GetObjectA',       'int',     ['pointer', 'int', 'pointer']);
    } catch (e) { err('textout_probe', '' + e); return false; }
    g_textout_lfbuf = Memory.alloc(64);
    g_textout_q.ready = true;
    return true;
}

// Read the LOGFONTA of the font currently selected into `hdc`, or null.
function readSelectedFont(hdc) {
    try {
        const hf = g_textout_q.GetCurrentObject(hdc, OBJ_FONT);
        if (hf.isNull()) return null;
        const n = g_textout_q.GetObjectA(hf, 60, g_textout_lfbuf);
        if (n < 28) return null;
        const b = g_textout_lfbuf;
        return {
            h:       b.readS32(),
            w:       b.add(4).readS32(),
            weight:  b.add(16).readS32(),
            italic:  b.add(20).readU8(),
            charset: b.add(23).readU8(),
            face:    b.add(28).readCString(),
        };
    } catch (e) { return null; }
}

// Shared body for the TextOutA / ExtTextOutA hooks.  `strPtr` is the glyph
// string (NOT NUL-terminated) and `c` its length in bytes.
function textOutOnEnter(hdc, x, y, strPtr, c) {
    // Cheap gates FIRST — before any GDI queries — so the intro/attract debug
    // text outside the target window adds ~zero overhead (it otherwise floods
    // the channel and slows the process below the menu).
    if (g_flip_frame < g_textout_lo || g_flip_frame > g_textout_hi) return;
    if (g_textout_n >= TEXTOUT_CAP) return;
    g_textout_calls++;
    const bytes = [];
    let text = '';
    try {
        const n = (c > 0 && c < 64) ? c : 0;
        if (n > 0) {
            const u8 = new Uint8Array(strPtr.readByteArray(n));
            for (let i = 0; i < n; i++) {
                bytes.push(u8[i]);
                text += (u8[i] >= 0x20 && u8[i] < 0x7f)
                    ? String.fromCharCode(u8[i]) : '.';
            }
        }
    } catch (e) {}
    let color = null, bkmode = null;
    try { color  = g_textout_q.GetTextColor(hdc) >>> 0; } catch (e) {}
    try { bkmode = g_textout_q.GetBkMode(hdc); } catch (e) {}
    const font = readSelectedFont(hdc);
    const fkey = font ? (font.h + 'x' + font.w + '/' + font.face + '/' + font.italic) : '?';
    const key = x + '|' + y + '|' + bytes.join(',') + '|' + color + '|' + bkmode + '|' + fkey;
    if (g_textout_seen[key] !== undefined) return;   // dedup the per-frame redraw
    g_textout_seen[key] = g_flip_frame;
    g_textout_n++;
    send({kind: 'textout', frame: g_flip_frame, x: x, y: y, c: c,
          bytes: bytes, text: text, color: color, bkmode: bkmode, font: font});
}

function installTextOutProbe() {
    if (g_textout_hooked) return;
    if (!ensureTextOutQueryFns()) return;
    const gdi = Process.findModuleByName('gdi32.dll');
    const to = gdi.findExportByName('TextOutA');
    if (to) {
        // BOOL TextOutA(HDC hdc, int x, int y, LPCSTR str, int c)
        Interceptor.attach(to, {
            onEnter: function (args) {
                textOutOnEnter(args[0], args[1].toInt32(), args[2].toInt32(),
                               args[3], args[4].toInt32());
            },
        });
        logmsg('textout probe installed @ gdi32!TextOutA');
    } else {
        err('textout_probe', 'TextOutA export not found');
    }
    // BOOL ExtTextOutA(HDC, int x, int y, UINT opt, RECT*, LPCSTR str, UINT c, INT* dx)
    const eto = gdi.findExportByName('ExtTextOutA');
    if (eto) {
        Interceptor.attach(eto, {
            onEnter: function (args) {
                textOutOnEnter(args[0], args[1].toInt32(), args[2].toInt32(),
                               args[5], args[6].toInt32());
            },
        });
        logmsg('textout probe installed @ gdi32!ExtTextOutA');
    }
    g_textout_hooked = true;
}

// Hook FUN_0048d940 (sprite-cell render) and dump the box-widget 9-slice
// composition.  __thiscall: ecx = the cell node; stack args = (surface, baseX,
// baseY, p4).  We replicate the function's own resolve+position math so the
// emitted (res_id, frameSel, scrx, scry, w, h) is exactly what blits.  Gated to
// [g_box_lo,g_box_hi] (the box-art bank renders many UI sprites, so a window
// keeps it to the new-game menu) and deduped by node|frameSel (the panel
// redraws the same slice nodes every frame).
function installBoxProbe() {
    if (g_box_hooked) return;
    Interceptor.attach(rva(BOX_RENDER_VA), {
        onEnter: function (args) {
            if (g_flip_frame < g_box_lo || g_flip_frame > g_box_hi) return;
            if (g_box_n >= BOX_CAP) return;
            let node;
            try { node = this.context.ecx; } catch (e) { return; }
            if (node === undefined || node === null || node.isNull()) return;
            try {
                // Render guard (matches 0x48d940's entry test) — recorded, not
                // enforced, so we see every slice node (incl. static ones).
                const enable = node.add(0x20).readU32();
                const skip   = node.add(0x24).readU32();
                const type  = node.add(8).readU32();
                const bank  = node.add(0x28).readPointer();      // puVar2
                if (bank.isNull()) return;
                const slot  = bank.readPointer();                 // *puVar2
                if (slot.isNull()) return;
                const entries = slot.readPointer();               // slot->entries
                let res_id = null;
                try { res_id = slot.add(0x40).readU16(); } catch (e) {}

                const base  = node.add(0x2c).readU16();
                const count = node.add(0x6e).readU16();
                const idx   = node.add(0x72).readU16();
                const frames = [];
                for (let i = 0; i < count && i < 16; i++)
                    frames.push(node.add(0x2e + i * 2).readU16());
                let frameSel = base;
                if (count !== 0 && frames.length > 0)
                    frameSel = (base + frames[idx % frames.length]) & 0xffff;

                let frec = null, fdx = null, fdy = null, fw = null, fh = null;
                if (!entries.isNull()) {
                    frec = entries.add((frameSel & 0xffff) * 4).readPointer();
                    if (!frec.isNull()) {
                        fdx = frec.add(0xc).readS32();
                        fdy = frec.add(0x10).readS32();
                        fw  = frec.add(0x14).readS32();
                        fh  = frec.add(0x18).readS32();
                    }
                }

                const p2 = args[1].toInt32();   // baseX
                const p3 = args[2].toInt32();    // baseY
                let X, Y;
                if (type === 1) {
                    const sub = node.add(0x174).readPointer();
                    const a5  = sub.add(0x14).readS32();
                    const a6  = sub.add(0x18).readS32();
                    const pitch = node.add(0x1ac).readS32();
                    X = node.add(0x7c).readS32() + node.add(0xc).readS32() + p2;
                    Y = (a5 - a6) * pitch + node.add(0x80).readS32() +
                        node.add(0x10).readS32() + p3;
                } else if (type === 2) {
                    X = node.add(0x7c).readS32() + node.add(0xc).readS32() + p2;
                    Y = node.add(0x80).readS32() + node.add(0x10).readS32() + p3;
                } else { X = p2; Y = p3; }
                const scrx = (fdx !== null ? fdx : 0) + X;
                const scry = (fdy !== null ? fdy : 0) + Y;

                const key = node.toUInt32() + '|' + frameSel;
                if (g_box_seen[key] !== undefined) return;
                g_box_seen[key] = g_flip_frame;
                g_box_n++;
                send({kind: 'box_cell', frame: g_flip_frame,
                      node: node.toUInt32(), type: type, res_id: res_id,
                      enable: enable, skip: skip,
                      base: base, count: count, idx: idx, frames: frames,
                      frameSel: frameSel, scrx: scrx, scry: scry,
                      w: fw, h: fh, fdx: fdx, fdy: fdy,
                      bank: bank.toUInt32(), slot: slot.toUInt32()});
            } catch (e) { err('box_probe', '' + e); }
        },
    });
    // The 9-slice box-frame renderers: FUN_0048cb90 (fade-scaled, __thiscall
    // ecx=box node, args=surface,p4) and FUN_0048cf80 (explicit w/h).  These
    // draw the bordered panel — TL/T/TR/L/C/R/BL/B/BR slices from the 9 frame
    // ids at node+0x60..0x72 over the bank node+0x5c.  Dump that whole spec so
    // the static box composition is ground-truthed (bank res id + 9 frames +
    // corner cell w/h + node w/h + fade).
    const dumpBoxFrame = function (tag, node, surf) {
        try {
            if (node === undefined || node === null || node.isNull()) return;
            // The 9-slice bank is the object embedded AT node+0x5c (FUN_0048cf80
            // passes &node[0x5c] as ECX to FUN_00418470, whose *ECX is the slot
            // ptr).  So the slot ptr is the dword at node+0x5c (one deref) — NOT
            // two.  res_id = slot+0x40.
            const slot = node.add(0x5c).readPointer();
            let res_id = null;
            if (!slot.isNull()) {
                try { res_id = slot.add(0x40).readU16(); } catch (e) {}
            }
            const sh = [];                       // the 11 shorts 0x60..0x74
            for (let off = 0x60; off <= 0x74; off += 2)
                sh.push(node.add(off).readU16());
            const spec = {
                tl: node.add(0x60).readU16() + node.add(0x62).readU16(),
                top: node.add(0x64).readU16(), tr: node.add(0x66).readU16(),
                lmid: node.add(0x68).readU16(), center: node.add(0x6a).readU16(),
                rmid: node.add(0x6c).readU16(), bl: node.add(0x6e).readU16(),
                bottom: node.add(0x70).readU16(), br: node.add(0x72).readU16(),
                cornerw: node.add(0x74).readS32(), cornerh: node.add(0x78).readS32(),
            };
            const nodeW = node.add(0x14).readS32();
            const nodeH = node.add(0x18).readS32();
            const fade  = node.add(0x54).readS32();
            const ox = node.add(0xc).readS32(), oy = node.add(0x10).readS32();
            const key = tag + '|' + node.toUInt32();
            if (g_box_seen[key] !== undefined) return;
            g_box_seen[key] = g_flip_frame;
            send({kind: 'box_frame', tag: tag, frame: g_flip_frame,
                  node: node.toUInt32(), res_id: res_id, spec: spec,
                  shorts: sh, nodeW: nodeW, nodeH: nodeH, fade: fade,
                  ox: ox, oy: oy});
        } catch (e) { err('box_frame', '' + e); }
    };
    Interceptor.attach(rva(0x48cb90), {
        onEnter: function (args) {
            if (g_flip_frame < g_box_lo || g_flip_frame > g_box_hi) return;
            dumpBoxFrame('48cb90', this.context.ecx, args[0]);
        },
    });
    Interceptor.attach(rva(0x48cf80), {
        onEnter: function (args) {
            if (g_flip_frame < g_box_lo || g_flip_frame > g_box_hi) return;
            dumpBoxFrame('48cf80', this.context.ecx, args[0]);
        },
    });

    g_box_hooked = true;
    logmsg('box probe installed @ FUN_0048d940 + FUN_0048cb90 + FUN_0048cf80');
}

// Hook the first phase-7 sparkle spawn (FUN_0056c070).  It is the TAS anchor
// for "subtitle animation start" (tick 0 of the particle system) — emitted
// always so any capture/trace can align to it — and, when seed-pinning is on,
// the point at which we overwrite DAT_008a4f94 so the twinkle stream is
// reproducible.  One-shot: only the first spawn of the run.
function installSparkleAnchor() {
    Interceptor.attach(rva(SPAWN_VA), {
        onEnter: function (args) {
            if (g_seed_pinned) return;       // one-shot: first spawn only
            g_seed_pinned = true;
            // Anchor: subtitle/sparkle animation starts here (tick 0).
            send({kind: 'anchor', name: 'subtitle_anim_start',
                  frame: g_flip_frame});
            if (g_seed_pin) {
                const seedAddr = rva(SEED_VA);
                let before = 0;
                try { before = seedAddr.readU32(); } catch (e) {}
                try { seedAddr.writeU32(g_seed_value >>> 0); } catch (e) {
                    err('seed_pin_write', '' + e);
                }
                send({kind: 'seed_pinned', frame: g_flip_frame,
                      before: before >>> 0, value: g_seed_value >>> 0});
            }
        },
    });
    logmsg('sparkle anchor installed @ FUN_0056c070' +
           (g_seed_pin ? ' (+ seed pin DAT_008a4f94 <- 0x' +
            (g_seed_value >>> 0).toString(16) + ')' : ''));
}

// Hook rand() and tally call sites while the window is armed.  Cheap when
// disarmed (one flag test).  The caller VA = the return address minus the
// module base, so it maps straight to a Ghidra address.
function installRandProbe() {
    if (g_rand_hooked) return;
    const base = Process.findModuleByName('sotes.unpacked.exe').base;
    Interceptor.attach(rva(RAND_VA), {
        onEnter: function () {
            if (!g_rand_window_active) return;
            g_rand_total++;
            let caller = '?';
            try {
                caller = '0x' + this.returnAddress.sub(base).toString(16);
            } catch (e) {}
            g_rand_callers[caller] = (g_rand_callers[caller] || 0) + 1;
        },
    });
    g_rand_hooked = true;
    logmsg('rand probe installed @ FUN_005bf505');
}

// ─── scene-boundary TAS anchors ──────────────────────────────────────────
// Beyond the intro's subtitle_anim_start, the trace crosses scene boundaries
// where the flip count diverges from the port (per-scene load cost): the
// new-game config scene (FUN_00564780, case 0x24 runner) and the Elemental-
// Stone prologue cutscene (FUN_0056cd20).  Hook each entry and emit a named
// anchor stamped with the live flip AND the RNG seed (DAT_008a4f94), so the
// trace-diff can re-offset retail's flip axis onto the port's at the boundary
// and flag any RNG desync.  Emitted on EVERY entry (the flip disambiguates
// re-entries in a title loop); the port emits the matching names from
// enter_newgame / enter_prologue.
const SCENE_ANCHORS = [
    { va: 0x564780, name: 'newgame_enter' },
    { va: 0x56cd20, name: 'prologue_enter' },
];
let g_scene_anchors_hooked = false;
function installSceneAnchors() {
    if (g_scene_anchors_hooked) return;
    const seedAddr = rva(SEED_VA);
    SCENE_ANCHORS.forEach(function (a) {
        Interceptor.attach(rva(a.va), {
            onEnter: function () {
                let seed = 0;
                try { seed = seedAddr.readU32() >>> 0; } catch (e) {}
                send({kind: 'anchor', name: a.name, frame: g_flip_frame,
                      rng: seed});
                // rand-probe window: arm at newgame_enter, flush at prologue_enter.
                if (g_rand_probe) {
                    if (a.name === 'newgame_enter') {
                        g_rand_window_active = true;
                        g_rand_callers = {}; g_rand_total = 0;
                    } else if (a.name === 'prologue_enter') {
                        g_rand_window_active = false;
                        send({kind: 'rand_window', from: 'newgame_enter',
                              to: 'prologue_enter', total: g_rand_total,
                              callers: g_rand_callers});
                    }
                }
            },
        });
    });
    g_scene_anchors_hooked = true;
    logmsg('scene anchors installed @ ' +
           SCENE_ANCHORS.map(function (a) {
               return a.name + '(0x' + a.va.toString(16) + ')'; }).join(', '));
}

function memWatchFlush(frameNumber) {
    if (g_mem_watch_buffer.length === 0) return;
    const events = g_mem_watch_buffer;
    g_mem_watch_buffer = [];
    send({kind:   'mem_access_batch',
          frame:  frameNumber,
          count:  events.length,
          events: events});
}

// Whether a trapped access landed inside the recorded extent of region
// `idx` (vs. a page neighbor that merely shares the 4KiB page).
function memWatchInRegion(idx, addrPtr) {
    const r = g_mem_watch_regions[idx];
    if (!r) return false;
    const base = rva(r.va);
    return addrPtr.compare(base) >= 0 &&
           addrPtr.compare(base.add(r.size)) < 0;
}

// (Re-)arm MemoryAccessMonitor over the cached ranges.  Idempotent —
// MemoryAccessMonitor.enable() replaces any prior monitor, which is
// exactly the re-arm we want after a page's one-shot fires.
function memWatchArm() {
    MemoryAccessMonitor.enable(g_mem_watch_ranges, {onAccess: memWatchOnAccess});
}

function memWatchOnAccess(details) {
    const idx = details.rangeIndex | 0;
    const region = g_mem_watch_regions[idx] || {};
    const inRegion = memWatchInRegion(idx, details.address);

    if (g_mem_watch_precise && !inRegion) {
        // Page neighbor consumed this page's one-shot.  Re-arm and keep
        // hunting, unless the page is so hot we've burned the budget.
        g_mem_watch_neighbor++;
        if (g_mem_watch_rearm < MEM_WATCH_REARM_CAP) {
            g_mem_watch_rearm++;
            try { memWatchArm(); }
            catch (e) { err('memWatchArm', e.message); }
        } else if (g_mem_watch_rearm === MEM_WATCH_REARM_CAP) {
            g_mem_watch_rearm++;   // log-once sentinel
            log_rearm_exhausted(region, idx);
        }
        return;
    }

    g_mem_watch_n++;
    g_mem_watch_buffer.push({
        op:     details.operation,           // read | write | execute
        from:   toGhidraVa(details.from),    // faulting insn VA
        addr:   toGhidraVa(details.address), // accessed data VA
        region: idx,
        label:  region.label || '',
        ts:     nowMs(),
    });
    if (g_mem_watch_buffer.length >= MEM_WATCH_FLUSH_AT) {
        memWatchFlush(g_flip_frame);
    }
    // Keep watching so additional distinct writers of the same field
    // surface, up to a sane cap.
    if (g_mem_watch_precise &&
        g_mem_watch_n < MEM_WATCH_MAX_HITS &&
        g_mem_watch_rearm < MEM_WATCH_REARM_CAP) {
        g_mem_watch_rearm++;
        try { memWatchArm(); }
        catch (e) { err('memWatchArm', e.message); }
    }
}

function log_rearm_exhausted(region, idx) {
    logmsg('mem_watch: re-arm budget (' + MEM_WATCH_REARM_CAP +
           ') exhausted on a hot page for region "' +
           (region.label || idx) + '" before any in-region write — ' +
           'consider a narrower region.');
}

function installMemoryWatch(regions, precise) {
    g_mem_watch_regions = (regions || []).map(function (r) {
        return {
            va:     r.va | 0,
            size:   (r.size | 0) || 16,
            label:  String(r.label || ('0x' + (r.va >>> 0).toString(16))),
            access: (r.access === 'rw') ? 'rw' : 'w',
        };
    });
    if (g_mem_watch_regions.length === 0) {
        err('installMemoryWatch', 'no regions given');
        return false;
    }
    g_mem_watch_precise  = (precise !== false);
    g_mem_watch_n        = 0;
    g_mem_watch_neighbor = 0;
    g_mem_watch_rearm    = 0;
    g_mem_watch_ranges   = g_mem_watch_regions.map(function (r) {
        return {base: rva(r.va), size: r.size};
    });

    try {
        memWatchArm();
    } catch (e) {
        err('installMemoryWatch', e.message + ' ' + (e.stack || ''));
        return false;
    }

    g_mem_watch_enabled = true;
    logmsg('mem_watch: armed ' + g_mem_watch_regions.length + ' region(s) [' +
           (g_mem_watch_precise ? 'precise' : 'raw') + ']: ' +
           g_mem_watch_regions.map(function (r) {
               return r.label + '@0x' + (r.va >>> 0).toString(16) +
                      '+' + r.size + '(' + r.access + ')';
           }).join(', '));
    send({kind:    'mem_watch_ready',
          precise: g_mem_watch_precise,
          regions: g_mem_watch_regions.map(function (r) {
              return {va: r.va, size: r.size, label: r.label,
                      access: r.access};
          })});
    return true;
}

function findOurModule() {
    // Prefer Process.mainModule (Frida 16+) — this is the executable
    // that started the process, regardless of its on-disk filename.
    // run-retail.sh drops the unpacked exe at sotes-unpacked-<pid>.exe
    // so name-matching against MODULE_NAME wouldn't find it.
    if (Process.mainModule) return Process.mainModule;

    // Fallback for older Frida: name-match or just take module 0.
    const mods = Process.enumerateModules();
    for (let i = 0; i < mods.length; i++) {
        const n = mods[i].name.toLowerCase();
        if (n === MODULE_NAME.toLowerCase()) return mods[i];
        // Heuristic: any module that contains "sotes" in its name is us
        // (covers sotes.exe, sotes-unpacked-NNN.exe).  Excludes
        // sotesp.dll / sotesd.dll / sotesw.dll which have a suffix
        // before .dll, not before .exe.
        if (n.endsWith('.exe') && n.indexOf('sotes') !== -1) return mods[i];
    }
    return mods[0] || null;
}

function withModule(callback) {
    // Frida is attached at the entry-point of the spawned process; the
    // module is always already loaded by the time the agent runs.  But
    // be defensive: poll a few times before giving up.
    let mod = findOurModule();
    if (mod) { callback(mod); return; }
    let tries = 0;
    const iv = setInterval(function () {
        mod = findOurModule();
        if (mod || tries++ > 50) {
            clearInterval(iv);
            if (mod) callback(mod);
            else err('withModule', 'main module never appeared');
        }
    }, 20);
}

// ─── init RPC ──────────────────────────────────────────────────────────

rpc.exports = {
    init: function (opts) {
        opts = opts || {};
        g_hide_window         = !!opts.hide_window;
        g_turbo_enabled       = !!opts.turbo;
        g_silent_audio_enabled = !!opts.silent_audio;
        g_cursor_probe         = !!opts.cursor_probe;
        g_fade_probe           = !!opts.fade_probe;
        g_pace_probe           = !!opts.pace_probe;
        g_textout_probe        = !!opts.textout_probe;
        if (typeof opts.textout_lo === 'number') g_textout_lo = opts.textout_lo | 0;
        if (typeof opts.textout_hi === 'number' && opts.textout_hi > 0)
            g_textout_hi = opts.textout_hi | 0;
        g_box_probe            = !!opts.box_probe;
        if (typeof opts.box_lo === 'number') g_box_lo = opts.box_lo | 0;
        if (typeof opts.box_hi === 'number' && opts.box_hi > 0)
            g_box_hi = opts.box_hi | 0;
        g_seed_pin             = !!opts.seed_pin;
        if (typeof opts.seed_value === 'number') g_seed_value = opts.seed_value >>> 0;
        if (typeof opts.pace_every === 'number' && opts.pace_every > 0)
            g_pace_every = opts.pace_every | 0;
        if (typeof opts.turbo_step_ms === 'number') g_turbo_step_ms = opts.turbo_step_ms;
        g_rand_probe           = !!opts.rand_probe;
        g_lockstep             = !!opts.lockstep;
        if (typeof opts.lockstep_step_ms === 'number' && opts.lockstep_step_ms > 0)
            g_lockstep_step_ms = opts.lockstep_step_ms | 0;
        if (typeof opts.lockstep_epsilon_ms === 'number' && opts.lockstep_epsilon_ms >= 0)
            g_lockstep_epsilon_ms = opts.lockstep_epsilon_ms | 0;
        // msgbox redirect default ON — pass {msgbox_redirect:false} to
        // see real popups (debugging the harness itself).
        if (typeof opts.msgbox_redirect === 'boolean') g_msgbox_redirect = opts.msgbox_redirect;
        if (typeof opts.auto_click_launch  === 'boolean') g_auto_click_launch  = opts.auto_click_launch;
        if (typeof opts.auto_disable_sound === 'boolean') g_auto_disable_sound = opts.auto_disable_sound;
        if (typeof opts.force_windowed     === 'boolean') g_force_windowed     = opts.force_windowed;

        // Structural-parity harness modes (off unless explicitly enabled).
        if (opts.call_trace) {
            g_call_trace_enabled = true;
            g_call_trace_vas = Array.isArray(opts.call_trace_vas)
                ? opts.call_trace_vas.slice() : [];
            g_call_trace_frames =
                (Array.isArray(opts.call_trace_frames) && opts.call_trace_frames.length)
                    ? new Set(opts.call_trace_frames) : null;
        }
        if (opts.capture_frames_enabled) {
            g_capture_enabled = true;
            g_capture_frames =
                (Array.isArray(opts.capture_frames) && opts.capture_frames.length)
                    ? new Set(opts.capture_frames) : null;
        }
        if (opts.input_inject_enabled && Array.isArray(opts.input_trace)) {
            g_inject_enabled = true;
            g_inject_trace = opts.input_trace
                .map(function (e) { return { frame: e.frame | 0, ids: e.ids || [] }; })
                .sort(function (a, b) { return a.frame - b.frame; });
            // Opt-in poll/latch debug window around the LAST scripted press
            // (so probes of a deep sub-menu are covered, not just the first).
            if (opts.inject_debug && g_inject_trace.length > 0) {
                const last = g_inject_trace[g_inject_trace.length - 1].frame;
                g_poll_dbg_lo = last - 1;
                g_poll_dbg_hi = last + 3;
            }
        }

        withModule(function (mod) {
            g_base = mod.base;
            send({
                kind: 'ready',
                module: mod.name,
                base: mod.base.toString(),
                fridaVersion: Frida.version,
            });

            try { installMessageBoxRedirect(); } catch (e) { err('install_msgbox', '' + e); }
            try { installHwndOwnershipTracking(); } catch (e) { err('install_hwnd', '' + e); }
            try { installHideWindowHook(); } catch (e) { err('install_hide', '' + e); }
            try { installMessagePumpCounter(); } catch (e) { err('install_pump', '' + e); }
            try { installTurboHooks(); } catch (e) { err('install_turbo', '' + e); }
            try { installSilentAudioHooks(); } catch (e) { err('install_audio', '' + e); }
            try { installDialogBypass(); } catch (e) { err('install_dlg_bypass', '' + e); }
            try { installPeriodicWindowScan(); } catch (e) { err('install_window_scan', '' + e); }

            // Structural-parity harness: frame anchor + call-trace + mem-watch
            // + frame capture + input injection all key off the Flip frame.
            if (g_call_trace_enabled || opts.mem_watch || g_capture_enabled ||
                g_inject_enabled || g_cursor_probe || g_fade_probe ||
                g_pace_probe || g_textout_probe || g_box_probe || g_lockstep) {
                try { installFlipFrameHook(); } catch (e) { err('install_flip', '' + e); }
            }
            if (g_cursor_probe) {
                try { installCursorProbe(); }
                catch (e) { err('install_cursor_probe', '' + e); }
            }
            if (g_fade_probe) {
                try { installFadeProbe(); }
                catch (e) { err('install_fade_probe', '' + e); }
            }
            if (g_textout_probe) {
                try { installTextOutProbe(); }
                catch (e) { err('install_textout_probe', '' + e); }
            }
            if (g_box_probe) {
                try { installBoxProbe(); }
                catch (e) { err('install_box_probe', '' + e); }
            }
            // Sparkle anchor (subtitle-anim-start TAS marker + optional seed
            // pin) — install whenever we have a flip-frame counter to stamp it,
            // or when seed-pinning is requested.
            if (g_seed_pin || g_capture_enabled || g_pace_probe ||
                g_fade_probe || g_cursor_probe || g_inject_enabled || g_lockstep) {
                try { installSparkleAnchor(); }
                catch (e) { err('install_sparkle_anchor', '' + e); }
            }
            // Scene-boundary anchors ride the same flip counter as the sparkle
            // anchor; install whenever the flip hook is live.
            if (g_capture_enabled || g_inject_enabled || g_lockstep ||
                g_call_trace_enabled) {
                try { installSceneAnchors(); }
                catch (e) { err('install_scene_anchors', '' + e); }
            }
            if (g_rand_probe) {
                try { installRandProbe(); }
                catch (e) { err('install_rand_probe', '' + e); }
            }
            if (g_inject_enabled) {
                try { installInputInjection(); }
                catch (e) { err('install_inject', '' + e); }
            }
            if (g_call_trace_enabled && g_call_trace_vas.length > 0) {
                try { installCallTraceHooks(g_call_trace_vas); }
                catch (e) { err('install_call_trace', '' + e); }
            }
            if (opts.mem_watch) {
                try { installMemoryWatch(opts.mem_watch_regions,
                                         opts.mem_watch_precise !== false); }
                catch (e) { err('install_mem_watch', '' + e); }
            }

            // Force the Interceptor to commit every attach/replace we
            // queued.  Without this, attaches done while the process is
            // suspended can sit in a deferred state and never trigger.
            // Discovered the hard way in the first retail smoke run.
            try { Interceptor.flush(); logmsg('Interceptor.flush ok'); }
            catch (e) { err('flush', '' + e); }
        });
    },
};
