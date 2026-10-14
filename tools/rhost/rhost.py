#!/usr/bin/env python3
"""rhost.py — driver for the in-process retail host (tools/rhost) on the Windows box.

  rhost.py build                          build build/rhost/rhost.exe (nix devshell mingw)
  rhost.py stage                          upload rhost.exe + runner scripts to C:\\oss-rhost
  rhost.py setup                          one-time: stage + install the runner task + copy game data
  rhost.py run SCEN|TRACE.jsonl [..]      replay scenario(s) (tests/scenarios/<SCEN>/trace.jsonl)
  rhost.py boot [--frames N]              no input: boot → title → attract (smoke test)
  rhost.py det SCEN [-n N] [--calibrate]  N runs, dump at end, diff (determinism gate)
  rhost.py corpus [--only RE]             every scenario; summary.tsv
  rhost.py verify [--only RE]             every scenario × every shadow hook → evidence table
  rhost.py diff A.rhs B.rhs               classify the differences between two dumps
  rhost.py record NAME [--save DIR|TGZ]   HUMAN PLAY on the Windows desktop (realtime) → trace
  rhost.py coverage [--only RE]           first-hit coverage of every IDA function over the corpus

Common opts: --frames N --seed S --hook SPEC --dump-at F,..|end --shot-at F,..|end
             --shot-every N --hash-every N --edition E -j J --extra "raw rhost args"
Windows: `ssh win` (OpenSSH, PowerShell) + the interactive-session queue runner
`oss-rhost-runner` (C:\\oss-rhost\\queue) — separate from openrecet's C:\\orc-rhost, so both
projects can run batches at the same time. Results: /mnt/ephemeral/opensummoners/rhost-runs/.
"""
import argparse
import datetime
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
VOLATILE = os.path.join(HERE, "volatile.txt")
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
    scp(os.path.join(BUILD, "rhost.exe"), f"{WIN}:C:/oss-rhost/bin/rhost.exe")
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
        any(os.path.getmtime(os.path.join(ROOT, "src", f)) > t for f in ("rng.c", "rng.h", "input.c", "input.h"))


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
        if os.path.exists(VOLATILE):
            shutil.copy(VOLATILE, rdir)
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
        ps_args = ",".join("'" + x.replace("'", "''") + "'" for x in args)
        lines.append(f"while (@($procs | Where-Object {{ !$_.HasExited }}).Count -ge $J) {{ Start-Sleep -Milliseconds 50 }}")
        lines.append(f"$procs += Start-Process -FilePath '{WROOT}\\bin\\rhost.exe' -ArgumentList @({ps_args}) -PassThru "
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


def run_batch(a, runs, tag, fetch=("rhost.log",)):
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
    fetch = ("rhost.log", "shot_*.png", "stdout.txt", "stderr.txt") + (("state_*.rhs",) if a.dump_at else ())
    batch, bdir, res, out = run_batch(a, runs, "run", fetch=fetch)
    print_results(res, [r[0] for r in runs])
    return 0 if all(res.get(r[0], {}).get("status") == "ok" for r in runs) else 1


def cmd_boot(a):
    runs = [("boot", None, None, None)]
    a.frames = a.frames or 1500
    fetch = ("rhost.log", "shot_*.png", "stdout.txt", "stderr.txt") + (("state_*.rhs",) if a.dump_at else ())
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
    with open(os.path.join(bdir, "summary.tsv"), "w") as f:
        f.write("scenario\tstatus\treason\tframe\ttick\tcomplete\trng\tstate_sem\tfps\n")
        for n in names:
            r = res.get(n, {})
            f.write("\t".join([n] + [r.get(k, "-") for k in ("status", "reason", "frame", "tick", "complete", "rng", "state_sem", "fps")]) + "\n")
    return 0 if done == len(names) else 1


VERIFY_HOOKS = ["rand", "input_poll"]


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
    for h in VERIFY_HOOKS:
        c, d, k, sc, first = tot.get(h, [0, 0, 0, set(), ""])
        print(f"{h:22s} {c:9d} {d:8d} {k:8d} {len(sc)}" + (f"  FIRST: {first}" if d else ""))
    return 0 if all(tot.get(h, [0, 0])[1] == 0 for h in VERIFY_HOOKS) else 1


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
    sub.add_parser("build").set_defaults(fn=cmd_build)
    sub.add_parser("stage").set_defaults(fn=cmd_stage)
    sub.add_parser("setup").set_defaults(fn=cmd_setup)
    p = sub.add_parser("run"); p.add_argument("scenarios", nargs="+"); common(p); p.set_defaults(fn=cmd_run)
    p = sub.add_parser("boot"); common(p); p.set_defaults(fn=cmd_boot)
    p = sub.add_parser("det"); p.add_argument("scenario"); p.add_argument("-n", type=int, default=3)
    p.add_argument("--calibrate", action="store_true"); common(p); p.set_defaults(fn=cmd_det)
    p = sub.add_parser("corpus"); p.add_argument("--only"); common(p); p.set_defaults(fn=cmd_corpus)
    p = sub.add_parser("verify"); p.add_argument("--only"); common(p); p.set_defaults(fn=cmd_verify)
    p = sub.add_parser("coverage"); p.add_argument("--only"); p.add_argument("scenarios", nargs="*"); common(p); p.set_defaults(fn=cmd_coverage)
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
