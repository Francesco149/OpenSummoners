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
  rhost.py record NAME [--save DIR|TGZ | --from SCEN]   HUMAN PLAY on the Windows desktop (realtime) → trace;
                                          --from = continue SCEN's play from its final save (Load at the title)
  rhost.py final-save SCEN [--refresh]    the user\\ dir (saves) SCEN's pure replay leaves (cached, _saves/)
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
def trace_meta(tr):
    """the trace's {"meta": {...}} line (merged if several)"""
    meta = {}
    for line in open(tr):
        line = line.strip()
        if line.startswith("{") and '"meta"' in line:
            meta.update(json.loads(line)["meta"])
    return meta


def scenario_inputs(name, a=None):
    """→ (trace path, save dir/tgz or None, edition).  A trace whose meta says "from": "SCEN" starts from the
    user\\ dir SCEN's replay leaves behind (its final save; derived on demand + cached, `final_save`)."""
    if name.endswith(".jsonl") and os.path.exists(name):
        tr = os.path.abspath(name)
        d = os.path.dirname(tr)
    else:
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
    meta = trace_meta(tr)
    if meta.get("from"):
        target = getattr(a, "target", None) or os.environ.get("RHOST_TARGET", "win")
        save, digest = final_save(meta["from"], target)
        want = meta.get("from_save")    # the recording's (Windows) start save
        if want and digest != want and target == "win":
            print(f"WARNING {name}: {meta['from']}'s final save is {digest}, the recording started from {want} "
                  f"(the replay may desync; a changed predecessor trace?)")
    return tr, save, ed


# ---------------------------------------------------------------- save chains (a scenario continuing another's play)
def save_digest(udir):
    """content hash of a user\\ dir (file names + bytes), 12 hex"""
    import hashlib
    h = hashlib.sha256()
    for f in sorted(os.listdir(udir)):
        p = os.path.join(udir, f)
        if os.path.isfile(p):
            h.update(f.lower().encode() + b"\0" + open(p, "rb").read() + b"\0")
    return h.hexdigest()[:12]


def save_slots(udir):
    """the slot saves a user\\ dir holds: a statue save is savedataNN.sdt (kind 10040 — the title's Load reads that
    kind); .dat = kind 10010, .bak = the overwrite backup"""
    return sorted(f for f in os.listdir(udir) if re.fullmatch(r"savedata\d\d\.(dat|sdt)", f, re.I))


def final_save(scen, target):
    """the user\\ dir a PURE replay of SCEN leaves behind (its saves + config.dat) → (dir, digest).
    Deterministic: a function of (trace, start save, edition, target); retail writes the saves itself, so no save
    file is ever committed — a chain is re-derivable from the corpus alone.  Cached in tests/scenarios/SCEN/_saves/
    (gitignored), keyed on those inputs; `rhost.py final-save SCEN --refresh` re-derives."""
    import hashlib
    tr, save, ed = scenario_inputs(scen, argparse.Namespace(target=target))
    k = hashlib.sha256(open(tr, "rb").read())
    k.update(f"|{ed or 'enold'}|{target}|".encode())
    if save:
        k.update((save_digest(save) if os.path.isdir(save) else hashlib.sha256(open(save, "rb").read()).hexdigest()).encode())
    cache = os.path.join(SCEN, scen, "_saves", f"final-{target}-{k.hexdigest()[:12]}")
    ok = cache + ".ok"     # beside the dir, not in it (the dir is staged as the run's user\\ verbatim)
    if os.path.isdir(cache) and os.path.exists(ok) and not FINAL_SAVE_REFRESH:
        return cache, open(ok).read().split()[0]
    print(f"[final save of {scen} ({target}): replaying it, pure retail]")
    # a clean run: no hooks / probes / seed override (the chain's save is canonical retail output)
    ra = argparse.Namespace(target=target, jobs=1, edition=None, wine_timeout=7200)
    batch, bdir, res, out = run_batch(ra, [(scen, tr, save, ed)], "final-save-" + scen)
    r = res.get(scen, {})
    if r.get("status") != "ok" or r.get("complete") != "1":
        raise SystemExit(f"final save of {scen}: the replay did not complete ({r or 'no result'}; {bdir})")
    if os.path.exists(cache):
        shutil.rmtree(cache)
    os.makedirs(os.path.dirname(cache), exist_ok=True)
    src = os.path.join(bdir, scen, "user")
    if target == "win":
        if not scp(f"{WIN}:C:/oss-rhost/runs/{batch}/{scen}/user", cache, recursive=True, check=False):
            raise SystemExit(f"final save of {scen}: the run left no user\\ dir ({bdir})")
    elif os.path.isdir(src):
        shutil.copytree(src, cache)
    else:
        raise SystemExit(f"final save of {scen}: the run left no user\\ dir ({bdir})")
    digest = save_digest(cache)
    open(ok, "w").write(f"{digest} {batch} f{r.get('frame')} sem={r.get('state_sem')}\n")
    return cache, digest


