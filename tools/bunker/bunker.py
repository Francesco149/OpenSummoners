#!/usr/bin/env python3
"""bunker: rhost HEADLESS under wine on a temporary NixOS box, inside ONE bounded pool (tools/bunker/README.md).
Port of openrecet's tools/bunker, namespaced oss-* (openrecet's orc-bunker pool and other projects' pools share the box).
The third rhost host: `rd` live sessions (RD_HOST=bunker, or `auto`: win → bunker → wine) and `rhost.py run ..
--target bunker --headless` (rhost.run_batch → run_batch here; also `rd verify` of a bunker session).

    bunker.py setup                        # work dir, nix GC roots (wine, bwrap, python, fontconfig), the prefix
    bunker.py stage                        # rhost build (if stale) + bin/<sha12> + game dirs + this dir's scripts (cached)
    bunker.py run SCEN.. [-j N] [--extra " --x"] [--frames N] [--keep]
    bunker.py status                       # the pool: rd sessions, memory; disk
    bunker.py teardown                     # stop the pool (every OSS bunker process), delete /var/lib/oss-bunker

ssh: ONE multiplexed connection (ControlMaster, ControlPersist 2h, socket CTL) carries every command, rsync and rd
session forward — bunker's sshd refuses connections past `MaxStartups 10:30:100`.
Host: RHOST_BUNKER (default root@10.0.10.58). Local results: RHOST_BUNKER_RUNS (/mnt/ephemeral/opensummoners/bunker-runs).
"""
import argparse
import fcntl
import hashlib
import io
import json
import os
import re
import subprocess
import sys
import tarfile
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "rhost"))
import rhost  # noqa: E402

HOST = os.environ.get("RHOST_BUNKER", "root@10.0.10.58")
W = "/var/lib/oss-bunker"                       # W/root = C:\oss-rhost (= /oss in a container)
PY = f"{W}/gcroots/python/bin/python3"
POOL = f"{PY} {W}/bin/pool.py"
LOCAL = os.environ.get("RHOST_BUNKER_RUNS", "/mnt/ephemeral/opensummoners/bunker-runs")
WROOT = rhost.WROOT
EXE = "rhost.exe"                               # = drive.staged_exe("bunker"); no by-name kills on the box (pid + argv only)
# nixpkgs attr → gcroots link. wine: wineWow64Packages.unstable of the box's system nixpkgs (rev 1c89083e…: 11.19,
# cache.nixos.org; the same store path openrecet's bunker pinned) — see README
NIX = {"wine-wow64-unstable": "wineWow64Packages.unstable", "bwrap": "bubblewrap", "python": "python3",
       "fontconfig": "fontconfig.out"}
WINE = "wine-wow64-unstable"
WINE_PIN = "/nix/store/zjxh9vbxp2h12a018i0krpswfidbjh1k-wine-wow64-11.19"
SCRIPTS = ("pool.py", "batch.py")
STAMP = ".oss-staged"

# ---- the one ssh connection
CTL = os.path.join(os.environ.get("XDG_RUNTIME_DIR") or f"/tmp/oss-{os.getuid()}", f"oss-bunker-{re.sub(r'[^A-Za-z0-9.]', '_', HOST)}.ctl")
OPTS = ["-o", "BatchMode=yes", "-o", f"ControlPath={CTL}", "-o", "ControlMaster=no", "-o", "ServerAliveInterval=30",
        "-o", "ServerAliveCountMax=4"]
RSH = "ssh " + " ".join(OPTS)


def master_up():
    return subprocess.run(["ssh"] + OPTS + ["-O", "check", HOST], capture_output=True).returncode == 0


