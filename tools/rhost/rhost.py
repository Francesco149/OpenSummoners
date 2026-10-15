#!/usr/bin/env python3
"""rhost.py — driver for the in-process retail host (tools/rhost) on the Windows box.

  rhost.py build                          build build/rhost/rhost.exe (nix devshell mingw)
  rhost.py stage                          upload rhost.exe + runner scripts to C:\\oss-rhost
  rhost.py setup                          one-time: stage + install the runner task + copy game data
  rhost.py run SCEN|TRACE.jsonl [..]      replay scenario(s) (tests/scenarios/<SCEN>/trace.jsonl)
  rhost.py boot [--frames N]              no input: boot → title → attract (smoke test)
  rhost.py det SCEN [-n N] [--calibrate]  N runs, dump at end, diff (determinism gate)
  rhost.py corpus [--only RE] [--check|--bless]  every scenario; summary.tsv; expected.json gate
  rhost.py verify [--only RE]             every scenario × every shadow hook → evidence table
  rhost.py diff A.rhs B.rhs               classify the differences between two dumps
  rhost.py record NAME [--save DIR|TGZ]   HUMAN PLAY on the Windows desktop (realtime) → trace
  rhost.py coverage [--only RE]           first-hit coverage of every IDA function over the corpus
  rhost.py portshots SCEN --at F,F        retail (rhost) | port exe at the same flips → cmp PNGs + differ_px

Common opts: --frames N --seed S --hook SPEC --dump-at F,..|end --shot-at F,..|end
             --shot-every N --hash-every N --edition E -j J --extra "raw rhost args"
Windows: `ssh win` (OpenSSH, PowerShell) + the interactive-session queue runner
`oss-rhost-runner` (C:\\oss-rhost\\queue) — separate from openrecet's C:\\orc-rhost, so both
projects can run batches at the same time. Results: /mnt/ephemeral/opensummoners/rhost-runs/.
"""
import argparse
import datetime
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tarfile
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
HERE = os.path.join(ROOT, "tools", "rhost")
SCEN = os.path.join(ROOT, "tests", "scenarios")
BUILD = os.path.join(ROOT, "build", "rhost")
LOCAL_RUNS = os.environ.get("RHOST_RUNS", "/mnt/ephemeral/opensummoners/rhost-runs")
IDA_DIR = os.environ.get("OSS_IDA_LOCAL", "/mnt/ephemeral/opensummoners/decompiled-ida")
WIN = os.environ.get("RHOST_WIN", "win")
WROOT = "C:\\oss-rhost"
WEXE = "oss-rhost.exe"   # Windows-side binary name (distinct from openrecet's rhost.exe)
def volatile_file(ed):
    return os.path.join(HERE, f"volatile.{ed}.txt")
EXES = {"enold": "sotes.unpacked.exe", "ense-en": "sotes_en.unpacked.exe", "ense-jp": "sotes.unpacked.exe"}
GAMEDIR = {"enold": "enold", "ense-en": "ense", "ense-jp": "ense"}
SSH_NOISE = re.compile(r"post-quantum|pq\.html|may need to be upgraded|store now, decrypt later")
sys.path.insert(0, HERE)


def sh(cmd, check=True):
    r = subprocess.run(cmd, shell=isinstance(cmd, str), text=True)
    if check and r.returncode:
        raise SystemExit(f"command failed ({r.returncode}): {cmd}")
    return r


def ssh_win(ps, timeout=7200):
    r = subprocess.run(["ssh", "-o", "BatchMode=yes", WIN, ps], text=True, capture_output=True, timeout=timeout)
    err = "\n".join(l for l in r.stderr.splitlines() if not SSH_NOISE.search(l))
    if r.returncode != 0:
        raise SystemExit(f"ssh win failed ({r.returncode}): {ps[:300]}\n{err}")
    return r.stdout


def scp(src, dst, recursive=False, check=True):
    cmd = ["scp", "-q", "-o", "BatchMode=yes"] + (["-r"] if recursive else []) + [src, dst]
    r = subprocess.run(cmd, text=True, capture_output=True)
    if check and r.returncode != 0:
        raise SystemExit(f"scp failed: {' '.join(cmd)}\n{r.stderr}")
    return r.returncode == 0


def win_job(job_id, ps_path_win, timeout=14400):
    """run C:\\...\\x.ps1 in the INTERACTIVE session through the oss-rhost queue runner"""
    q = WROOT + "\\queue"
    ps = (f"if (!(Get-ScheduledTask -TaskName oss-rhost-runner -ErrorAction SilentlyContinue)) {{ 'NO RUNNER: rhost.py setup'; exit 3 }}; "
          f"Copy-Item '{ps_path_win}' '{q}\\{job_id}.tmp'; Move-Item '{q}\\{job_id}.tmp' '{q}\\{job_id}.job.ps1'; "
          f"while (!(Test-Path '{q}\\{job_id}.done')) {{ Start-Sleep -Milliseconds 200 }}; "
          f"Get-Content '{q}\\{job_id}.out'; Remove-Item '{q}\\{job_id}.*'")
    return ssh_win(ps, timeout=timeout)


# ---------------------------------------------------------------- build/stage
def cmd_build(a=None):
    sh(["nix", "develop", ROOT, "--command", "bash", os.path.join(HERE, "build.sh")])