FINAL_SAVE_REFRESH = False


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
                    ("hash_every", "--hash-every"), ("vmmap_at", "--vmmap-at"), ("shot_at", "--shot-at"), ("shot_every", "--shot-every"),
                    ("log_every", "--log-every"), ("stall_ms", "--stall-ms")):
        v = getattr(a, k, None)
        if v is not None and v != "":
            args += [flag, str(v)]
    if getattr(a, "fb_hash", False):
        args += ["--fb-hash"]
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
    for name, trace, save, ed, *over in runs:      # over: [hook spec] — a per-run --hook (hookbisect)
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
        if over:
            saved_hook, a.hook = getattr(a, "hook", None), over[0]
            args = rhost_args(a, run_win, trace_win, ed)
            a.hook = saved_hook
        else:
            args = rhost_args(a, run_win, trace_win, ed)
        # the hook spec ALWAYS through a file (empty for a pure run): a 15 KB command line grew the CRT heap at
        # startup, and reading the file only in hybrid runs did too — either moved where the system DLLs' heaps
        # put the objects retail keeps pointers to (pure and hybrid states differed from frame 0)
        spec = ""
        if "--hook" in args:
            k = args.index("--hook")
            spec = args[k + 1]
            del args[k:k + 2]
        open(os.path.join(rdir, "hooks.txt"), "w").write(spec)
        args += ["--hook", f"@{run_win}\\hooks.txt"]
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
        # a new inode, never an in-place overwrite: a batch still running maps the old image (an in-place
        # write would swap its code pages under it)
        tmp = f"{dst}.{os.getpid()}.new"   # per process: two batches starting together staged through one name
        shutil.copy2(src, tmp)
        os.replace(tmp, dst)
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
    while os.path.exists(f"/tmp/.X{disp}-lock") or os.path.exists(f"/tmp/.X11-unix/X{disp}"):
        disp += 1   # another batch's Xvfb: two wineservers on one display die with BadWindow
    xvfb = subprocess.Popen(["Xvfb", f":{disp}", "-screen", "0", "1280x1024x16", "-nolisten", "tcp"],
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(1.0)
    env = dict(os.environ, WINEPREFIX=WINE_PREFIX, WINEDEBUG="-all", DISPLAY=f":{disp}")
    # a wineserver still up from an earlier batch keeps its desktop on THAT batch's (gone) display: new
    # processes on this display die with BadWindow — one batch per prefix, so it is shut down first
    subprocess.run(["wineserver", "-k"], env=env, capture_output=True)
    subprocess.run(["wineserver", "-w"], env=env, capture_output=True, timeout=60)

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


def resolve_runs(names, a=None):
    return [(os.path.splitext(os.path.basename(n))[0] if n.endswith(".jsonl") else n, *scenario_inputs(n, a)) for n in names]


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
    runs = resolve_runs(a.scenarios, a)
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
        tr, save, ed = scenario_inputs(a.scenario, a)
    if getattr(a, "learn_every", None):   # mid-run dumps too: slots alive only mid-run (the sem series checks them)
        ep = os.path.join(SCEN, a.scenario, "expected.json" if a.target == "win" else f"expected.{a.target}.json")
        last = int(json.load(open(ep))["frame"]) if os.path.exists(ep) else (a.frames or 0)
        mids = list(range(a.learn_every, last, a.learn_every))[:120]
        a.dump_at = ",".join(str(f) for f in mids) + ("," + a.dump_at if a.dump_at else "")
    a.dump_at = (a.dump_at + ",end") if a.dump_at else "end"
    runs = [(f"r{i}", tr, save, ed) for i in range(a.n)]
    batch, bdir, res, out = run_batch(a, runs, "det-" + os.path.basename(a.scenario), fetch=("rhost.log", "state_*.rhs"))
    print_results(res, [r[0] for r in runs])
    labels = sorted(f for f in os.listdir(os.path.join(bdir, "r0")) if f.startswith("state_") and f.endswith(".rhs")) \
        if os.path.isdir(os.path.join(bdir, "r0")) else []
    bad, learned = 0, set()
    for lab in labels or ["state_end.rhs"]:
        base = os.path.join(bdir, "r0", lab)
        for i in range(1, a.n):
            other = os.path.join(bdir, f"r{i}", lab)
            if not (os.path.exists(base) and os.path.exists(other)):
                print(f"--- r0 vs r{i} {lab}: dump missing"); bad += 1; continue
            print(f"--- r0 vs r{i} {lab}")
            rows = rhs.diff(base, other, show=20)
            real = [r for r in rows if r[3] not in ("ext-ptr(both)", "ext-ptr(stale)", "volatile-addr")]
            bad += len(real)
            learned |= {r[0] for r in rows if r[3] in ("ext-ptr(both)", "ext-ptr(stale)")}
        if getattr(a, "learn_every", None) and lab != "state_end.rhs":
            for i in range(a.n):   # 44 MB apiece: dropped once learnt
                q = os.path.join(bdir, f"r{i}", lab)
                if os.path.exists(q):
                    os.remove(q)
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


SERIES_EVERY = 600   # corpus --bless / --check: the state_sem every N frames (expected.json "sem_series")


def sem_series(log, target="wine"):
    """frame -> the series hash from an rhost.log's `hash fN ...` lines (--hash-every).  Wine: semv (also blind to
    values pointing into system-DLL regions: wine places system-heap objects by thread timing, its layout is fixed).
    Windows: sem (address-masked; the layout itself moves per run there, so value masking would not be stable)"""
    out = {}
    if os.path.exists(log):
        for m in re.finditer(r"^hash f(\d+) full=\w+ sem=(\w+) masked=\d+(?: semv=(\w+))?", open(log, errors="replace").read(), re.M):
            out[m.group(1)] = (m.group(3) if target == "wine" else None) or m.group(2)
    return out


def fb_series(log):
    """frame -> the back-buffer hash chain at that frame (`hash fN ... fb=` lines; --fb-hash with --hash-every)"""
    out = {}
    if os.path.exists(log):
        for m in re.finditer(r"^hash f(\d+) .* fb=(\w+)", open(log, errors="replace").read(), re.M):
            out[m.group(1)] = m.group(2)
    return out


def cmd_corpus(a):
    names = all_scenarios(a.only)
    if a.bless or a.check:
        a.fb_hash = True   # the render gate: every presented frame's pixels, chained (expected "fb" / "fb_series")
    if getattr(a, "hybrid", False):     # the hybrid gate: every ported function REPLACES retail
        hs = hybrid_hooks()
        a.hook = ",".join(["replace:" + h for h in hs] + ([a.hook] if a.hook else []))
        print(f"hybrid gate: {len(hs)} hooks replacing retail")
    if (a.bless or a.check) and not a.hash_every and getattr(a, "target", "win") == "wine":
        # the sem series (wine): a transient divergence a later room reload erases still shows.  Not on Windows:
        # its layout moves per run, and mid-run system pointers stay unmasked there (calibrating them means 16 GB
        # of dumps per scenario over the network) — the end state is the gate there
        a.hash_every = SERIES_EVERY
    runs = resolve_runs(names, a)
    batch, bdir, res, out = run_batch(a, runs, "corpus", fetch=("rhost.log",))
    print_results(res, names)
    done = sum(1 for n in names if res.get(n, {}).get("complete") == "1")
    print(f"CORPUS: {done}/{len(names)} complete; results in {bdir}")
    keys = ("frame", "tick", "rng", "state_sem", "fb")
    bad = 0
    for n in names:
        ep = os.path.join(SCEN, n, "expected.json" if getattr(a, "target", "win") == "win" else f"expected.{a.target}.json")
        got = {k: res.get(n, {}).get(k) for k in keys}
        series = sem_series(os.path.join(bdir, n, "rhost.log"), getattr(a, "target", "win"))
        fbs = fb_series(os.path.join(bdir, n, "rhost.log"))
        if a.bless and res.get(n, {}).get("status") == "ok":
            if series and a.hash_every == SERIES_EVERY:
                got["sem_series"] = series
            if fbs and a.hash_every == SERIES_EVERY:
                got["fb_series"] = fbs
            json.dump({k: v for k, v in got.items() if v is not None}, open(ep, "w"), indent=1); continue
        if a.check and os.path.exists(ep):
            exp = json.load(open(ep))
            diff = {k: (exp.get(k), got[k]) for k in keys if (k in exp or k != "fb") and exp.get(k) != got[k]}
            ef = exp.get("fb_series") or {}
            ff = next((f for f in sorted(ef, key=int) if f in fbs and fbs[f] != ef[f]), None)
            if ff is not None:   # the earliest sampled frame by which a presented frame's pixels differ
                diff[f"fb@f{ff}"] = (ef[ff], fbs[ff])
            es = exp.get("sem_series") or {}
            first = next((f for f in sorted(es, key=int) if f in series and series[f] != es[f]), None)
            if first is not None:   # the earliest frame whose state differs (+ how many sampled frames differ)
                nd = sum(1 for f in es if f in series and series[f] != es[f])
                diff[f"sem@f{first}"] = (es[first], f"{series[first]} ({nd}/{len(es)} sampled frames differ)")
            elif es and not series:
                print(f"  note {n}: no sem series in this run (--hash-every {SERIES_EVERY} missing?)")
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
                "room_formation", "char_follow_set", "actor_state_restore", "palette_tint@16", "map_res_load",
                "slot_frames_free@4", "slot_frames_free_all", "slots_free_graded", "pal_swap_apply", "room_populate", "structure_init", "entity_set_handle", "device_init", "entity_snd_free",
                "char_skill_find", "char_ground_probe", "voice_set_free", "voice_set_stop", "tod_off", "grid_f1c", "stat_jitter", "char_stats_init", "char_idle_init", "char_idle_add", "char_member_skill_add", "char_combo_add", "char_clips_set", "char_clips2_set", "char_carry_set", "char_step_set", "char_pace_set", "char_pace_raw", "char_c880_set", "char_move_set", "char_15a5c_set", "char_turn_set", "char_walk_set", "char_jump_set", "char_run_set", "char_hit_set", "char_member_pose_set", "char_crouch_set", "char_nav_skills_set", "char_approach_add", "char_skills_by_code", "char_skill_learn", "char_skill_def", "char_nav_set", "char_retarget_set", "char_ai_skill_add", "ai_skill_boxes_add", "char_voice_init", "char_npc_default", "char_form0_set", "char_party_mode_set", "char_idle_voice_set", "char_init",
                "member_create", "member_attach", "pslot_code", "member_palette", "char_skills_refresh", "char_skill_usable",
                "member_skill_at", "slot_entries_free", "slot_clone", "ctrl_geometry_free",
                "scene_obj_init", "flag_find", "flag_add", "room_flag_set", "pslot_reset", "room_party_defaults", "room_inputs_alloc", "room_delete", "area_find", "input_init", "room_area_enter", "fill_clear_mapped", "room_has_retry", "room_retry_point", "keep_find", "room_next_set", "room_party_heal", "room_gold_add", "roomloop_new", "room_from_scene", "obj_delete", "obj_free", "pslot_input_set", "keep_snapshot", "room_end_cleanup", "roomloop_pools_free", "roomloop_free", "session_end", "area_xref", "frame_cache_clear", "file_read_section", "pool_obj_delete", "world_delete", "room_to_scene", "scene_start",
                "pslot_by_id", "dramatist_row", "area_level_row", "pslot_heal", "room_party_level", "pslot_init", "member_skills_learn", "room_items_grant", "room_leader_set", "room_story_setup", "room_story_setup2", "skill_row_find", "item_fits", "pslot_status_set", "pslot_revive_mark", "item_stats_add", "room_stats_refresh", "room_item_equip", "room_party_dismiss", "room_member_leave", "room_member_join", "room_member_add", "room_item_add", "stat_row_find", "char_profile_find", "stats_recompute", "part_facing", "arche_pose", "char_status_inflict", "char_knockdown", "body_switch_touch", "char_drift", "char_drift_move", "particle_live", "room_hitstop_set", "switch_touch", "char_room_logs", "script_engine", "script_begin", "script_end", "script_flag", "script_leader", "script_party_cast", "script_cast_add", "script_wait", "actor_by_handle", "scene_msg", "scene_msg_rec", "scene_face", "scene_face_rec", "scene_party_stance", "scene_talk_face", "script_flag_set", "scene_camera", "scene_next_event", "scene_pause", "scene_mark", "scene_fade_out", "scene_fade_in", "scene_wait", "scene_wait_fade", "scene_area_title", "scene_place_at_door", "scene_spawn_at_door", "scene_place_rec", "room_bgm_restart", "char_party_ai_skills", "char_status_tick", "char_fade_tick", "char_status_decay", "char_hit_queue", "world_log_line", "entity_part_descs", "entity_sprite_desc", "shot_spawn", "room_warp_tick", "room_leader_next", "char_lead_ok", "char_on_ground", "room_pickups", "pickup_touch", "item_sparkle", "room_party_tick", "room_item_pick", "quest_count", "room_views_refresh", "world_notice_reset", "win_mgr_topmost", "win_mgr_open", "win_init", "win_ctrl_take", "win_ctrl_at", "grid_col_place", "grid_col_style", "ui_ref_set", "ui_link_drop", "win_text_add", "ctrl_text_init", "win_grid_add", "grid_row_reset", "text_metrics_set", "ctrl_cursor_set", "icon_set", "sys_frame_style", "sys_cursor_style", "grid_page_fit", "ui_scroll_init", "money_text", "money_value", "room_item_count", "room_party_count", "grid_cell_text", "glyph_recs_set", "text_parse", "text_wrap", "text_layout_set", "ctrl_text_set", "ctrl_keys_alloc", "ctrl_key_add", "ctrl_keys_expand", "sys_msg_keys", "sys_box_cursor", "glyph_token_search", "ctrl_grid_init", "win_grid_new", "grid_row_add", "cell_icon_set", "win_title_add", "sys_win_open", "sys_page_arrows", "menu_msg_open", "menu_party_open", "menu_party_fill", "menu_have_counts", "menu_item_fits", "menu_sprites_free", "icon_tick", "text_key", "grid_nav", "ctrl_key", "ctrl_tick", "win_tick", "msg_update", "ctrl_input", "jobs_busy",
                "grid_gauge_clamp", "cell_color_set", "grid_row_text_add", "sys_list_open", "item_def", "room_views_mark",
                "member_rows_add", "stat_row_set", "member_sprite", "equip_panel_fill",
                "cell_icon_alpha", "sys_balloon_style", "sys_box_style", "view_project", "box_place", "box_fade_tick",
                "body_near_x", "cast_face_cmd", "path_dfs", "path_search", "cast_step",
                "win_anim", "win_slide", "mgr_win_to_back", "win_ctrls_free", "fx_rows_center_out", "fx_rows_edges_in",
                "fx_rows_sweep_down", "fx_rows_sweep_up", "overlay_fade", "overlay_tick", "win_ctrl_to_back", "win_anim_set",
                "notice_tick", "hud_ease", "stats_mp_ratio", "hud_status_icon", "hud_status_drop", "hud_view_tick",
                "view_box_visible", "screen_step",
                "item_kind", "room_items_of_kind", "room_items_stored", "key_token_replace", "menu_msg_set",
                "msg_window_open", "sys_msg_style", "item_list_fill",
                "cast_cmd_clear", "scene_cast_to", "scene_cast_to_rec", "scene_cast_step", "scene_party_gather",
                "scene_device_spawn", "char_unload", "script_switch",
                "facing_back", "char_mag_sum", "char_atk_roll", "char_skill_roll", "char_lunge", "char_attack_hits",
                "body_hit_spawn", "char_hit_target",
                "angle_wrap", "char_particle", "entity_slide_step", "orbit_init", "orbit_step@16", "body_pose_set",
                "slash_step@16", "slash_trail", "crate_push@256",
                "hit_breaks_shots", "shot_end", "foe_gauge_set", "foe_gauge_near",
                "char_mag_roll", "clip_seq_step@4", "body_pose_swap", "char_shot_fire", "particle_tint_set",
                "entity_sprite_descs@4", "voice_group_prio", "sana_skills@4",
                "char_take_hit", "hit_combo", "dmg_party_scale", "dmg_record", "dmg_glyphs", "crate_knock",
                "char_die", "foe_drops", "pickup_spawn", "exp_share", "float_text", "char_part_release",
                "mon_slime", "mon_skill51", "mon_skill29", "ai_rec_hold_set", "char_skill_list_add",
                "arche_sword@4", "arche_combo_skill",
                "skill_cast@4", "skill_charge", "skill_stage", "skill_mp_pay", "summon_recall", "summon_room_ok",
                "skill_space_ahead", "summon_init", "summon_spawn",
                "shots_tick@16", "room_band64_tick@16", "shot_init", "band96_tick", "shot_449700", "shot_4497b0", "shot_4538f0", "shot_46e190", "shot_476500", "shot_476550", "shot_476740", "shot_476a80", "shot_476ab0", "shot_476d30", "shot_476e20", "shot_476ef0", "shot_477070", "shot_4771d0", "shot_477310", "shot_477560", "shot_477650", "shot_477b70", "shot_477f10", "shot_4781f0", "shot_478550", "shot_4785a0", "shot_478820", "shot_54cab0", "shot_54cca0", "shot_55f150",
                "beh_441f50", "beh_47d790", "beh_47da60", "beh_47de50", "beh_47e010", "beh_47e7c0", "beh_47eae0", "beh_47ecd0", "beh_47f670", "beh_47f9d0", "beh_481310", "beh_481ac0", "beh_482240", "beh_482380", "beh_482460", "beh_482880", "beh_482ac0", "beh_482e10", "beh_4834f0", "beh_483b50", "beh_483de0", "beh_483fb0", "beh_484530", "beh_484ea0", "beh_485350", "beh_4854e0", "beh_485990", "beh_442490", "beh_47ae00", "beh_47f610", "beh_47fd40", "beh_480b30", "beh_48bdd0", "beh_48be60", "beh_479e40", "beh_481790", "beh_481a20", "beh_479e10", "beh_47a970", "beh_4826b0", "beh_482700", "beh_482760", "beh_483040", "beh_484830", "beh_484d60", "beh_485330", "beh_483920", "beh_4848a0", "beh_442640", "beh_4843c0", "beh_442710", "beh_485cb0", "beh_47b870", "beh_48b920", "beh_5b4200", "beh_480260", "beh_480580", "beh_4832e0", "beh_484bc0",
                "ih_455d80", "ih_44d2e0", "ih_4512d0", "ih_4466b0", "ih_467260", "item_effect", "ih_5b28a0",
                "sc_401d80", "sc_402030", "sc_4020a0", "sc_4024e0", "sc_402610", "sc_402f40", "sc_406500", "sc_406ab0", "sc_406c70", "sc_41a3a0", "sc_41e2b0", "sc_41e340", "sc_41eb30", "sc_41ece0", "sc_41efe0", "sc_41f050", "sc_439510", "sc_439560", "sc_49cf60", "sc_49cf80", "sc_49e250", "sc_49e6c0", "sc_4a5e40", "sc_4a5e70", "sc_4a5ea0", "sc_4a5ec0", "sc_4a5f80", "sc_4a63d0", "sc_4ac610", "sc_4ac660", "sc_4b3b10", "sc_4b3f30", "sc_402f60", "sc_4b4570", "sc_4bbe00", "sc_4bbe40", "sc_4bbe80", "sc_4c5300", "sc_4cbfb0", "sc_4cbfd0", "sc_41f000", "sc_4cbff0", "sc_4cc010", "sc_4cc030", "sc_4cc250", "sc_4cc470", "sc_4cca20", "sc_4cca50", "sc_4d6490", "sc_5b4330",
                "fm_591590", "fm_5984c0", "fm_598460", "fm_56c430",
                "sv_55f620", "sv_561bf0", "sv_561cf0", "sv_560530", "sv_41cfd0",
                "ai_479f90", "si_409520", "si_41d4a0", "si_40a2a0"]