def ensure_master(timeout=8):
    """the multiplexing master (started once, under a lock: concurrent rd / rhost processes share it) → True if up"""
    if master_up():
        return True
    os.makedirs(os.path.dirname(CTL), exist_ok=True)
    with open(CTL + ".lock", "w") as lk:
        fcntl.flock(lk, fcntl.LOCK_EX)
        if master_up():
            return True
        try:
            os.remove(CTL)                              # a stale socket of a dead master
        except OSError:
            pass
        r = subprocess.run(["ssh", "-o", "BatchMode=yes", "-o", f"ConnectTimeout={timeout}", "-o", f"ControlPath={CTL}",
                            "-o", "ControlMaster=yes", "-o", "ControlPersist=2h", "-o", "ServerAliveInterval=30",
                            "-o", "ServerAliveCountMax=4", "-fN", HOST],
                           stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True,
                           timeout=timeout + 10)
        return r.returncode == 0 and master_up()


def up(timeout=3):
    """bunker answers (the master is up or can be made within TIMEOUT s)"""
    try:
        return ensure_master(timeout)
    except (subprocess.TimeoutExpired, OSError):
        return False


def ssh(cmd, check=True, capture=False, input=None, timeout=None):
    """CMD on the bunker (root), over the master → its exit code (stdout with capture)"""
    if not ensure_master():
        raise SystemExit(f"bunker ({HOST}) unreachable: no ssh master")
    r = subprocess.run(["ssh"] + OPTS + [HOST, cmd], text=input is None or isinstance(input, str),
                       stdout=subprocess.PIPE if capture else None, input=input, timeout=timeout)
    if check and r.returncode:
        raise SystemExit(f"ssh {HOST}: exit {r.returncode}: {cmd[:200]}")
    return r.stdout if capture else r.returncode


def forward(lport, rport, cancel=False):
    """add (cancel) a local forward 127.0.0.1:LPORT → bunker 127.0.0.1:RPORT on the master → ok"""
    return subprocess.run(["ssh"] + OPTS + ["-O", "cancel" if cancel else "forward", "-L", f"127.0.0.1:{lport}:127.0.0.1:{rport}", HOST],
                          capture_output=True).returncode == 0


def rsync(*args):
    # --chown: the container maps its uid 1000 to root; files owned by the sender's uid would be unwritable inside.
    # --chmod u+rwX: the game files are -rw----r-x here
    if not ensure_master():
        raise SystemExit(f"bunker ({HOST}) unreachable: no ssh master")
    subprocess.run(["rsync", "-a", "--chown=root:root", "--chmod=u+rwX", "-e", RSH] + list(args), check=True)


def cmd_setup(a=None):
    links = " && ".join(f"nix build --out-link gcroots/{k} nixpkgs#{v}" for k, v in NIX.items())
    out = ssh(f"set -e; mkdir -p {W}/gcroots {W}/bin {W}/root/game {W}/root/bin {W}/root/runs/drive {W}/prefix-headless; cd {W}; "
              f"{links}; ln -sfn {WINE} gcroots/wine; readlink -f gcroots/*", capture=True)
    print(out, end="")
    if WINE_PIN not in out:
        print(f"[WARNING: wine is not the pinned {WINE_PIN}: the box's system nixpkgs moved — results may differ]")
    stage_scripts()
    if ssh(f"test -e {W}/prefix-headless/system.reg", check=False):
        print("[creating the headless wine prefix (once)]")
        ssh(f"{POOL} run setup --rw-prefix python3 -c 'import subprocess as s; s.run([\"wineboot\", \"-i\"], capture_output=True); "
            f"s.run([\"wineserver\", \"-w\"])'")
    ssh(f"ln -sfn /oss {W}/prefix-headless/drive_c/oss-rhost")
    print(f"[bunker {HOST}: {W} set up; `bunker.py stage` (or the first batch / rd session) pushes rhost + the game]")


def stage_scripts():
    rsync(*[os.path.join(HERE, f) for f in SCRIPTS], f"{HOST}:{W}/bin/")


def exe_sha():
    return rhost.file_sha12(os.path.join(rhost.BUILD, "rhost.exe"))


