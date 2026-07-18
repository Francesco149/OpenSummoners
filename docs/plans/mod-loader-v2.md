# Universal Mod Loader + Mod API — design scope (v2)

**North star: MAX STABILITY.** A buggy or careless mod must not be able to crash the game; two mods
hooking the same function must not clobber each other; nothing touches engine state off the main
thread.  Everything below is subordinate to that.

**Goal:** a game-agnostic native loader that hosts (a) a **Lua** modding API — hook any function
safely (even when multiple mods hook it), read/write memory, call engine functions from the main
thread, plus the usual primitives; (b) **optional native DLL mods** that route hooks through the same
registry; and (c) a **shared ImGui UI** each mod adds panels to (and can pop its own window).  Then
rework the **voice patch** and the **trainer** onto it.  **Scope this session; build next.**

## Why now / what it fixes
- The voice-patch fullscreen bug (`findings/ense-voice-monster-se-drop.md`) was a **worker-thread seed
  racing the engine's single-threaded boot** — i.e. the #1 rule (*all engine interaction on the MAIN
  thread*) violated by hand.  A framework that makes the main-thread path the *default* eliminates a
  whole bug class.
- USER's coexistence concern: the voice patch + trainer both want the main thread + both hook engine
  code; today each hand-rolls its own detour and they can clobber a shared 5-byte patch.  A **managed,
  chained hook registry** removes that.
- Consolidate two ad-hoc mods (trainer, voice) onto one hardened base.

## Build on what already works (don't reinvent — generalize)
- **`tools/mod_loader`** — the generic proxy `version.dll` that forwards to `realver.dll` and
  `LoadLibrary`s `mods\*.dll`.  → becomes the **loader host** (add the Lua runtime + registries + UI).
- **`tools/sotes_trainer`** — the PROVEN stability patterns to lift wholesale: the engine-thread
  **safepoint queue** (`eq_*`/`drain_engine_queue`), **MinHook entry-thunks** (calling-convention-
  agnostic, SEH-safe), **guarded memory** (`mem_readable`/`mem_writable` via VirtualQuery),
  **scene-state gating** (`scene_present()` + `g_scene_settle`), the **crash-resilient trace**
  (flush-to-file + safepoint marks), and the **ImGui/DX11** window.
- **`tools/ennse_voice`** — the **WndProc-bounce main-thread bootstrap** (just shipped): the safe,
  game-agnostic way to get onto the engine thread with no engine VA.

## Hard-won stability lessons → enforced invariants
1. **No engine calls / heap allocs off the main thread.**  (The voice bug.)  → every engine call, poke,
   and alloc runs on the safepoint queue; worker/UI threads only READ (guarded) + ENQUEUE.
2. **Don't hook teardown/rebuild/boot-init.**  (`door-gate.md`; render-root free+realloc; input-device
   activation.)  → hooks install/remove **on the safepoint**, **gated** on a stable scene state; the
   game profile lists never-hook VAs + the boot/teardown windows.
3. **Even correct hooks can create inconsistent GAME state** (warpgate broke scripted sequences →
   crash).  → the loader exposes scene-state signals (settle, scripted/cutscene flags) so a mod can
   self-gate; a mod bypass that corrupts story state is the mod's bug, but the loader makes the signals
   available.
4. **Never fault on a bad read/write** → all memory access VirtualQuery-guarded; expensive scans
   throttled.
5. **Survive crashes for post-mortem** → crash-resilient flush-to-file log + internal safepoint marks,
   built in.
6. **Single instance, clean unload** → the loader refuses a double-load, restores every hook's original
   bytes on unload, and reaps cleanly.
7. **Exception isolation (the multi-mod keystone)** → every mod callback (Lua or native, hook or UI) runs
   inside an SEH/`pcall` boundary.  A faulting hook is **disabled + logged**, not propagated; a Lua error
   is caught + the mod flagged.  One bad mod never takes down the game or the other mods.

## Architecture (layers, bottom→top)
```
 version.dll proxy (host)  ── forwards version.* → realver.dll; single-instance; crash log
   ├─ Memory service       ── guarded r/w, AOB scan, module base + ASLR reloc, struct/cdef
   ├─ Main-thread executor ── WndProc bootstrap → engine-safepoint hook → job queue + futures
   ├─ Hook registry        ── ONE trampoline per target VA → central dispatcher → ordered mod chain
   │                          (Tier-1 entry observers | Tier-2 typed pre/post/replace)
   ├─ Native-mod bridge    ── stable C ABI so mods\*.dll register hooks/UI in the SAME registries
   ├─ Lua runtime          ── the scripting front-end onto all of the above (mods\*/init.lua)
   ├─ UI host              ── one ImGui context (own DX11 window); mods add panels / own windows
   └─ Game profiles        ── per-game facts (safepoint VA, window class, key structs, no-hook list)
```

