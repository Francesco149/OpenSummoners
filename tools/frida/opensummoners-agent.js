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
const g_seen_hwnd_keys = {};   // dedupe: emit once per (hwnd, title) pair

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
    ['PeekMessageA', 'PeekMessageW', 'GetMessageA', 'GetMessageW'].forEach(function (name) {
        const p = resolveExport('user32.dll', name);
        if (!p) return;
        Interceptor.attach(p, {
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

    // Sleep → 0 ms.  Both kernel32!Sleep and the engine's Sleep-equivalent
    // any winmm-backed timer.  Same shape as OpenMare's turbo v2.
    const sleep = resolveExport('kernel32.dll', 'Sleep');
    if (sleep) {
        Interceptor.replace(sleep, new NativeCallback(function (ms) {
            // No-op.  The engine immediately re-enters the loop.
        }, 'void', ['uint']));
        logmsg('Interceptor.replace Sleep (no-op)');
    }

    // Virtualised timeGetTime.  When the engine reads "current time" to
    // compute a frame delta, return our monotonically bumped counter.
    // Pinning to the main thread avoids accidentally lying to background
    // threads (audio mixer, file I/O) that might use the same API and
    // depend on real wall-clock.
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

            // Detect main game window appearance → back off scan rate.
            if (!g_main_game_window_seen && cls === 'CLASS_LIZSOFT_SOTES') {
                g_main_game_window_seen = true;
                if (g_window_scan_handle !== null) {
                    clearInterval(g_window_scan_handle);
                    g_window_scan_handle = setInterval(scan, WINDOW_SCAN_SLOW_MS);
                    logmsg('main window appeared — scan rate → ' +
                           WINDOW_SCAN_SLOW_MS + ' ms');
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
        if (typeof opts.turbo_step_ms === 'number') g_turbo_step_ms = opts.turbo_step_ms;
        // msgbox redirect default ON — pass {msgbox_redirect:false} to
        // see real popups (debugging the harness itself).
        if (typeof opts.msgbox_redirect === 'boolean') g_msgbox_redirect = opts.msgbox_redirect;
        if (typeof opts.auto_click_launch  === 'boolean') g_auto_click_launch  = opts.auto_click_launch;
        if (typeof opts.auto_disable_sound === 'boolean') g_auto_disable_sound = opts.auto_disable_sound;
        if (typeof opts.force_windowed     === 'boolean') g_force_windowed     = opts.force_windowed;

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

            // Force the Interceptor to commit every attach/replace we
            // queued.  Without this, attaches done while the process is
            // suspended can sit in a deferred state and never trigger.
            // Discovered the hard way in the first retail smoke run.
            try { Interceptor.flush(); logmsg('Interceptor.flush ok'); }
            catch (e) { err('flush', '' + e); }
        });
    },
};
