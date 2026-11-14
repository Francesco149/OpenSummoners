# bunker — rhost headless under wine in ONE bounded pool (temporary NixOS box)

Port of openrecet's `tools/bunker` (its README = the design record), namespaced **oss-\***: openrecet's `orc-bunker`
and other projects' pools (`om-`, `ol2-`) share the box — never touch theirs. Third rhost host next to Windows and
maximus-wine. Host `bunker` (NixOS 26.11, i5-10600K 6c/12t, 16 GB). `RHOST_BUNKER` = ssh target (default
`root@10.0.10.58`). **Temporary: access lost on reboot** ⇒ every bunker path fails fast (`rd` under `auto` skips it).

Used by:
- **`rd`** live sessions: `RD_HOST=bunker`, or `auto` (win → bunker → wine) — `tools/drive/drive.py` calls `up`,
  `stage`, `spawn_session`, `forward`, `session_alive`, `kill_session`, `session_file`, `session_get` (file or dir:
  `rd endsave` fetches `user\`).
- **`rhost.py … --target bunker --headless`** (`rhost.run_batch` → `bunker.run_batch`): `run`, `rd verify` of a bunker
  session, `final-save` chains (`"from"` traces: predecessor replayed on bunker, cached as `final-bunker-*`). Headless
  only (no X / GPU); no `--show`. **Also every `--target wine` batch when bunker answers** (USER 2026-11-14: the wine gate = headless; `corpus --bless/--check --target wine` → `expected.wine.json`, maximus wine headless when bunker is down; `RHOST_WINE_HOST=local|bunker` forces).
- CLI:

```sh
python3 tools/bunker/bunker.py setup                 # once: work dir, nix GC roots, wine prefix (~19 s from scratch)
python3 tools/bunker/bunker.py stage                 # rhost build if stale → root/bin/<sha12>, game dirs, scripts (by stamp)
python3 tools/bunker/bunker.py run SCEN.. [-j N] [--frames N] [--extra " --x"] [--keep]
python3 tools/bunker/bunker.py status | teardown
```

A batch: `stage` (one ssh round trip when current) → `rhost.make_batch` locally (identical argv.json to `--target wine
--headless`) → rsync to `root/runs/<batch>/` → ONE container in the pool runs it (`batch.py`, -j at a time) → rsync back
to `/mnt/ephemeral/opensummoners/bunker-runs/<batch>/` (`RHOST_BUNKER_RUNS`) → remote dir deleted unless `--keep`.

## ssh: one multiplexed master
sshd refuses past `MaxStartups 10:30:100`. Every command / rsync / session forward rides ONE master:
`ControlPath=$XDG_RUNTIME_DIR/oss-bunker-<host>.ctl` (lock `….ctl.lock`), `ControlPersist=2h`; sessions add/remove
`ssh -O forward|cancel -L lp:127.0.0.1:port`. Clients `ControlMaster=no`.

## Pool (`pool.py`, on bunker as root)
- Transient unit **`oss-bunker-pool.service`** (`sleep infinity` holding a cgroup, `--collect`): `MemoryMax=5G
  MemorySwapMax=0 CPUQuota=800% AllowedCPUs=0-3,6-9` (= openrecet's CPU set; 5 G leaves room for orc 6 G + om/ol2 4 G
  each, host floor guards the sum). Box env overrides: `OSS_MEM OSS_CPUS OSS_QUOTA OSS_WINE OSS_HOST_FLOOR_MB
  OSS_STARTING_MB`.
- Every launcher writes its pid to the pool's `cgroup.procs` before exec'ing bwrap ⇒ bound over ALL OSS bunker procs.
- Admission under `root/.admit.lock` (shared w/ in-container `batch.py` via read-only `/pool`): free = `memory.max −
  (current − (file − shmem))`. rd session: ≤ `--max` (`RD_BUNKER_MAX_SESSIONS` 6), ≥ `--floor-mb`
  (`RD_BUNKER_MEM_FLOOR_MB` 1024) free after 400 MB/session still replaying (no `serve.port` yet), host MemAvailable
  ≥ 1 GB; else `NOADMIT` after `--wait` s. Batch run: ≥ 600 MB free (+350 MB per run started < 5 s ago), host ≥ 1 GB.
- Session = up when `serve.port` ("PORT PID") appears in its run dir (≤ 900 s).

## Container (bwrap per batch / per session)
`--unshare-all --share-net --uid 1000 --cap-drop ALL --die-with-parent --new-session`; `/nix/store` ro; writable only
`/oss` (= `root/` = `C:\oss-rhost` via `drive_c/oss-rhost → /oss`); prefix = copy-on-write overlay
(`--overlay-src prefix-headless --tmp-overlay /prefix`: own wineserver per container); scripts ro at `/bunker`;
`--clearenv`, no DISPLAY / WAYLAND_DISPLAY, `WINEDEBUG=-all`. Session: `sh -c '"$@"; :'` keeps sh as bwrap's parent
(survives the ssh command); `pool.py kill PID NAME` only when PID's argv names `C:\oss-rhost\runs\drive\NAME` — never
by exe name. Files land root-owned (rsync `--chown=root:root --chmod=u+rwX`; game files are `-rw----r-x` locally):
container uid 1000 = host root.

## Layout — everything under `/var/lib/oss-bunker/` (2.6 GB)
| path | what |
|---|---|
| `gcroots/{wine-wow64-unstable,bwrap,python,fontconfig}` + `wine` → first | `nix build --out-link nixpkgs#…` (system registry = rev `1c89083e`; cache hits, shared store paths w/ openrecet) |
| `bin/{pool.py,batch.py}` + `.oss-staged` | host-side pool / in-container batch runner |
| `root/` = `C:\oss-rhost` (`/oss`) | `game/{enold,ense}` (601 MB + 1.5 GB, the user's own files, symlinks dereferenced), `bin/<sha12>/rhost.exe`, `runs/<batch>/`, `runs/drive/<session>/` (removed by `rd stop`), `.admit.lock` |
| `prefix-headless/` (`/prefix`, overlaid) | WINEPREFIX from `wineboot -i` in a container (582 MB) |

Outside: only nix bookkeeping (`/nix/var/nix/gcroots/auto/*` → the gcroots) + the transient unit. No services, no
`/etc`, no global installs.

## Wine (pinned)
`wineWow64Packages.unstable` @ nixpkgs `1c89083e451e891152365b799595cf7790a43325` =
**`/nix/store/zjxh9vbxp2h12a018i0krpswfidbjh1k-wine-wow64-11.19`** (same path as openrecet's; `setup` warns if the box's
nixpkgs moved). maximus = wine 11.20.

## Measured (2026-11-13, bin `7e04fd6b6d51`)
- `setup` from scratch 19 s (store paths present); first `stage` 28 s (rhost.exe + 2.1 GB game).
- `rhost.py run rh-intro-errands rh-errands-walk --target bunker --headless -j 2`: f=4320 t=3054 rng=8cfb941b
  sem=20f208527d700782 / f=5120 t=3857 rng=6692c88a sem=63fb273cb146ba7c — frame / tick / rng = expected.wine.json,
  state_sem = local `--target wine --headless` (expected.wine.json's sem is the Xvfb run: headless differs, known).
  1350 / 1357 fps (maximus wine same moment: 367 / 372), 3.0 / 4.0 s, peak RSS 308 / 311 MB; batch 6.2 s, 19 s wall
  incl. nix develop + rsync.
- rd: `rd new bk1 --from rh-intro-errands@2900` 13.6 s wall (replay 2.7 s; ~6 s of it nix develop); `rd play bk1
  ".*300"` 7.2 s; `rd look` 6.9 s (PNG = the game: Arche at the new house, f3200); `rd verify` **MATCH** (frame 3200
  rng d729b0f8 semv 1cfb7c68944fae63) 11.2 s; `rd stop` → pool empty, run dir gone.
- Live session footprint: rhost.exe RSS 353 MB + wine helpers ~140 MB; pool non-reclaimable ≈ 290 MB/session ⇒ ~10 in 5 G
  by memory, CPU caps useful parallelism at 8.
- `-j 8`, 8 long scenarios (887k frames: rh-combat, -early-game{,-2,-3}, -water-shrine, -after-prologue,
  -errands-items, -my-play): **8/8 frame / tick / rng = expected.wine.json**; batch 353 s (= the longest,
  after-prologue 242,559 f), 518–1071 fps each at -j8 (≈ 0.45–0.9× of -j1), peak RSS 316–373 MB, pool memory.peak
  2.65 GB. The 4 `"from"` chains first derived their predecessors' final saves on bunker, serially at -j1:
  early-game 137,758 f in 113 s (1,220 fps), after-prologue 320 s (758 fps); cached as `_saves/final-bunker-*`.
  Whole command: 17 min 25 s.

## Teardown
```sh
python3 tools/bunker/bunker.py teardown     # stops oss-bunker-* units (the pool: every OSS bunker process), rm -rf
                                            # /var/lib/oss-bunker + its gcroots/auto links; closes the ssh master
```
Store paths are shared with openrecet's bunker: don't `nix-collect-garbage` on its behalf while it is set up.