## Hooking design (the crux of "safe even with multiple mods")
- **One trampoline per hooked VA** (MinHook backend, proven).  The trampoline → the loader's **central
  dispatcher**, which walks an **ordered, per-owner chain** of that VA's registered hooks.  Add/remove
  = editing the chain, never re-patching bytes → **no two mods fight over the 5 bytes**.
- **Two tiers** (stability gradient):
  - **Tier 1 — entry observers (DEFAULT, ultra-stable):** register-capture thunk (`{ecx,edx,esp,
    ret_addr, arg[0..N]}`), the trainer's model.  No signature needed, calling-convention-agnostic,
    can't modify/block → a Tier-1 mod *cannot* destabilize the callee.  Covers ~most needs (observe,
    read args, trigger reactions).
  - **Tier 2 — typed hooks (opt-in, powerful):** mod declares the C signature; **libffi** (or LuaJIT
    FFI closures) marshals so a **pre** hook can modify args / **block** + return a value, and a
    **post** hook can modify the return.  Chain order defined; first-to-block wins + logged.
- **Chain read path is lock-free** (snapshot/RCU): hot calls never take a lock; mutations happen on the
  safepoint under a lock.  **Safe unload:** a hook is removed only when no call to it is in flight
  (quiescent-state on the safepoint), then the trampoline is torn down if the chain is empty.
- **Reentrancy-safe** (a hook may call the original; the callee may recurse).

## Main-thread executor
- **Bootstrap = WndProc subclass** on the game window (the just-shipped voice-patch pattern): guaranteed
  main thread, **no engine VA**, game-agnostic.  From the first message we're safely on the engine
  thread → install the per-frame safepoint hook + start draining.