# REPLACE-only hooks (a whole room per call / DirectInput): proven by `corpus --check --hook replace:…`
# (chs_49f7c0: ported but OFF — diverges on rh-errands-items from f7835: 12-byte records at 0x20b60398..; open)
REPLACE_ONLY_HOOKS = ["chs_4d7d80", "chs_4dc510", "chs_4d64f0", "chs_4cd6d0", "chs_4d13a0", "chs_4ccc90", "chs_4c6b40", "chs_4e19e0", "chs_49d930", "chs_4a0c90", "chs_4a6b80", "chs_4ac6d0", "chs_4a66a0", "chs_4a5fa0", "chs_4ae150", "chs_4ae420", "chs_4afdb0", "chs_4b3b20", "chs_4b3c70", "chs_4b4d40", "chs_4bbef0", "chs_4bd2e0", "chs_4bd6b0", "chs_4bd820", "chs_4bdf90", "chs_4bdbe0", "chs_4be220", "chs_4c1a30", "chs_4c1c20", "chs_4c2310", "chs_4c29c0", "chs_4c2dc0", "chs_4c3ee0", "chs_4c45a0",
                      "rd_417c40", "rd_418470", "rd_43e250", "rd_48c150", "rd_48c820", "rd_491820", "rd_494e10", "rd_495dc0", "rd_495e40", "rd_4961e0", "rd_496240",
                      "rp_5b5eb0", "rp_5b8b40", "rp_5b9350", "rp_5b9390", "rp_5b95c0", "rp_5b97e0", "rp_5b9830", "rp_5b98c0", "rp_5b9450", "rp_5b9490", "rp_5b94d0", "rp_5b94e0", "rp_5b9500", "rp_5b9520", "rp_5b9a40", "rp_5b9ae0", "rp_5b9b70", "rp_5b9bf0", "rp_5bd550", "rp_5bd680",
                      "tm_41bbe0", "tm_562ea0", "tm_5642e0", "tm_5644a0", "tm_564690", "tm_564780", "tm_419900", "tm_5658c0", "tm_565d10", "tm_566250", "tm_566350", "tm_566a80", "tm_56da40", "tm_56daa0", "tm_56dc20", "tm_56a220", "tm_41c150", "tm_41c240", "tm_560a40", "tm_41c2f0", "tm_41c320", "tm_560a00", "tm_562a10", "tm_41bb80", "tm_56c2b0", "tm_562af0", "tm_40bb70", "tm_56d710", "tm_41d780", "tm_49a6e0",
                      "fm_591120", "fm_596d60", "fm_597aa0", "fm_5985f0", "fm_590480", "fm_590f20", "fm_590ca0", "fm_59d540", "fm_59ad60",
                      "sv_41a430", "sv_41a890", "sv_41b670", "sv_41bd20", "sv_41bf60", "sv_561560", "sv_561d20",
                      "si_408730", "si_40a340", "si_41a160", "si_408b90", "si_40a5d0",
                      "el_401df0", "el_402890", "el_407f30", "el_40a2e0", "el_40ac90", "el_415ae0", "el_4192f0", "el_419980", "el_41b470", "el_41c390", "el_41cb10", "el_41db30", "el_41dba0", "el_41e070", "el_41e4c0", "el_41f0c0", "el_426110", "el_438970", "el_43c1b0", "el_43d4b0", "el_43d540", "el_43d830", "el_43dd30", "el_43e010", "el_441de0", "el_4428f0", "el_44bf60", "el_4675c0", "el_467750", "el_468970", "el_479ca0", "el_47a300", "el_47a420", "el_47a7f0", "el_49d330", "el_49eea0", "el_4c5af0", "el_4cc0e0", "el_54ed80", "el_5534d0", "el_556e50", "el_55fdb0",
                      "lo_479960", "lo_479a10", "lo_479de0", "lo_406440", "lo_401140", "lo_4011b0", "lo_4013a0", "lo_401730",
                      "ts_40fe00", "ts_410250", "ts_54c530", "ts_55ff80", "ts_5612f0", "ts_5640b0", "ts_564110", "ts_565b00", "ts_566300", "ts_568780", "ts_569220", "ts_56a270", "ts_56a450", "ts_56a670", "ts_56aa70", "ts_56ad50", "ts_56d620", "ts_56dd80", "ts_583fe0", "ts_584400", "ts_584420", "ts_584550", "ts_5846f0", "ts_584710", "ts_585260", "ts_585400", "ts_585530", "ts_585590", "ts_585810", "ts_585ae0", "ts_585e70", "ts_59d160", "ts_59d690", "ts_59d900", "ts_59dc60", "ts_59df50", "ts_59e1d0", "ts_59e230", "ts_59e5d0", "ts_59e9e0",
                      "en_410090", "en_49bf90", "en_5646c0", "en_56c640", "en_56c900", "en_56dbb0", "en_5a00c0", "en_5a3670", "en_5a3870", "en_5a4320", "en_5a4440", "en_5a45c0",
                      "gp_468b10", "gp_46cb40", "gp_46cd10",
                      "sj_4034f0", "sj_4051d0",
                      "ab_54f140", "ab_552c70", "ab_552f60", "ab_553520", "ab_553ec0", "ab_554570", "ab_554c40", "ab_5554e0", "ab_5556d0",
                      "ih_445e50", "ih_446900", "ih_4484b0", "ih_448dc0", "ih_449810", "ih_44a160", "ih_44a200", "ih_44a740", "ih_44b160", "ih_44bc10", "ih_44c250", "ih_44d8f0", "ih_44e1c0", "ih_44e730", "ih_44e8d0", "ih_44f690", "ih_450810", "ih_4516e0", "ih_452020", "ih_452730", "ih_4539b0", "ih_455c90", "ih_455cf0", "ih_455d20", "ih_456d30", "ih_45e260", "ih_45e3e0", "ih_461640", "ih_467330",
                      "ci_428c00", "ci_429060", "ci_429d50", "ci_429db0", "ci_42a500", "ci_42a9a0", "ci_42b550", "ci_42b580", "ci_42b5f0", "ci_42b840", "ci_42b890", "ci_42b9a0", "ci_42c130", "ci_42c510", "ci_42cce0", "ci_42d3c0", "ci_42da50", "ci_42f840", "ci_42ff00", "ci_430400",
                      "mm_5915c0", "mm_592020", "mm_592190", "mm_592850", "mm_593d90", "mm_593e40", "mm_593f60", "mm_5943e0", "mm_594a20", "mm_594c80", "mm_594f40", "mm_595320", "mm_595350", "mm_5956c0", "mm_595dd0", "mm_5962a0", "mm_5963a0", "mm_596620", "mm_596ac0", "mm_596b50", "mm_597480", "mm_5974b0", "mm_597670", "mm_598d00", "mm_5990b0", "mm_599140", "mm_5994e0", "mm_599a60", "mm_599ac0", "mm_599e60",
                      "mh_418ec0", "mh_4190d0", "mh_4199b0", "mh_419c70", "mh_419eb0", "mh_4e61d0", "mh_4e6520", "mh_4e6870", "mh_5b1f10", "mh_5b35f0", "mh_5b3850", "mh_409150",
                      "wn_5a4770", "wn_562210", "wn_5624c0", "wn_562b50", "wn_562b70", "wn_560900", "wn_58ffa0", "wn_5b9ab0", "wn_5b9ac0",
                      "al_56e190", "al_57ca40", "al_5752e0", "al_57b280", "al_57a330", "al_582e90", "al_5749b0", "al_5748c0", "al_579680", "al_579740", "al_579860", "al_5798f0", "al_579a00", "al_579bd0", "al_579ec0", "al_579f40", "al_57a030", "al_57a1a0", "al_57a260", "al_582b80", "al_582d00", "al_582d10", "al_583c90", "al_583ee0", "al_563ef0",
                      "wm_5b12e0", "id_5ba200", "wm_5afb00", "wm_5afb90", "wm_5afbf0", "wm_5afc90", "wm_5aff00", "wm_5aff20", "wm_5affe0", "wm_5b01e0", "wm_5b0260", "wm_5b0280", "wm_5b0390", "wm_5b03e0", "wm_5b0450", "wm_5b0890", "wm_5b0c60", "wm_5b0ce0", "wm_5b0d70", "wm_5b0dd0", "wm_5b0ee0", "wm_5b1030", "wm_5b10d0", "wm_5b14c0", "wm_5b5a60", "wm_5b5a90", "wm_5b5ac0", "wm_5b5af0", "wm_5b5d50", "wm_5b5d90", "wm_5b5dc0", "wm_5b5ef0", "wm_5b5f50",
                      "zd_5b7ee0", "zd_5b7f80", "zd_5b7fe0", "zd_5b8040", "zd_5b80d0", "zd_5b8480", "zd_5b88c0", "zd_5b8900", "zd_5b8950", "zd_5b89d0", "zd_5b8a20", "zd_5b8ae0", "zd_5b8da0", "zd_5b8dd0", "zd_5b8e00", "zd_5b8ea0", "zd_5b8fc0", "zd_5b9130", "zd_5b91d0", "zd_5b9240", "zd_5b9410", "zd_5b9740", "zd_5bcff0", "zd_5bd020", "zd_5bd040", "zd_5bd380", "zd_5bd3b0", "zd_5bd3d0", "zd_5bd4d0", "zd_5bd520",
                      "sd_5ba740", "sd_5ba7e0", "sd_5ba820", "sd_5ba850", "sd_5ba860", "sd_5ba9a0", "sd_5baa60", "sd_5baa80", "sd_5baaa0", "sd_5bab10", "sd_5bad20", "sd_5bad40", "sd_5bad60", "sd_5bad80", "sd_5badd0", "sd_5bae00", "sd_5bae30", "sd_5bae70", "sd_5bae90", "sd_5baec0", "sd_5baed0", "sd_5bafa0", "sd_5bafe0", "sd_5bb040", "sd_5bb180", "sd_5bb250", "sd_5bb2f0", "sd_5bb380", "sd_5bb3a0", "sd_5bb3d0", "sd_5bb5c0", "sd_5bb610", "sd_5bb740", "sd_5bb870", "sd_5bb880", "sd_5bb890", "sd_5bb8c0", "sd_5bb8e0", "sd_5bb8f0", "sd_5bb930", "sd_5bb9b0", "sd_5bbb10", "sd_5bbb50", "sd_5bbc20", "sd_5bbc60", "sd_5bbc90", "sd_5bbcd0", "sd_5bbd20", "sd_5bbdb0", "sd_5bbe20", "sd_5bbeb0", "sd_5bc0c0", "sd_5bc150", "sd_5bc7c0", "sd_5bcb30", "sd_5bcb80", "sd_5bcdf0", "sd_5bce40", "sd_5bce80", "sd_5bceb0", "sd_5bcf30", "sd_5bcfa0", "sd_5bcfb0",
                      "id_5b9cf0", "id_5b9d90", "id_5b9de0", "id_5b9e30", "id_5b9f80", "id_5b9fc0", "id_5ba070", "id_5ba120", "id_5ba1c0", "id_5ba220", "id_5ba240", "id_5ba290", "id_5ba2b0", "id_5ba2d0", "id_5ba330", "id_5ba3a0", "id_5ba3b0", "id_5ba3d0", "id_5ba3f0", "id_5ba410", "id_5ba4c0", "id_5ba520", "id_5ba540", "id_5ba650", "id_5ba6a0",
                      "fs_5b6060", "fs_5b60d0", "fs_5b60e0", "fs_5b61b0", "fs_5b6200", "fs_5b6250", "fs_5b62a0", "fs_5b6340", "fs_5b6410", "fs_5b64b0", "fs_5b64e0", "fs_5b64f0", "fs_5b6520", "fs_5b6580", "fs_5b65a0", "fs_5b65b0", "fs_5b6630", "fs_5b6650", "fs_5b66a0", "fs_5b66b0", "fs_5b66f0", "fs_5b67c0", "fs_5b6890", "fs_5b68f0", "fs_5b6940", "fs_5b6990", "fs_5b6bc0", "fs_5b6db0",
                      "fl_4178e0", "fl_4184a0", "fl_4188b0", "fl_499850", "bs_5b6e40", "bs_5b6e70", "bs_5b6e80", "bs_5b6e90", "bs_5b6eb0", "bs_5b6ec0", "bs_5b6ee0", "bs_5b6ef0", "bs_5b6f00", "bs_5b6f10", "bs_5b6f30", "bs_5b6f80", "bs_5b71f0", "bs_5b7270", "bs_5b7310", "bs_5b74f0", "bs_5b7620", "bs_5b7800", "bs_5b7970", "bs_5b7b90", "bs_5b7bd0", "bs_5b7c10", "bs_5b7cd0", "bs_5b9280", "bs_5b9630", "bs_5b9910", "bs_5ba390",
                      "room_loop", "input_repoll_flush", "load_room", "scene_loop", "menu_pace", "menu_run",
                      "shop_run", "shop_buy", "shop_who", "menu_ctx_clear", "scene_run",
                      "world_new", "world_free",
                      "menu_party_close", "msg_pages", "storage_deposit", "storage_withdraw", "storage_run",
                      "item_find", "furniture_search", "scene_yes_no", "item_give", "book_read", "sprite_set_free"]   # (their shadows skip: the grid's 2.9 MB block moves the arena commit)
