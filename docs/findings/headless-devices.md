# rhost headless devices, flip coroutine, savestates, command server (2026-11-13)

How SotES became an L2 target (step + snapshot + search) for agent-driven play — openrecet's method
(`/opt/src/openrecet/docs/findings/headless-savestates.md`) on this engine. Tool reference: `tools/drive/README.md`;
agent card `tools/drive/DRIVER.md`.

## 1. Device census (`rhost --dev-census`)

Per-slot call counter in every rh_scrub thunk (IAT import / COM vtable slot) + argument shapes. Windows real device,
rh-combat + rh-my-play + rh-water-shrine (~220k frames):
- **DirectDraw7 = a sysmem blitter only.** Every surface OFFSCREENPLAIN|SYSTEMMEMORY (caps 0x840), 16-bpp 565, explicit
  format. No primary, no Flip, no palette, no BltFast, no GetAttachedSurface. Surface methods called: Blt (32M), Lock /
  Unlock whole-surface DDLOCK_WAIT (4.2M), GetDC / ReleaseDC (212k, all on 640x480), SetColorKey DDCKEY_SRCBLT 0xF81F
  (55k), SetClipper, GetSurfaceDesc. Clipper: SetClipList (not SetHWnd). DD7: CreateSurface (~1/frame: a temp surface),
  CreateClipper, SetCooperativeLevel.
- **Blt flags:** only DDBLT_WAIT or DDBLT_WAIT|DDBLT_KEYSRC; never stretched, colour-filled, self. Only failure:
  DDERR_INVALIDRECT on zero-width rects (32k); out-of-surface rects succeed (clip list).
- **Lock pitch** = round_up(w*2, 8); desc flags 0x100f (+0x10000 DDSD_CKSRCBLT once keyed).
- **GetDC** returns a DEFAULT-state DC every time (211,677/211,677: bk OPAQUE, text black, bk white, stock font + pen,
  align 0; never the same HDC twice).
- **Present = GDI**: GetDC(window) + BitBlt once a frame. Text = GDI TextOutA (~71/frame, one per glyph/word).
- **DirectShow BGM:** RenderFile, Run/Pause/Stop, GetPositions per frame, SetPositions, GetDuration, put_Volume.
  GetDuration = ASF File Properties PlayDuration − Preroll·10⁴ (9/9 tracks equal quartz).
- **Host leaks into retail:** GetClientRect(desktop) 1920x1080, GetClientRect(quiet game window) 634x451.

## 2. Headless devices (`rh_dev.c`, `--headless`)

Fakes for exactly that surface; method tables from mingw headers (`gen_dev.py`: counting S_OK stub per unwritten method,
signature asserts). State in the DEV region 0xE8000000..0xF4000000 (free on Windows x64 + wine wow64; wine holds
0xb0000000..0xe8000000). Large surfaces (≥128 KB, DIB-compatible pitch) live in **section slots** (0xF4000000 + i·640 KB:
a page-file section mapped at a fixed address, GDI's DIB section over the same section → GetDC with zero copies).
Message-only game window; GetClientRect pinned to the reference values; the window DC = a 1x1 sink.

**Evidence** (Windows, `corpus --check --extra=--headless` vs real-device expected.json, 2026-11-13): frame, tick, RNG
and **fb (every presented frame's pixels) equal on 13/15**; state_sem differs on all 15 (fake device pointers stored in
retail memory — not yet named), fb differs on rh-after-prologue and rh-se-newgame (SE edition). Causes (the fakes' own
UNSUPPORTED log): the heap-surface DC cache never freed (first divergence f62834 = first exhaustion), SE's 24-bpp
surfaces. Fixed (4724cb2): **re-gate 2026-11-13: frame / tick / RNG / fb equal on 15/15**; only state_sem differs.
Wine without X: boot f1200 rng 404a0a8f = Xvfb run; 529–611 fps vs 280 (Xvfb), no exit hang.

## 3. Flip coroutine (rhost.c)

The flip stub (`pushad; pushfd; call flip_swap; popfd; popad; jmp flip`, all 50 sites) switches to a host stack;
retail's whole context stays on the game stack (esp → g_gesp). The host runs the boundary work (`boundary()` = old
pre_flip) and, with `--serve`, the command loop; `rh_step()` resumes retail to its next Flip. Host code never runs on
the game stack. Dead game stack below g_gesp zeroed (`rh_scrub_below`). Real-device runs with it reproduce the blessed
state hashes (rh-boot-title, rh-se-newgame, rh-after-prologue: 3/3; full corpus: see PROGRESS).
Frame convention: a session at frame F has completed F flips; `--frames N` stops at flip #N (N+1 flips).

## 4. Savestates (`rh_snap.c`)

openrecet's tracker (anchor = base copy + write protect; dirty list; take/restore O(dirty)). Tracked: retail .data/.bss,
arena (+VREG commit/decommit hooks), DSound pool, fixed host-object page, DEV, mapped DIB slots. Host state: clocks,
frame/ticks, fb chain, g_gesp + game TIB, trace cursor (forward-only: missing it = the first restore bug), DInput
level/queue, DSound counters, fake kernel handles, fake GDI numbering (GDI deletes deferred under --headless), user\\
save files by content per write generation, heap metadata, movie head, screen text. Kernel-write imports touch first;
DIB slots touched at GetDC (GDI writes through its own view).

**Evidence:** `--snap-test 1500:3000:2` on rh-intro-errands (wine): both restore passes equal in sem / dev / RNG / fb,
end state = plain replay (tick 3054, rng 8cfb941b, state_sem identical). `rd verify` after search + undo + restores:
semv equal to a fresh replay.

## 5. Command server (`rh_serve.c`, `--serve PORT`)

openrecet's protocol: run (key-mask segments, until/avoid/samples), prog/eval (postfix VM), r/w, anchor/snap/restore/
free/diff, hash, movie, text, dump, **shot** (PNG from the composition surface: ~20 ms, no replay), explore (unchanged
search incl. score / hold). Input = 32-bit mask over the game's keys (KEYMAP = drive.py KEYS). Restore 0.6 ms.

## Open

- headless state_sem ≠ real on all scenarios: name the words (`--dump-at` both, `rhost.py diff`); expected class = fake
  object pointers / HDCs in retail memory → volatile list or deterministic equivalents.
- session speed ~370 frames/s under wine with tracking vs ~600 plain: profile; render-skip candidates gated by skip-check.
- bunker host for SotES (tools/bunker port), Windows-native sessions measured.