- **Per-frame safepoint** = a Tier-1 hook on the profile's designated per-frame fn (SotES: `0x437c70`).
  The executor **drains the job queue** there each frame (the trainer's `drain_engine_queue`).
- **Lua:** `mod.main(fn)` runs `fn` on the main thread at the next safepoint, returns its result
  (await or fire-and-forget); `mod.on_frame(fn)` registers a per-frame callback.  All engine calls
  inside are safe by construction.

## Memory + FFI
- `mod.mem.read/write` typed (u8..u64, f32/f64, bytes, cstr), **always VirtualQuery-guarded**.
- `mod.scan(pattern)` AOB; `mod.module(name)` base; VA reloc by ASLR delta (0 live here, but general).
- **Struct model:** LuaJIT `ffi.cdef` engine structs → cast a pointer to a typed view (`this.field`),
  the readability win the trainer gets from Ghidra tagging, now at runtime.

## Lua API surface (sketch — refine while building)
```
mod.log(...)                     mod.version, mod.gamedir
mod.mem.{read,write,scan,module} mod.reloc(va)
mod.hook.entry(va, cb)           -- Tier 1 (observe)         → handle
mod.hook.typed(va, sig, {pre=,post=})  -- Tier 2            → handle
mod.hook.remove(handle)
mod.main(fn) / mod.on_frame(fn)  -- main-thread exec
mod.call(va, sig, ...)           -- FFI call (wrap in mod.main if it touches the scene)
mod.dll.load(path) / mod.dll.proc(h, name)   -- native bridge from Lua
mod.ui.panel(name, draw)         mod.ui.window(name, draw, opts)
mod.game.*                       -- the loaded game profile (safepoint, structs, flags)
mod.on.{scene_change, settle, unload, ...}   -- lifecycle events
```

## Native-mod bridge
- A stable **C ABI** (a vtable/struct of function pointers the loader hands a mod's `OssModInit`
  entry): `hook_entry`, `hook_typed`, `mem_*`, `main_enqueue`, `ui_panel`, `log`, `game`.
- `mods\<name>.dll` exports `OssModInit(const OssApi*)`; it registers into the **same** hook + UI
  registries as Lua mods → native + Lua mods interleave safely.  (This is how a perf-critical hook or
  a big feature ships native while trivial mods stay Lua.)

## ImGui UI host
**Two loader-MANAGED ImGui instances** — a mod never creates its own context, so mod UIs *and* overlays
can't clash at the ImGui level:

- **(1) Main UI — a separate loader-owned ImGui window** (its own DX11 device; the trainer's proven,
  STABLE model — no hooking the game's renderer).  This is the DEFAULT UI path.
  - `mod.ui.panel(name, draw)` → a collapsible section/tab in the loader window.
  - `mod.ui.window(name, draw, opts)` → the mod owns a floating ImGui window / OS viewport (the
    trainer's **map-graph window**).

- **(2) Overlay — a SECOND loader-managed ImGui instance drawn ON TOP of the game** (opt-in, NEVER the
  default; a mod chooses to draw over the game).  Because the loader owns this instance too, multiple
  mods' overlays composite without clashing at the context level — at two safety levels:
  - **Managed info panels (safe):** `mod.overlay.panel(name, draw)` → a flowing, TOGGLABLE panel the
    loader lays out over the game (like the main UI, but on the game surface); the loader arranges them
    so they don't collide.
  - **Freehand draw (riskier, opt-in):** `mod.overlay.draw(fn)` → the mod draws arbitrary primitives
    anywhere on screen (world markers, hit-boxes, lines).  Powerful, but the loader CANNOT guarantee two
    mods' freehand layers won't visually overlap — explicitly the mod's own risk.
  - Overlay rendering on **DDraw7** (compositing ImGui over the primary surface) is the fragile part →
    built as its OWN backend AFTER the main UI is solid; the `mod.overlay.*` API is backend-agnostic.

- **UI thread = read-only + enqueue** (the trainer's rule): draw callbacks — window OR overlay — may
  READ game memory (guarded) but any write/engine-call goes through `mod.main`.  Enforced + documented.

## Game profiles
- Per-game facts in `profiles/<exe>.lua`, auto-selected by the host exe: the per-frame **safepoint VA**,
  **window class**, **no-hook / boot-teardown** windows, and shared **struct cdefs** + key globals.
- Keeps the core game-agnostic and all SotES knowledge in one reusable place (sibling projects
  openrecet / OpenMare can drop in their own profile).

## Mod format + load
- `mods\<name>\init.lua` (Lua mod) and/or `mods\<name>.dll` (native mod); optional `mod.toml`
  (name, load order, deps, target game).  The host scans `mods\`, orders by deps, loads each inside an
  isolation boundary (invariant #7).  Hot-reload of Lua mods = a nice-to-have (unregister → reload).

## Migration (careful — don't break installed users)
1. **Voice patch first.**  Rework `ennse_voice` as a mod (Lua script using `mod.main` for the seed — it
   dogfoods the API and the seed is ~10 lines) under the new loader.  **Test the FULL install→launch→
   voice-plays→fullscreen-mob-SFX flow to 100% before touching the release.**  Keep the *current*
   shipping patch (this repo's `version.dll` + `ennse_voice.dll`) live and unchanged until the new one
   is proven, so the one-liner (`web-install.ps1`) never serves a broken build.  Swap the release only
   after green.
2. **Trainer second.**  Rework `sotes_trainer` onto the API (hooks via the registry, `mod.main`,
   `mod.mem`, `mod.ui.panel` + the map-graph `mod.ui.window`).  This also *verifies coexistence*
   (voice + trainer as two mods sharing the registry — the concern that motivated all this).

## Decisions (LOCKED 2026-07-18, USER)
- **Lua runtime: LuaJIT** — **JIT OFF** (interpreter-only, for stability) + **FFI ON** (native calls +
  struct `cdef`s).  Typed-hook marshaling uses **LuaJIT FFI closures** (libffi only as a fallback for a
  native-only build).
- **UI: a separate loader-owned ImGui window** as the DEFAULT path.  Overlays = a SECOND
  loader-managed ImGui instance mods OPT INTO (never default): a safe managed togglable-panel layer
  (`mod.overlay.panel`, loader-arranged so mods don't collide) + a riskier opt-in freehand layer
  (`mod.overlay.draw`, no cross-mod clash guarantee); built as a later DDraw7 backend behind the
  backend-agnostic `mod.overlay.*` API.
- **Hooks: entry-observer default + typed (opt-in)** — the stability gradient.
- **Voice patch → a Lua mod** (dogfoods `mod.main`; ~10-line main-thread seed).
- Still open (pick while building): the loader name + Lua namespace (`mod.*` placeholder).

## Phased build plan (next session+)
- **P0 Scaffold:** loader hosts LuaJIT, scans `mods\*`, `mod.log`, hello-world mod boots.
- **P1 Memory+FFI:** guarded r/w, scan, module/reloc, struct cdefs.
- **P2 Main-thread executor:** WndProc bootstrap → safepoint hook → queue + `mod.main`/`on_frame`; SotES profile.
- **P3 Hook registry:** Tier-1 chain (MinHook + dispatcher, multi-mod), then Tier-2 typed (libffi); safepoint-gated install/remove.
- **P4 Native bridge:** C ABI, shared registry, `OssModInit`.
- **P5 UI host:** DX11 window, `ui.panel`/`ui.window`, UI-thread model.
- **P6 Voice mod:** port + **exhaustive install/launch test** → swap the release.
- **P7 Trainer mod:** port + map-graph window; coexistence verified.

Each phase independently testable; P6 gates on a 100%-green install flow before any push.