SHADOW_ONLY_HOOKS = ["map_decode"]     # its REPLACE form is the load_room port (the decode needs the recorded resource)
# name@N = shadow every Nth call (the full-state shadows cost ~10-20 ms per compared call)


def cmd_verify(a):
    names = all_scenarios(a.only)
    want = {h.split("@")[0]: h for h in (a.hooks or "").split(",") if h}   # `name@N` overrides the sampling
    hooks = [want.get(h.split("@")[0], h) for h in VERIFY_HOOKS if not want or h.split("@")[0] in want]
    a.hook = ",".join("shadow:" + h for h in hooks)
    runs = resolve_runs(names, a)
    k = max(1, a.split)
    if k > 1:      # the shadows are independent (retail stays authoritative): each scenario × k hook subsets in parallel
        runs = [(f"{r[0]}~{i}", *r[1:4], ",".join("shadow:" + h for h in hooks[i::k])) for r in runs for i in range(k)]
    batch, bdir, res, out = run_batch(a, runs, "verify")
    if k > 1:
        merged = {}
        for key, v in res.items():
            base, _, rest = key.partition("#")
            n = base.split("~")[0] + ("#" + rest if rest else "")
            if rest:
                merged.setdefault(n, []).extend(v)
            elif n not in merged or v.get("complete") != "1":
                merged[n] = v
        res = merged
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
    for hs in hooks:
        h = hs.split("@")[0]
        c, d, sk, sc, first = tot.get(h, [0, 0, 0, set(), ""])
        print(f"{hs:22s} {c:9d} {d:8d} {sk:8d} {len(sc)}" + (f"  FIRST: {first}" if d else ""))
    return 0 if all(tot.get(h.split("@")[0], [0, 0])[1] == 0 for h in hooks) else 1