def game_dirs():
    """{edition dir name: local path} — rhost.WINE_GAMES (the user's own copies; symlinks followed when synced)"""
    return {ed: p for ed, p in rhost.WINE_GAMES.items() if os.path.isdir(p)}


def game_sig():
    """names + sizes + mtimes of the local game dirs (symlinks followed) → the stamp a staged copy carries"""
    h = hashlib.sha1()
    for ed, g in sorted(game_dirs().items()):
        for d, ds, fs in os.walk(g, followlinks=True):
            ds.sort()
            for f in sorted(fs):
                st = os.stat(os.path.join(d, f))
                h.update(f"{ed}/{os.path.relpath(os.path.join(d, f), g)}\0{st.st_size}\0{int(st.st_mtime)}\n".encode())
    return h.hexdigest()[:16]


def scripts_sig():
    return hashlib.sha1(b"".join(open(os.path.join(HERE, f), "rb").read() for f in SCRIPTS)).hexdigest()[:16]


_staged = {}
_stage_lock = threading.Lock()


def stage(build=True):
    """rhost build (if stale) + bin/<sha12>/rhost.exe + game + scripts on the bunker, each only when its stamp differs
    (one round trip when all are current) → sha12; the exe is C:\\oss-rhost\\bin\\<sha12>\\rhost.exe"""
    with _stage_lock:
        if build and rhost.rhost_stale():
            rhost.cmd_build(None)
        sha = exe_sha()
        if _staged.get("sha") == sha:
            return sha
        gs, ss = game_sig(), scripts_sig()
        have = ssh(f"cat {W}/root/game/{STAMP} 2>/dev/null; echo; cat {W}/bin/{STAMP} 2>/dev/null; echo; "
                   f"test -f {W}/root/bin/{sha}/{EXE} && echo BIN || true", capture=True).split("\n")
        if "BIN" not in have:
            ssh(f"mkdir -p {W}/root/bin/{sha}")
            rsync(os.path.join(rhost.BUILD, "rhost.exe"), f"{HOST}:{W}/root/bin/{sha}/{EXE}.tmp")
            ssh(f"mv {W}/root/bin/{sha}/{EXE}.tmp {W}/root/bin/{sha}/{EXE}")
        if have[0].strip() != gs:
            for ed, g in sorted(game_dirs().items()):        # symlinks into the Steam dir, dereferenced
                rsync("-L", "--delete", g + "/", f"{HOST}:{W}/root/game/{ed}/")
            ssh(f"echo {gs} > {W}/root/game/{STAMP}")
        if len(have) < 2 or have[1].strip() != ss:
            stage_scripts()
            ssh(f"echo {ss} > {W}/bin/{STAMP}; mkdir -p {W}/root/runs/drive")
        _staged["sha"] = sha
        return sha


def cmd_stage(a=None):
    sha = stage()
    print(sha, f"{WROOT}\\bin\\{sha}\\{EXE}")