def cmd_stage(a=None):
    ssh_win(f"New-Item -ItemType Directory -Force {WROOT}\\bin,{WROOT}\\runs,{WROOT}\\queue,{WROOT}\\game | Out-Null; exit 0")
    # deployed as oss-rhost.exe: openrecet's stage does `Get-Process rhost | Stop-Process` (by
    # NAME) — a shared `rhost.exe` name let a sibling restage kill our in-flight batch mid-run
    scp(os.path.join(BUILD, "rhost.exe"), f"{WIN}:C:/oss-rhost/bin/{WEXE}")
    for f in ("runner.ps1", "install-runner.ps1", "stage-game.ps1"):
        scp(os.path.join(HERE, "win", f), f"{WIN}:C:/oss-rhost/{f}")
    print("staged", BUILD, "->", WROOT)


def cmd_setup(a):
    cmd_build()
    cmd_stage()
    print(ssh_win(f"powershell -NoProfile -ExecutionPolicy Bypass -File {WROOT}\\install-runner.ps1"))
    for ed in ("enold", "ense"):
        print(ssh_win(f"powershell -NoProfile -ExecutionPolicy Bypass -File {WROOT}\\stage-game.ps1 -Edition {ed}"))


def rhost_stale():
    exe = os.path.join(BUILD, "rhost.exe")
    if not os.path.exists(exe):
        return True
    t = os.path.getmtime(exe)
    return any(os.path.getmtime(os.path.join(HERE, f)) > t for f in os.listdir(HERE) if f.endswith((".c", ".h", ".sh"))) or \
        any(os.path.getmtime(os.path.join(ROOT, "src", f)) > t for f in os.listdir(os.path.join(ROOT, "src")) if f.endswith((".c", ".h")))


def ensure_staged():
    exe = os.path.join(BUILD, "rhost.exe")
    stamp = os.path.join(BUILD, ".staged")
    if rhost_stale():
        cmd_build()
    if not os.path.exists(stamp) or os.path.getmtime(stamp) < os.path.getmtime(exe) or \
            os.path.getmtime(stamp) < max(os.path.getmtime(os.path.join(HERE, "win", f)) for f in os.listdir(os.path.join(HERE, "win"))):
        cmd_stage()
        open(stamp, "w").close()


# ---------------------------------------------------------------- scenarios
def scenario_inputs(name):
    """→ (trace path, save dir/tgz or None, edition)"""
    if name.endswith(".jsonl") and os.path.exists(name):
        return os.path.abspath(name), None, None
    d = os.path.join(SCEN, name)
    tr = os.path.join(d, "trace.jsonl")
    if not os.path.exists(tr):
        raise SystemExit(f"no trace: {tr}")
    save, ed = None, None
    for line in open(tr):
        line = line.strip()
        if line.startswith("{") and '"savefile"' in line:
            ref = re.search(r'"savefile"\s*:\s*"([^"]*)"', line).group(1)
            save = None if ref == "@fresh" else os.path.normpath(os.path.join(d, ref))
        if line.startswith("{") and '"edition"' in line:
            ed = re.search(r'"edition"\s*:\s*"([^"]*)"', line).group(1)
    return tr, save, ed


def stage_save(save, rdir):
    """a save = the game's user\\ dir (savedataNN.dat ...): a directory or a .tar.gz of one"""
    dst = os.path.join(rdir, "user")
    if os.path.isdir(save):
        shutil.copytree(save, dst)
    else:
        os.makedirs(dst)
        with tarfile.open(save) as t:
            t.extractall(dst, filter="data")


def rhost_args(a, run_win, trace_win, edition):
    gd = f"{WROOT}\\game\\{GAMEDIR[edition]}"
    args = ["--exe", f"{gd}\\{EXES[edition]}", "--game", gd, "--run", run_win]
    if trace_win:
        args += ["--trace", trace_win]
    for k, flag in (("frames", "--frames"), ("seed", "--seed"), ("hook", "--hook"), ("dump_at", "--dump-at"),
                    ("hash_every", "--hash-every"), ("shot_at", "--shot-at"), ("shot_every", "--shot-every"),
                    ("log_every", "--log-every"), ("stall_ms", "--stall-ms")):
        v = getattr(a, k, None)
        if v is not None and v != "":
            args += [flag, str(v)]
    if getattr(a, "coverage_file", None):
        args += ["--coverage", f"{run_win}\\coverage_in.txt"]
    if getattr(a, "show", False):
        args += ["--show"]
    if getattr(a, "extra", None):
        args += shlex.split(a.extra)
    return args