def cmd_coverage(a):
    tsv = os.path.join(IDA_DIR, a.edition or "enold", "functions_ida.tsv")
    vas = [l.split("\t")[0] for l in open(tsv).readlines()[1:]]
    cov_in = os.path.join(LOCAL_RUNS, "coverage_in.txt")
    os.makedirs(LOCAL_RUNS, exist_ok=True)
    open(cov_in, "w").write("\n".join(vas) + "\n")
    a.coverage_file = cov_in
    names = all_scenarios(a.only) if not a.scenarios else a.scenarios
    runs = resolve_runs(names, a)
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
    tr, save, ed = scenario_inputs(a.scenario, a)
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


def hybrid_hooks():
    return [h for h in dict.fromkeys([h.split("@")[0] for h in VERIFY_HOOKS] + REPLACE_ONLY_HOOKS) if h not in SHADOW_ONLY_HOOKS]


def cmd_hookbisect(a):
    """which REPLACE hook makes SCEN diverge from pure retail by --frames?  One batch per round: a pure
    reference + N groups of the candidates (each group replaced alone); the diverging groups split again
    until single hooks remain.  Compares frame / tick / rng (+ state_sem on the Windows target)."""
    cand = a.hooks.split(",") if a.hooks else hybrid_hooks()
    tr, save, ed = scenario_inputs(a.scenario, a)
    # wine: semv (blind to system-heap values); Windows: state_sem only at a scenario's end (mid-run it is not
    # stable there: system pointers the end-state calibration never saw)
    keys = ("frame", "tick", "rng") + (("state_semv",) if a.target == "wine" else () if a.frames else ("state_sem",)) \
        + (("fb",) if getattr(a, "fb_hash", False) else ())
    rnd = 0
    while True:
        rnd += 1
        n = min(a.groups, len(cand))
        groups = [cand[i::n] for i in range(n)]
        runs = [("ref", tr, save, ed, "")] + [(f"g{i}", tr, save, ed, ",".join("replace:" + h for h in g)) for i, g in enumerate(groups)]
        batch, bdir, res, out = run_batch(a, runs, f"bisect{rnd}-" + a.scenario)
        ref = {k: res.get("ref", {}).get(k) for k in keys}
        if not ref["rng"]:
            raise SystemExit(f"round {rnd}: no reference result")
        bad = []
        for i, g in enumerate(groups):
            got = {k: res.get(f"g{i}", {}).get(k) for k in keys}
            div = got != ref
            print(f"  round {rnd} g{i} ({len(g)} hooks{': ' + g[0] if len(g) == 1 else ''}): {'DIVERGE ' + str(got) if div else 'ok'}")
            if div:
                bad.append(g)
        if not bad:
            print(f"no single group diverges alone (an interaction of hooks across groups?): candidates {len(cand)}")
            return 1
        singles = [g[0] for g in bad if len(g) == 1]
        if singles:
            print("DIVERGING HOOK(S): " + ", ".join(singles))
            return 0
        cand = bad[0]