def run_batch(a, runs, tag, fetch=None, keep=False):
    """rhost.run_batch's contract for --target bunker: A = rhost's options (headless only), RUNS = (name, trace, save,
    edition[, hook]) → (batch, local bdir, {name: RESULT}, out). make_batch locally (the identical argv.json as
    `rhost.py run --headless --target wine`), push, run every run in one container in the pool (-j at a time), fetch the
    run dirs back."""
    if not rhost.headless_requested(a):
        raise SystemExit("--target bunker runs headless only (no X server / GPU there): add --headless, or use win / wine")
    if getattr(a, "show", False):
        raise SystemExit("--target bunker is headless: drop --show, or use win")
    sha = stage()
    os.makedirs(LOCAL, exist_ok=True)
    saved, rhost.LOCAL_RUNS = rhost.LOCAL_RUNS, LOCAL
    try:
        batch, bdir = rhost.make_batch(a, runs, tag)
    finally:
        rhost.LOCAL_RUNS = saved
    os.remove(os.path.join(bdir, "go.ps1"))       # the Windows runner script: not used here
    rsync(bdir + "/", f"{HOST}:{W}/root/runs/{batch}/")
    t0 = time.time()
    jobs = max(1, getattr(a, "jobs", 1) or 1)
    to = int(getattr(a, "wine_timeout", 0) or 4 * 3600)
    rc = ssh(f"{POOL} run {batch} python3 /bunker/batch.py /oss/runs/{batch} '{WROOT}\\bin\\{sha}\\{EXE}' "
             f"-j {jobs} --timeout {to}", check=False)
    wall = time.time() - t0
    # not each run's tmp\ (retail unpacks the BGM .wma there: ~80 MB per scenario, ~4 GB per corpus batch; 2026-11-18
    # bunker-runs had grown to 163 GB of it) — nothing reads it after the run
    rsync("--exclude=/*/tmp/", f"{HOST}:{W}/root/runs/{batch}/", bdir + "/")
    if not keep:
        ssh(f"rm -rf {W}/root/runs/{batch}")
    res, out_lines = {}, []
    for n, *_ in runs:
        out_lines.append(f"=== {n}")
        log = os.path.join(bdir, n, "rhost.log")
        text = open(log, errors="replace").read() if os.path.exists(log) else ""
        out_lines.append(text)
        for line in text.splitlines():               # = rhost.run_batch_wine's parse
            if line.startswith("RESULT"):
                res[n] = dict(kv.split("=", 1) for kv in line.split()[1:] if "=" in kv)
            elif re.search(r"FATAL|DIVERGE|hook-result|MessageBox", line):
                res.setdefault(n + "#notes", []).append(line.strip())
    print(f"[bunker batch {batch}: {len(runs)} run(s) in {wall:.1f}s, -j{jobs}, container rc {rc} → {bdir}]")
    return batch, bdir, res, "\n".join(out_lines)


def bstat(bdir, n):
    try:
        return json.load(open(os.path.join(bdir, n, "bunker.json")))
    except OSError:
        return {}


def cmd_run(a):
    runs = rhost.resolve_runs(a.scenarios)
    b = rhost.replay_args(target="bunker", headless=True, frames=a.frames, extra=a.extra or None, jobs=a.j)
    _, bd, res, _ = run_batch(b, runs, "bunker-run", keep=a.keep)
    bad = 0
    for n, *_ in runs:
        r, st = res.get(n), bstat(bd, n)
        if not r:
            bad += 1
            print(f"{n:32s} NO RESULT  → {os.path.join(bd, n)}")
            continue
        fr, ms = int(r.get("frame", "0") or 0), float(r.get("wall_ms", "0") or 0)
        print(f"{n:32s} {r.get('status')} f={fr:>7d} t={r.get('tick')} rng={r.get('rng')} sem={r.get('state_sem')} "
              f"{st.get('wall_s', 0):6.1f}s {r.get('fps', '?')} fps  peak rss {st.get('peak_rss_kb', 0) // 1024} MB")
        bad += r.get("status") != "ok"
    return 1 if bad else 0


# ---- rd live sessions (tools/drive/drive.py calls these)
def spawn_session(name, files, exe_win, args, wait, cap, floor_mb):
    """FILES {relative path (subdirs ok, e.g. user/savedata01.dat): bytes} → C:\\oss-rhost\\runs\\drive\\NAME on the
    bunker; admitted + started detached in the pool → ("LIVE", pid, port) | ("NOADMIT", msg) | ("NOSTART", msg).
    One ssh round trip: the dir travels as a tar on stdin."""
    files = dict(files, **{"spawn.json": json.dumps({"exe": exe_win, "args": args}).encode()})
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode="w") as t:
        for n, data in files.items():
            ti = tarfile.TarInfo(n.replace(os.sep, "/"))
            ti.size, ti.mtime, ti.mode = len(data), int(time.time()), 0o644
            t.addfile(ti, io.BytesIO(data))
    out = ssh(f"{POOL} spawn {name} --wait {wait} --max {cap} --floor-mb {floor_mb}", check=False, capture=True,
              input=buf.getvalue(), timeout=wait + 960)
    out = out.decode(errors="replace") if isinstance(out, bytes) else (out or "")
    m = re.search(r"^LIVE (\d+) (\d+)", out, re.M)
    if m:
        return "LIVE", int(m.group(1)), int(m.group(2))
    if "NOADMIT" in out:
        return "NOADMIT", out.split("NOADMIT", 1)[1].strip()
    return "NOSTART", out[-800:]