def make_batch(a, runs, tag):
    """runs: list of (run_name, trace_path|None, save|None, edition|None)"""
    batch = datetime.datetime.now().strftime("%Y%m%d-%H%M%S") + "-" + tag
    bdir = os.path.join(LOCAL_RUNS, batch)
    os.makedirs(bdir)
    lines = ["$ErrorActionPreference = 'Continue'", f"$J = {max(1, a.jobs)}", "$procs = @()"]
    names = []
    for name, trace, save, ed in runs:
        ed = getattr(a, "edition", None) or ed or "enold"
        rdir = os.path.join(bdir, name)
        os.makedirs(rdir)
        if os.path.exists(volatile_file(ed)):
            shutil.copy(volatile_file(ed), os.path.join(rdir, "volatile.txt"))
        if save:
            stage_save(save, rdir)
        run_win = f"{WROOT}\\runs\\{batch}\\{name}"
        trace_win = None
        if trace:
            shutil.copy(trace, os.path.join(rdir, "trace.jsonl"))
            trace_win = f"{run_win}\\trace.jsonl"
        if getattr(a, "coverage_file", None):
            shutil.copy(a.coverage_file, os.path.join(rdir, "coverage_in.txt"))
        args = rhost_args(a, run_win, trace_win, ed)
        open(os.path.join(rdir, "argv.json"), "w").write(json.dumps(args))
        ps_args = ",".join("'" + x.replace("'", "''") + "'" for x in args)
        lines.append(f"while (@($procs | Where-Object {{ !$_.HasExited }}).Count -ge $J) {{ Start-Sleep -Milliseconds 50 }}")
        lines.append(f"$procs += Start-Process -FilePath '{WROOT}\\bin\\{WEXE}' -ArgumentList @({ps_args}) -PassThru "
                     f"-WindowStyle Hidden -RedirectStandardOutput '{run_win}\\stdout.txt' -RedirectStandardError '{run_win}\\stderr.txt'")
        names.append(name)
    lines.append("$procs | ForEach-Object { $_.WaitForExit() }")
    for name in names:
        run_win = f"{WROOT}\\runs\\{batch}\\{name}"
        lines.append(f"Write-Output '=== {name}'")
        lines.append(f"if (Test-Path '{run_win}\\rhost.log') {{ Select-String -Path '{run_win}\\rhost.log' -Pattern 'RESULT|FATAL|DIVERGE|hook-result|MessageBox' | ForEach-Object {{ $_.Line }} }}")
        lines.append(f"if (Test-Path '{run_win}\\stderr.txt') {{ Get-Content '{run_win}\\stderr.txt' -TotalCount 5 }}")
    open(os.path.join(bdir, "go.ps1"), "w").write("\r\n".join(lines) + "\r\n")
    return batch, bdir


# ---------------------------------------------------------------- wine target (maximus, headless)
# A prefix whose drive_c/oss-rhost → WINE_ROOT, so retail sees the SAME Windows paths
# (C:\oss-rhost\game\<ed>, …\runs\<batch>\<run>) as on the Windows host. Game data: WINE_GAMES.
# Wine is a valid LOGIC oracle (RNG/state match Windows: SE boot f900 rng 9d661dcf both); pixels and
# speed are Windows truth (wined3d/llvmpipe).
WINE_ROOT = os.environ.get("RHOST_WINE_ROOT", "/mnt/ephemeral/opensummoners/wine/root")
WINE_PREFIX = os.environ.get("RHOST_WINEPREFIX", "/mnt/ephemeral/opensummoners/wine/prefix")
WINE_GAMES = {"enold": os.environ.get("RHOST_WINE_GAME_ENOLD", "/mnt/ephemeral/opensummoners/game/enold"),
              "ense": os.environ.get("RHOST_WINE_GAME_ENSE", "/mnt/ephemeral/opensummoners/game/ense")}


def wine_stage():
    for d in ("bin", "game", "runs"):
        os.makedirs(os.path.join(WINE_ROOT, d), exist_ok=True)
    src, dst = os.path.join(BUILD, "rhost.exe"), os.path.join(WINE_ROOT, "bin", "rhost.exe")
    if not os.path.exists(dst) or os.path.getmtime(dst) < os.path.getmtime(src):
        shutil.copy2(src, dst)
    for ed, path in WINE_GAMES.items():
        link = os.path.join(WINE_ROOT, "game", ed)
        if os.path.isdir(path) and not os.path.islink(link):
            os.symlink(path, link)
    if not os.path.isdir(os.path.join(WINE_PREFIX, "drive_c")):
        subprocess.run(["wineboot", "-i"], env=dict(os.environ, WINEPREFIX=WINE_PREFIX, WINEDEBUG="-all"), capture_output=True)
    link = os.path.join(WINE_PREFIX, "drive_c", "oss-rhost")
    if not os.path.islink(link):
        os.symlink(WINE_ROOT, link)