def cmd_diff(a):
    import rhs
    rows = rhs.diff(a.a, a.b, show=a.show)
    print(f"{len(rows)} differing dwords")


def cmd_record(a):
    """human play: visible window on the Windows desktop, realtime, real keyboard.  --from SCEN: boot with the
    saves SCEN's play left behind (its final user\\ dir) — Load the slot at the title and keep playing; the trace
    records "from" and replays re-derive that save from SCEN's trace (nothing to commit but the trace)."""
    name = a.name
    out_dir = os.path.join(SCEN, name)
    if os.path.exists(os.path.join(out_dir, "trace.jsonl")) and not a.force:
        raise SystemExit(f"{out_dir}/trace.jsonl exists (use --force)")
    if a.save and getattr(a, "from_scen", None):
        raise SystemExit("--save and --from are exclusive")
    a.target = "win"      # a human at the Windows desktop
    save, digest, ed = a.save, None, a.edition
    if getattr(a, "from_scen", None):
        save, digest = final_save(a.from_scen, "win")
        ed = ed or scenario_inputs(a.from_scen, a)[2]
        slots = save_slots(save)
        print(f"starting from {a.from_scen}'s final save {digest}: " +
              (f"slot(s) {', '.join(slots)} — Load at the title" if slots else
               "NO slot save (the predecessor never saved at a statue; only its config / system data carry over)"))
    ensure_staged()
    a.show = True
    a.extra = ((a.extra or "") + f" --record trace.jsonl").strip()
    a.jobs = 1
    runs = [(name, None, save, ed)]
    batch, bdir, res, out = run_batch(a, runs, "record-" + name, fetch=("rhost.log", "trace.jsonl"))
    print_results(res, [name])
    src = os.path.join(bdir, name, "trace.jsonl")
    if not os.path.exists(src):
        raise SystemExit("no trace recorded")
    os.makedirs(out_dir, exist_ok=True)
    meta = {"recorded": datetime.datetime.now().isoformat(timespec="seconds"), "edition": ed or "enold"}
    if digest:
        meta.update({"from": a.from_scen, "from_save": digest})
    with open(os.path.join(out_dir, "trace.jsonl"), "w") as f:
        f.write(json.dumps({"meta": meta}) + "\n")
        if a.save:
            f.write(f'{{"savefile": "{os.path.relpath(a.save, out_dir)}"}}\n')
        f.write(open(src).read())
    print(f"recorded → {out_dir}/trace.jsonl (verify: rhost.py det {name})")
    udir = os.path.join(bdir, name, "user")
    if scp(f"{WIN}:C:/oss-rhost/runs/{batch}/{name}/user", udir, recursive=True, check=False):
        slots = save_slots(udir)
        print(f"this play ends with slot save(s): {', '.join(slots)} — continue it with `rhost.py record NEXT --from {name}`"
              if slots else "this play left NO slot save: save at a statue before quitting to continue it later (--from)")


