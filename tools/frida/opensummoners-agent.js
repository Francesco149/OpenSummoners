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
// Period: every ~200 ms while the process is in launcher-dialog state.
const WINDOW_SCAN_INTERVAL_MS = 200;
let   g_window_scan_started = false;
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
    // ours and only force-hide those.  Plain Attach onLeave instead of
    // a full replace — we don't want to perturb the return value.
    ['CreateWindowExA', 'CreateWindowExW'].forEach(function (name) {
        const p = resolveExport('user32.dll', name);
        if (!p) return;
        Interceptor.attach(p, {
            onLeave: function (retval) {
                if (retval.isNull()) return;
                const key = retval.toString();
                g_owned_hwnds[key] = { source: name };
                send({ kind: 'hwnd_owned', hwnd: key, source: name });
            }
        });
        logmsg('attached ' + name);
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
function installPeriodicWindowScan() {
    if (g_window_scan_started) return;
    const u32 = Process.findModuleByName('user32.dll');
    if (!u32) { err('window_scan', 'user32.dll not loaded'); return; }

    const enumThreadWindows  = u32.findExportByName('EnumThreadWindows');
    const enumWindows        = u32.findExportByName('EnumWindows');
    const getWindowThreadPid = u32.findExportByName('GetWindowThreadProcessId');
    const getClassNameA      = u32.findExportByName('GetClassNameA');
    const getWindowTextA     = u32.findExportByName('GetWindowTextA');
    const isWindowVisible    = u32.findExportByName('IsWindowVisible');
    if (!enumWindows || !getWindowThreadPid || !getClassNameA ||
        !getWindowTextA || !isWindowVisible) {
        err('window_scan', 'one of the required user32 exports is missing');
        return;
    }

    const EnumWindows = new NativeFunction(enumWindows,
        'int', ['pointer', 'long']);
    const GetWindowThreadProcessId = new NativeFunction(getWindowThreadPid,
        'uint32', ['pointer', 'pointer']);
    const GetClassNameA  = new NativeFunction(getClassNameA,
        'int', ['pointer', 'pointer', 'int']);
    const GetWindowTextA = new NativeFunction(getWindowTextA,
        'int', ['pointer', 'pointer', 'int']);
    const IsWindowVisible = new NativeFunction(isWindowVisible,
        'int', ['pointer']);

    const ourPid = Process.id;
    const pidBuf = Memory.alloc(4);
    const cnBuf  = Memory.alloc(256);
    const wtBuf  = Memory.alloc(512);

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
            if (g_seen_hwnd_keys[key]) return 1;
            g_seen_hwnd_keys[key] = true;

            send({ kind: 'hwnd_seen', hwnd: hwnd.toString(),
                   cls: cls, title: title, visible: visible });

            // Belt-and-braces: also force-hide if the user asked for it.
            // Our CreateWindowEx onLeave hook should already have caught
            // this HWND, but if the dialog was created via a path we
            // didn't see, the periodic ShowWindow(SW_HIDE) handles it.
            if (g_hide_window && visible) {
                const sw = Process.findModuleByName('user32.dll').findExportByName('ShowWindow');
                if (sw) {
                    new NativeFunction(sw, 'int', ['pointer', 'int'])(hwnd, 0);
                    send({ kind: 'hide_force', api: 'periodic-scan',
                           hwnd: hwnd.toString(), original: 1 });
                }
                g_owned_hwnds[hwnd.toString()] = { source: 'periodic-scan' };
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
    setInterval(scan, WINDOW_SCAN_INTERVAL_MS);
    g_window_scan_started = true;
    logmsg('installed periodic window scan (every ' + WINDOW_SCAN_INTERVAL_MS + ' ms)');
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