def session_alive(pid, name):
    try:
        return ssh(f"{POOL} alive {int(pid)} {name}", check=False, capture=True, timeout=30).strip() == "ALIVE"
    except (subprocess.TimeoutExpired, SystemExit):
        return False


def kill_session(pid, name):
    ssh(f"{POOL} kill {int(pid)} {name}", check=False, capture=True, timeout=60)


def _sess_ok(name, f):
    return re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]*", name) and re.fullmatch(r"[A-Za-z0-9._-]+", f)


def session_file(name, f):
    """bytes of file F in the session's run dir, None when absent"""
    if not _sess_ok(name, f):
        return None
    r = subprocess.run(["ssh"] + OPTS + [HOST, f"cat {W}/root/runs/drive/{name}/{f}"], capture_output=True)
    return r.stdout if r.returncode == 0 else None


def session_get(name, f, dst):
    """file OR dir (e.g. user) F of the session's run dir → local DST (rsync) → True when it exists"""
    if not _sess_ok(name, f) or not ensure_master():
        return False
    src = f"{HOST}:{W}/root/runs/drive/{name}/{f}"
    kind = ssh(f"test -d {W}/root/runs/drive/{name}/{f} && echo D || (test -f {W}/root/runs/drive/{name}/{f} && echo F) || true",
               check=False, capture=True).strip()
    if kind not in ("D", "F"):
        return False
    r = subprocess.run(["rsync", "-a", "-e", RSH, src + ("/" if kind == "D" else ""), dst + ("/" if kind == "D" else "")],
                       capture_output=True)
    return r.returncode == 0 and os.path.exists(dst)


def cmd_status(a):
    ssh(f"{POOL} stat; systemctl list-units --no-legend 'oss-bunker-*'; du -sh {W} 2>/dev/null; free -m | head -2")


def cmd_teardown(a):
    ssh(f"for u in $(systemctl list-units --no-legend --plain 'oss-bunker-*' | cut -d' ' -f1); do systemctl stop $u; done; "
        f"rm -rf {W}; find /nix/var/nix/gcroots/auto -lname '{W}/*' -print -delete")
    subprocess.run(["ssh"] + OPTS + ["-O", "exit", HOST], capture_output=True)
    print(f"[torn down: {W} removed; its GC roots go with it — `nix-collect-garbage` on {HOST} frees the store paths]")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sp = p.add_subparsers(dest="cmd", required=True)
    sp.add_parser("setup").set_defaults(fn=cmd_setup)
    sp.add_parser("stage").set_defaults(fn=cmd_stage)
    sp.add_parser("status").set_defaults(fn=cmd_status)
    sp.add_parser("teardown").set_defaults(fn=cmd_teardown)
    r = sp.add_parser("run")
    r.add_argument("scenarios", nargs="+")
    r.add_argument("-j", type=int, default=4)
    r.add_argument("--frames", type=int)
    r.add_argument("--extra", default="")
    r.add_argument("--keep", action="store_true", help="keep the run dirs on the bunker")
    r.set_defaults(fn=cmd_run)
    a = p.parse_args()
    r = a.fn(a)
    sys.exit(r if isinstance(r, int) else 0)


if __name__ == "__main__":
    main()