def cmd_final_save(a):
    global FINAL_SAVE_REFRESH
    FINAL_SAVE_REFRESH = a.refresh
    d, digest = final_save(a.scenario, a.target)
    print(f"{a.scenario} final save ({a.target}): {d}\n  digest {digest}; slots: {', '.join(save_slots(d)) or 'none'}; "
          f"files: {', '.join(sorted(os.listdir(d)))}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    def common(p):
        p.add_argument("--frames", type=int)
        p.add_argument("--seed")
        p.add_argument("--hook")
        p.add_argument("--dump-at")
        p.add_argument("--vmmap-at")
        p.add_argument("--shot-at")
        p.add_argument("--shot-every", type=int)
        p.add_argument("--hash-every", type=int)
        p.add_argument("--log-every", type=int)
        p.add_argument("--stall-ms", type=int)
        p.add_argument("--fb-hash", action="store_true", help="chain a hash of every Flip's back buffer (RESULT fb=; the render gate)")
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
    p.add_argument("--calibrate", action="store_true")
    p.add_argument("--learn-every", type=int, help="also dump + compare every N frames (mid-run slots; with --calibrate)")
    common(p); p.set_defaults(fn=cmd_det)
    p = sub.add_parser("corpus"); p.add_argument("--only"); p.add_argument("--bless", action="store_true")
    p.add_argument("--check", action="store_true")
    p.add_argument("--hybrid", action="store_true", help="replace: every VERIFY_HOOKS + REPLACE_ONLY_HOOKS function (the hybrid gate)")
    common(p); p.set_defaults(fn=cmd_corpus)
    p = sub.add_parser("verify"); p.add_argument("--only"); p.add_argument("--hooks", help="only these VERIFY_HOOKS names")
    p.add_argument("--split", type=int, default=1, help="run each scenario N times in parallel, 1/N of the hooks each")
    common(p); p.set_defaults(fn=cmd_verify)
    p = sub.add_parser("hookbisect"); p.add_argument("scenario"); p.add_argument("--hooks", help="candidate hook names (default: the hybrid set)")
    p.add_argument("--groups", type=int, default=8); common(p); p.set_defaults(fn=cmd_hookbisect)
    p = sub.add_parser("coverage"); p.add_argument("--only"); p.add_argument("scenarios", nargs="*"); common(p); p.set_defaults(fn=cmd_coverage)
    p = sub.add_parser("portshots"); p.add_argument("scenario"); p.add_argument("--at", required=True)
    p.add_argument("--port-realtime", action="store_true", help="run the port on its wall clock instead of --det-clock")
    p.add_argument("--search", type=int, help="also find the best-matching retail flip within ±W of each port frame")
    p.add_argument("--port-extra", help="extra port args, e.g. --port-extra=--boot-loading")
    common(p); p.set_defaults(fn=cmd_portshots)
    p = sub.add_parser("diff"); p.add_argument("a"); p.add_argument("b"); p.add_argument("--show", type=int, default=40); p.set_defaults(fn=cmd_diff)
    p = sub.add_parser("record"); p.add_argument("name"); p.add_argument("--save"); p.add_argument("--force", action="store_true")
    p.add_argument("--from", dest="from_scen", help="start from SCEN's final save (its replay's user\\ dir); the trace records it")
    common(p); p.set_defaults(fn=cmd_record)
    p = sub.add_parser("final-save"); p.add_argument("scenario"); p.add_argument("--refresh", action="store_true")
    common(p); p.set_defaults(fn=cmd_final_save)
    a = ap.parse_args()
    if not hasattr(a, "jobs"):
        a.jobs = 1
    r = a.fn(a)
    sys.exit(r or 0)


if __name__ == "__main__":
    main()