def run_batch_wine(a, runs, tag, fetch):
    import concurrent.futures
    if rhost_stale():
        cmd_build()
    wine_stage()
    global LOCAL_RUNS
    saved, LOCAL_RUNS = LOCAL_RUNS, os.path.join(WINE_ROOT, "runs")
    try:
        batch, bdir = make_batch(a, runs, tag)
    finally:
        LOCAL_RUNS = saved
    # ONE Xvfb per batch shared by every worker: the runs share the prefix (one wineserver), and
    # wine processes of one wineserver on different X displays die with BadWindow
    disp = 400 + os.getpid() % 500
    xvfb = subprocess.Popen(["Xvfb", f":{disp}", "-screen", "0", "1280x1024x16", "-nolisten", "tcp"],
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(1.0)
    env = dict(os.environ, WINEPREFIX=WINE_PREFIX, WINEDEBUG="-all", DISPLAY=f":{disp}")

    def one(ir):
        i, r = ir
        name = r[0]
        argv = json.load(open(os.path.join(bdir, name, "argv.json")))
        out = subprocess.run(["wine", f"{WROOT}\\bin\\rhost.exe"] + argv,
                             capture_output=True, text=True, env=env, timeout=a.wine_timeout)
        open(os.path.join(bdir, name, "wine.out"), "w").write(out.stdout[-4000:] + out.stderr[-4000:])
        log = os.path.join(bdir, name, "rhost.log")
        return name, open(log, errors="replace").read() if os.path.exists(log) else ""
    t0 = time.time()
    results, out_lines = {}, []
    with concurrent.futures.ThreadPoolExecutor(max_workers=a.jobs) as ex:
        for name, log in ex.map(one, list(enumerate(runs))):
            out_lines.append(f"=== {name}")
            for line in log.splitlines():
                if line.startswith("RESULT"):
                    results[name] = dict(kv.split("=", 1) for kv in line.split()[1:] if "=" in kv)
                elif re.search(r"FATAL|DIVERGE|hook-result|MessageBox", line):
                    results.setdefault(name + "#notes", []).append(line.strip())
            out_lines.append(log)
    xvfb.terminate()
    print(f"[wine batch {batch}: {len(runs)} run(s) in {time.time() - t0:.1f}s (-j{a.jobs}) → {bdir}]")
    return batch, bdir, results, "\n".join(out_lines)


def run_batch(a, runs, tag, fetch=("rhost.log",)):
    if getattr(a, "target", "win") == "wine":
        return run_batch_wine(a, runs, tag, fetch)
    ensure_staged()
    batch, bdir = make_batch(a, runs, tag)
    scp(bdir, f"{WIN}:C:/oss-rhost/runs/", recursive=True)
    t0 = time.time()
    out = win_job(batch, f"{WROOT}\\runs\\{batch}\\go.ps1")
    results, cur = {}, None
    for line in out.splitlines():
        if line.startswith("=== "):
            cur = line[4:].strip()
        elif cur and line.startswith("RESULT"):
            results[cur] = dict(kv.split("=", 1) for kv in line.split()[1:] if "=" in kv)
        elif cur and line.strip():
            results.setdefault(cur + "#notes", []).append(line.strip())
    for name, *_ in runs:
        for f in fetch:
            dst = os.path.join(bdir, name) if "*" in f else os.path.join(bdir, name, f)
            scp(f"{WIN}:C:/oss-rhost/runs/{batch}/{name}/{f}", dst, check=False)
    print(f"[batch {batch}: {len(runs)} run(s) in {time.time() - t0:.1f}s (-j{a.jobs}) → {bdir}]")
    return batch, bdir, results, out


def resolve_runs(names):
    return [(os.path.splitext(os.path.basename(n))[0] if n.endswith(".jsonl") else n, *scenario_inputs(n)) for n in names]


def all_scenarios(only=None):
    names = sorted(d for d in os.listdir(SCEN) if os.path.exists(os.path.join(SCEN, d, "trace.jsonl"))
                   and any('"keys"' in l for l in open(os.path.join(SCEN, d, "trace.jsonl"))))
    return [n for n in names if not only or re.search(only, n)]


def print_results(results, names):
    for n in names:
        r = results.get(n)
        if not r:
            print(f"{n:40s} NO RESULT  {' | '.join(results.get(n + '#notes', [])[:4])}")
            continue
        print(f"{n:40s} {r.get('status'):5s} {r.get('reason', ''):8s} f={r.get('frame'):>6s} t={r.get('tick', '?'):>6s} "
              f"rng={r.get('rng')} sem={r.get('state_sem')} {r.get('fps', '?')}fps complete={r.get('complete', '?')}")
        for note in results.get(n + "#notes", [])[:6]:
            print("    " + note)


# ---------------------------------------------------------------- commands
def cmd_run(a):
    runs = resolve_runs(a.scenarios)
    fetch = ("rhost.log", "shot_*.png", "stdout.txt", "stderr.txt", "grid_*.bin") + (("state_*.rhs",) if a.dump_at else ())
    batch, bdir, res, out = run_batch(a, runs, "run", fetch=fetch)
    print_results(res, [r[0] for r in runs])
    return 0 if all(res.get(r[0], {}).get("status") == "ok" for r in runs) else 1


def cmd_boot(a):
    runs = [("boot", None, None, None)]
    a.frames = a.frames or 1500
    fetch = ("rhost.log", "shot_*.png", "stdout.txt", "stderr.txt", "grid_*.bin") + (("state_*.rhs",) if a.dump_at else ())
    batch, bdir, res, out = run_batch(a, runs, "boot", fetch=fetch)
    print_results(res, ["boot"])
    if not res.get("boot"):
        print(out[-3000:])
    return 0 if res.get("boot", {}).get("status") == "ok" else 1


def cmd_det(a):
    import rhs
    if a.scenario == "boot":
        tr, save, ed = None, None, None
        a.frames = a.frames or 1500
    else:
        tr, save, ed = scenario_inputs(a.scenario)
    a.dump_at = (a.dump_at + ",end") if a.dump_at else "end"
    runs = [(f"r{i}", tr, save, ed) for i in range(a.n)]
    batch, bdir, res, out = run_batch(a, runs, "det-" + os.path.basename(a.scenario), fetch=("rhost.log", "state_end.rhs"))
    print_results(res, [r[0] for r in runs])
    base = os.path.join(bdir, "r0", "state_end.rhs")
    bad, learned = 0, set()
    for i in range(1, a.n):
        other = os.path.join(bdir, f"r{i}", "state_end.rhs")
        if not (os.path.exists(base) and os.path.exists(other)):
            print(f"--- r0 vs r{i}: dump missing"); bad += 1; continue
        print(f"--- r0 vs r{i}")
        rows = rhs.diff(base, other, show=20)
        real = [r for r in rows if r[3] not in ("ext-ptr(both)", "ext-ptr(stale)", "volatile-addr")]
        bad += len(real)
        learned |= {r[0] for r in rows if r[3] in ("ext-ptr(both)", "ext-ptr(stale)")}
    sems = {res.get(r[0], {}).get("state_sem") for r in runs}
    print(f"DETERMINISM {'PASS' if not bad else 'FAIL'}: {bad} non-pointer differing dwords; "
          f"semantic hashes: {len(sems)} distinct; external-pointer slots seen: {len(learned)}")
    if a.calibrate and learned:
        VOLATILE = volatile_file(a.edition or ed or "enold")
        old = set()
        if os.path.exists(VOLATILE):
            old = {int(l.split()[0], 16) for l in open(VOLATILE) if l.strip() and not l.startswith("#")}
        new = sorted(old | learned)
        with open(VOLATILE, "w") as f:
            f.write("# rhost volatile addresses: slots that hold system-DLL/COM pointers (masked in state_sem).\n"
                    "# Learned by `rhost.py det SCEN --calibrate` (only diffs that are external pointers in BOTH runs).\n")
            for v in new:
                f.write(f"{v:08x}\n")
        print(f"calibrated {VOLATILE}: {len(old)} → {len(new)} addresses")
    return 0 if not bad else 1


def cmd_corpus(a):
    names = all_scenarios(a.only)
    runs = resolve_runs(names)
    batch, bdir, res, out = run_batch(a, runs, "corpus", fetch=("rhost.log",))
    print_results(res, names)
    done = sum(1 for n in names if res.get(n, {}).get("complete") == "1")
    print(f"CORPUS: {done}/{len(names)} complete; results in {bdir}")
    keys = ("frame", "tick", "rng", "state_sem")
    bad = 0
    for n in names:
        ep = os.path.join(SCEN, n, "expected.json" if getattr(a, "target", "win") == "win" else f"expected.{a.target}.json")
        got = {k: res.get(n, {}).get(k) for k in keys}
        if a.bless and res.get(n, {}).get("status") == "ok":
            json.dump(got, open(ep, "w"), indent=1); continue
        if a.check and os.path.exists(ep):
            exp = json.load(open(ep))
            diff = {k: (exp.get(k), got[k]) for k in keys if exp.get(k) != got[k]}
            if diff:
                bad += 1; print(f"  REGRESSION {n}: " + ", ".join(f"{k} {e}→{g}" for k, (e, g) in diff.items()))
    if a.bless:
        print(f"blessed expected{'' if a.target == 'win' else '.' + a.target}.json for every ok scenario (volatile.<edition>.txt changes state_sem: re-bless after --calibrate)")
    if a.check:
        print(f"CHECK: {'PASS' if not bad else 'FAIL'} ({bad} scenario(s) differ from expected.json)")
        return 1 if bad else 0
    with open(os.path.join(bdir, "summary.tsv"), "w") as f:
        f.write("scenario\tstatus\treason\tframe\ttick\tcomplete\trng\tstate_sem\tfps\n")
        for n in names:
            r = res.get(n, {})
            f.write("\t".join([n] + [r.get(k, "-") for k in ("status", "reason", "frame", "tick", "complete", "rng", "state_sem", "fps")]) + "\n")
    return 0 if done == len(names) else 1


VERIFY_HOOKS = ["rand", "input_poll", "mover_v", "sweep_h", "ent_move_h", "ent_move_v", "map_decode", "cam_follow", "cam_shake", "char_ai@4", "char_physics", "char_anim@32", "char_integrate@64", "hazard_probe", "npc_ai@4", "char_apply@16", "world_tick@8",
                "fx_add@4", "fx_tick@64", "splash@32", "snd_release@4", "snd_all@64", "snd_one@32", "actor_update@16", "particle_tick@16", "char_behave@4",
                "step_ok@4", "pick_char", "walk_to@4", "pace@4", "part_cx@16", "part_cy@16",
                "vpool_load", "vpool_take@4", "sfx_place@4", "sfx_play", "sfx_play_at@4", "voice_place", "voice_play", "snd_start@4",
                "voice_cue", "voiceset_tick@64", "vline_stop@4", "voiceset_sounding",
                "entity_unlist", "entity_release", "actor_release", "char_release", "set_collide", "carry_riders", "list_remove",
                "status_stuck@4", "grid_b_word@4", "pit_check@16", "respawn_mark@16", "mods_reset@16", "ai_reset", "body_slowed@4",
                "set_look_up", "targetable@4", "snd_loop@4", "sfx_play_jit", "force_action", "grid_c_at_feet@4", "add_hp", "add_mp",
                "ambient_timer@16", "idle_voice@16", "entity_scatter", "sys_by_mode",
                "actor_crate@4", "actor_break", "actor_slide_v@4", "count_riders@4", "entity_dust", "entity_anchor", "particle_spawn@4", "set_track", "grid_refresh", "grid_mark",
                "particle_init@16", "entity_setup@16", "entity_ride@4", "entity_ride_part@4", "polar_div", "polar_milli", "tint_fade_a@4",
                "tint_fade_b@4", "track_on@16", "snd_alloc", "clip_jitter", "parts_free", "set_angle@16", "fx_reset@16",
                "skill_avail@4", "party_find@4", "floor_below@4",
                "gimmick_pass@32", "gimmick_step@128", "switch_set", "switch_link", "actor_unlist_all", "switch_glow", "rect_hit",
                "chars_in_rect_count", "char_in_rect", "char_riding", "particle_find@4", "next_serial", "actor_shot", "warp_request",
                "party_gather", "touch_pass", "touch_actor", "room_input_flush", "probe_rect", "entity_near@4", "event_run",
                "sys_unload", "zone_pass@8", "zone_hit@16", "band96_near",
                "room_input_update@16", "room_char_step@32", "room_actor_step@64", "room_grid_anim@16", "room_party_down",
                "room_lead@16", "timed_sounds_tick@16", "input_dik_produce@16",
                "encounter_tick@4", "foes_near@16", "party_foes_near@4", "pslot_ok@16", "bgm_pick@4",
                "entity_bodies_alloc@16", "char_links_clear", "view_pools_free", "shake_clear", "volume_fade", "char_map_id",
                "char_map_npc", "pslot_status_clear", "gate_actor", "sprite_slot_ready", "sprite_slot_tint", "timed_sounds_fade",
                "fill_lookup", "world_leader@4", "camera_snap@4", "zones_reset", "quest_state", "view_record_init", "room_state_save",
                "map_res_free", "map_res_delete", "pool_head_delete", "entity_free@16", "actor_free@4", "char_free", "char_block_init",
                "room_formation", "char_follow_set", "actor_state_restore", "palette_tint@16", "map_res_load"]
# REPLACE-only hooks (a whole room per call / DirectInput): proven by `corpus --check --hook replace:…`
REPLACE_ONLY_HOOKS = ["room_loop", "input_repoll_flush", "load_room",
                      "world_new", "world_free"]   # (their shadows skip: the grid's 2.9 MB block moves the arena commit)
# name@N = shadow every Nth call (the full-state shadows cost ~10-20 ms per compared call)


def cmd_verify(a):
    names = all_scenarios(a.only)
    a.hook = ",".join("shadow:" + h for h in VERIFY_HOOKS)
    runs = resolve_runs(names)
    batch, bdir, res, out = run_batch(a, runs, "verify")
    tot = {}
    for n in names:
        for note in res.get(n + "#notes", []):
            m = re.match(r"hook-result shadow (\S+) calls=(\d+) diverge=(\d+) skipped=(\d+) compared=(\d+)(?: first: (.*))?", note)
            if m:
                t = tot.setdefault(m.group(1), [0, 0, 0, set(), ""])
                t[0] += int(m.group(5)); t[1] += int(m.group(3)); t[2] += int(m.group(4))
                if int(m.group(5)):
                    t[3].add(n)
                if m.group(6) and not t[4]:
                    t[4] = f"{n}: {m.group(6)}"
    done = sum(1 for n in names if res.get(n, {}).get("complete") == "1")
    print(f"\nVERIFY ({done}/{len(names)} scenarios complete)")
    print(f"{'function':22s} {'compared':>9s} {'diverge':>8s} {'skipped':>8s} scenarios")
    for hs in VERIFY_HOOKS:
        h = hs.split("@")[0]
        c, d, k, sc, first = tot.get(h, [0, 0, 0, set(), ""])
        print(f"{hs:22s} {c:9d} {d:8d} {k:8d} {len(sc)}" + (f"  FIRST: {first}" if d else ""))
    return 0 if all(tot.get(h.split("@")[0], [0, 0])[1] == 0 for h in VERIFY_HOOKS) else 1


def cmd_coverage(a):
    tsv = os.path.join(IDA_DIR, a.edition or "enold", "functions_ida.tsv")
    vas = [l.split("\t")[0] for l in open(tsv).readlines()[1:]]
    cov_in = os.path.join(LOCAL_RUNS, "coverage_in.txt")
    os.makedirs(LOCAL_RUNS, exist_ok=True)
    open(cov_in, "w").write("\n".join(vas) + "\n")
    a.coverage_file = cov_in
    names = all_scenarios(a.only) if not a.scenarios else a.scenarios
    runs = resolve_runs(names)
    batch, bdir, res, out = run_batch(a, runs, "coverage", fetch=("rhost.log", "coverage.txt"))
    print_results(res, [r[0] for r in runs])
    hit = {}
    for n, *_ in runs:
        p = os.path.join(bdir, n, "coverage.txt")
        if os.path.exists(p):
            for l in open(p):
                va, fr = l.split()
                hit.setdefault(va, []).append(n)
    sizes = {l.split("\t")[0]: int(l.split("\t")[1]) for l in open(tsv).readlines()[1:]}
    libs = {l.split("\t")[0] for l in open(tsv).readlines()[1:] if l.split("\t")[3] == "1"}
    game = [v for v in vas if v not in libs]
    gh = [v for v in game if v in hit]
    print(f"COVERAGE: {len(hit)}/{len(vas)} functions hit; game (non-FLIRT) {len(gh)}/{len(game)} = "
          f"{sum(sizes[v] for v in gh)}/{sum(sizes[v] for v in game)} bytes "
          f"({100.0 * sum(sizes[v] for v in gh) / max(1, sum(sizes[v] for v in game)):.1f}%)")
    with open(os.path.join(bdir, "coverage_union.tsv"), "w") as f:
        f.write("va\tsize\tscenarios\n")
        for v in vas:
            if v in hit:
                f.write(f"{v}\t{sizes[v]}\t{','.join(hit[v])}\n")
    print(f"→ {bdir}/coverage_union.tsv")


# DIK press → ring ids, the retail producer 0x46a880 (quirk #126); used to drive the PORT, which
# injects ring ids (--input-trace) + held levels (--held-trace) instead of emulating DirectInput
DIK_IDS = {0x1c: [20, 36], 0xc8: [1, 35], 0xd0: [3], 0xcb: [2], 0xcd: [4], 0x01: [39], 0x0e: [39],
           0x3b: [16], 0x3c: [21], 0x3d: [22], 0x3e: [23], 0x3f: [10], 0x40: [11], 0xc5: [18], 0x57: [19], 0x42: [34]}
DIK_IDS.update({0x02 + i: [24 + i] for i in range(10)})


def port_traces(trace, out_dir):
    held, ids, prev = [], [], set()
    for line in open(trace):
        line = line.strip()
        if not line.startswith("{") or '"keys"' not in line:
            continue
        d = json.loads(line)
        cur = set(d["keys"])
        held.append({"frame": d["frame"], "keys": sorted(cur)})
        pressed = sorted(cur - prev)
        ii = [i for k in pressed for i in DIK_IDS.get(k, [])]
        if ii:
            ids.append({"frame": d["frame"], "ids": ii})
        prev = cur
    hp, ip = os.path.join(out_dir, "port_held.jsonl"), os.path.join(out_dir, "port_ids.jsonl")
    open(hp, "w").write("".join(json.dumps(x) + "\n" for x in held))
    open(ip, "w").write("".join(json.dumps(x) + "\n" for x in ids))
    return hp, ip


def cmd_portshots(a):
    """retail (rhost) | port at the same flips, same scenario; the port runs --det-clock (rhost's clock model)"""
    from PIL import Image, ImageChops
    ensure_staged()
    sh(["nix", "develop", ROOT, "--command", "make", "-s", "-C", os.path.join(ROOT, "src")])
    ssh_win(f"New-Item -ItemType Directory -Force {WROOT}\\port | Out-Null; "
            f"if (!(Test-Path {WROOT}\\portgame\\sotesd.dll)) {{ Copy-Item -Recurse {WROOT}\\game\\enold {WROOT}\\portgame }}; exit 0")
    scp(os.path.join(ROOT, "build", "opensummoners.exe"), f"{WIN}:C:/oss-rhost/port/opensummoners.exe")
    tr, save, ed = scenario_inputs(a.scenario)
    frames = sorted(int(x) for x in a.at.split(","))
    W = a.search or 0
    # numbering: the port's capture N = the N-th present (1-based, g_present_frame after ++);
    # rhost's shot fK = the frame presented by flip K+1 (0-based) ⇒ port N ≡ retail shot N-1
    rframes = sorted({g - 1 for f in frames for g in range(max(1, f - W), f + W + 1)})
    if len(rframes) > 64:
        a.shot_at = None; a.shot_every = 1                       # dense window: every flip
    else:
        a.shot_at = ",".join(str(f) for f in rframes)
    a.frames = max(frames) + W + 2
    frames = [f for f in frames if f >= 1]
    a.jobs = 2
    batch, bdir = make_batch(a, [("retail", tr, save, ed)], "portshots")
    pdir = os.path.join(bdir, "port"); os.makedirs(pdir)
    port_traces(tr, pdir)
    pw = f"{WROOT}\\runs\\{batch}\\port"
    pargs = ["--hide-window", "--allow-multi"] + ([] if a.port_realtime else ["--det-clock"]) + ["--frames", str(max(frames) + 2), "--capture-frames", ",".join(map(str, frames)),
             "--capture-dir", pw, "--input-trace", pw + "\\port_ids.jsonl", "--held-trace", pw + "\\port_held.jsonl"] \
        + shlex.split(a.port_extra or "")
    go = open(os.path.join(bdir, "go.ps1"), newline="").read().split("\r\n")
    i = next(k for k, l in enumerate(go) if l.startswith("$procs | ForEach"))
    go.insert(i, f"$env:OPENSUMMONERS_GAME_DIR='{WROOT}\\portgame'; $procs += Start-Process -FilePath '{WROOT}\\port\\opensummoners.exe' "
                 f"-ArgumentList @({','.join(chr(39) + x.replace(chr(39), chr(39) * 2) + chr(39) for x in pargs)}) -PassThru -WindowStyle Hidden -WorkingDirectory '{WROOT}\\portgame'")
    open(os.path.join(bdir, "go.ps1"), "w").write("\r\n".join(go))
    scp(bdir, f"{WIN}:C:/oss-rhost/runs/", recursive=True)
    out = win_job(batch, f"{WROOT}\\runs\\{batch}\\go.ps1")
    scp(f"{WIN}:C:/oss-rhost/runs/{batch}/retail/*", os.path.join(bdir, "retail"), recursive=True, check=False)
    scp(f"{WIN}:C:/oss-rhost/runs/{batch}/port/*", pdir, recursive=True, check=False)
    rows = []
    for f in frames:
        rp = os.path.join(bdir, "retail", f"shot_f{f - 1:06d}.png")
        pp = os.path.join(pdir, f"port_frame_{f:05d}.bmp")
        if not (os.path.exists(rp) and os.path.exists(pp)):
            print(f"f{f}: missing {'retail' if not os.path.exists(rp) else 'port'} frame"); continue
        r = Image.open(rp).convert("RGB"); q = Image.open(pp).convert("RGB")
        if q.size != r.size:
            q = q.resize(r.size)
        d = ImageChops.difference(r, q)
        n = sum(1 for px in d.getdata() if px != (0, 0, 0))
        rows.append((f, n))
        m = Image.new("RGB", (r.width * 3, r.height))
        m.paste(r, (0, 0)); m.paste(q, (r.width, 0)); m.paste(d.point(lambda v: min(255, v * 8)), (r.width * 2, 0))
        m.save(os.path.join(bdir, f"cmp_f{f:06d}.png"))
    for f, n in rows:
        line = f"f{f:6d}  differ_px={n}"
        if W:
            pp = os.path.join(pdir, f"port_frame_{f:05d}.bmp")
            q = Image.open(pp).convert("RGB")
            best = None
            for g in range(max(1, f - W), f + W + 1):
                rp = os.path.join(bdir, "retail", f"shot_f{g - 1:06d}.png")
                if not os.path.exists(rp):
                    continue
                d = ImageChops.difference(Image.open(rp).convert("RGB"), q.resize(Image.open(rp).size))
                n2 = sum(1 for px in d.getdata() if px != (0, 0, 0))
                if best is None or n2 < best[1]:
                    best = (g, n2)
            if best:
                line += f"   best retail flip {best[0]} ({best[0] - f:+d}) differ_px={best[1]}"
        print(line)
    print(f"→ {bdir}/cmp_f*.png  (retail | port | diff×8)")


def cmd_diff(a):
    import rhs
    rows = rhs.diff(a.a, a.b, show=a.show)
    print(f"{len(rows)} differing dwords")


def cmd_record(a):
    """human play: visible window on the Windows desktop, realtime, real keyboard"""
    ensure_staged()
    name = a.name
    out_dir = os.path.join(SCEN, name)
    if os.path.exists(os.path.join(out_dir, "trace.jsonl")) and not a.force:
        raise SystemExit(f"{out_dir}/trace.jsonl exists (use --force)")
    a.show = True
    a.extra = ((a.extra or "") + f" --record trace.jsonl").strip()
    a.jobs = 1
    runs = [(name, None, a.save, a.edition)]
    batch, bdir, res, out = run_batch(a, runs, "record-" + name, fetch=("rhost.log", "trace.jsonl"))
    print_results(res, [name])
    src = os.path.join(bdir, name, "trace.jsonl")
    if not os.path.exists(src):
        raise SystemExit("no trace recorded")
    os.makedirs(out_dir, exist_ok=True)
    with open(os.path.join(out_dir, "trace.jsonl"), "w") as f:
        f.write(f'{{"meta": {{"recorded": "{datetime.datetime.now().isoformat(timespec="seconds")}", "edition": "{a.edition or "enold"}"}}}}\n')
        if a.save:
            f.write(f'{{"savefile": "{os.path.relpath(a.save, out_dir)}"}}\n')
        f.write(open(src).read())
    print(f"recorded → {out_dir}/trace.jsonl (verify: rhost.py det {name})")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    def common(p):
        p.add_argument("--frames", type=int)
        p.add_argument("--seed")
        p.add_argument("--hook")
        p.add_argument("--dump-at")
        p.add_argument("--shot-at")
        p.add_argument("--shot-every", type=int)
        p.add_argument("--hash-every", type=int)
        p.add_argument("--log-every", type=int)
        p.add_argument("--stall-ms", type=int)
        p.add_argument("--edition", choices=sorted(EXES))
        p.add_argument("--show", action="store_true")
        p.add_argument("-j", "--jobs", type=int, default=8)
        p.add_argument("--extra")
        p.add_argument("--target", choices=("win", "wine"), default=os.environ.get("RHOST_TARGET", "win"),
                       help="win = the Windows box (pixel + speed truth); wine = local headless (logic oracle)")
        p.add_argument("--wine-timeout", type=int, default=1800)
    sub.add_parser("build").set_defaults(fn=cmd_build)
    sub.add_parser("stage").set_defaults(fn=cmd_stage)
    sub.add_parser("setup").set_defaults(fn=cmd_setup)
    p = sub.add_parser("run"); p.add_argument("scenarios", nargs="+"); common(p); p.set_defaults(fn=cmd_run)
    p = sub.add_parser("boot"); common(p); p.set_defaults(fn=cmd_boot)
    p = sub.add_parser("det"); p.add_argument("scenario"); p.add_argument("-n", type=int, default=3)
    p.add_argument("--calibrate", action="store_true"); common(p); p.set_defaults(fn=cmd_det)
    p = sub.add_parser("corpus"); p.add_argument("--only"); p.add_argument("--bless", action="store_true")
    p.add_argument("--check", action="store_true"); common(p); p.set_defaults(fn=cmd_corpus)
    p = sub.add_parser("verify"); p.add_argument("--only"); common(p); p.set_defaults(fn=cmd_verify)
    p = sub.add_parser("coverage"); p.add_argument("--only"); p.add_argument("scenarios", nargs="*"); common(p); p.set_defaults(fn=cmd_coverage)
    p = sub.add_parser("portshots"); p.add_argument("scenario"); p.add_argument("--at", required=True)
    p.add_argument("--port-realtime", action="store_true", help="run the port on its wall clock instead of --det-clock")
    p.add_argument("--search", type=int, help="also find the best-matching retail flip within ±W of each port frame")
    p.add_argument("--port-extra", help="extra port args, e.g. --port-extra=--boot-loading")
    common(p); p.set_defaults(fn=cmd_portshots)
    p = sub.add_parser("diff"); p.add_argument("a"); p.add_argument("b"); p.add_argument("--show", type=int, default=40); p.set_defaults(fn=cmd_diff)
    p = sub.add_parser("record"); p.add_argument("name"); p.add_argument("--save"); p.add_argument("--force", action="store_true")
    common(p); p.set_defaults(fn=cmd_record)
    a = ap.parse_args()
    if not hasattr(a, "jobs"):
        a.jobs = 1
    r = a.fn(a)
    sys.exit(r or 0)


if __name__ == "__main__":
    main()
