#!/usr/bin/env python3
"""ida.py — drive headless IDA 9.x on the Windows box (`ssh win`) for SotES editions.

  python3 tools/ida/ida.py export [EDITION ...] [--kb] [--noasm]   # analyse/re-export, fetch
  python3 tools/ida/ida.py fetch  [EDITION ...]                    # re-fetch outputs only
  python3 tools/ida/ida.py status

EDITIONs (default: all): enold (port target, EN Steam 2012), ense-en (EN Steam SE, sotes_en.exe),
ense-jp (JP build shipped in the SE package, sotes.exe).  Windows layout: C:\\oss-ida\\<ed>\\
(exe + cached .i64 next to it), outputs C:\\oss-ida\\out\\<ed>\\ → fetched to
/mnt/ephemeral/opensummoners/decompiled-ida/<ed>/ (derived from the user's binary: never commit).
Separate from openrecet's C:\\orc-ida (own dirs, own DBs) so both can run at once.
--kb builds tools/ida/gen_knowledge.py's pack and applies it (names/types/protos) before export.
"""
import argparse, os, subprocess, sys, json, concurrent.futures as cf

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
LOCAL = os.environ.get("OSS_IDA_LOCAL", "/mnt/ephemeral/opensummoners/decompiled-ida")
WIN = r"C:\oss-ida"
EDITIONS = {
    # tag: (exe path under C:\oss-ida, expected sha256 prefix of the unpacked exe)
    "enold":   (r"enold\sotes.unpacked.exe", "9e032483b998"),
    "ense-en": (r"ense-en\sotes_en.exe.unpacked.exe", "36a0294df405"),
    "ense-jp": (r"ense-jp\sotes.exe.unpacked.exe", "c46353a5b82e"),
}

def ssh(cmd, check=True, capture=True):
    r = subprocess.run(["ssh", "-o", "BatchMode=yes", "win", cmd], capture_output=capture, text=True)
    if check and r.returncode:
        sys.exit("ssh failed (%d): %s\n%s" % (r.returncode, cmd[:200], (r.stderr or "")[-2000:]))
    return r.stdout if capture else ""

def scp_to(local, remote):
    subprocess.run(["scp", "-q", "-o", "BatchMode=yes", local, "win:" + remote.replace("\\", "/")], check=True)

def stage():
    ssh(r"New-Item -ItemType Directory -Force C:\oss-ida\out | Out-Null")
    for f in ("oss_ida_export.py", "run_ida.ps1"):
        scp_to(os.path.join(ROOT, "tools/ida", f), WIN + "\\" + f)

def export_one(ed, kb_win, noasm):
    exe, _ = EDITIONS[ed]
    i64 = WIN + "\\" + exe + ".i64"
    have_db = "True" in ssh("Test-Path '%s'" % i64)
    inp = i64 if have_db else WIN + "\\" + exe
    opts = []
    if kb_win: opts.append("kb=" + kb_win)
    if noasm: opts.append("noasm")
    cmd = "powershell -NoProfile -ExecutionPolicy Bypass -File %s\\run_ida.ps1 -Exe '%s' -Out '%s\\out\\%s'" % (WIN, inp, WIN, ed)
    if opts:
        cmd += " -Opts '%s'" % ";".join(opts)
    out = ssh(cmd)
    return ed, out.strip()

def fetch(ed):
    dst = os.path.join(LOCAL, ed); os.makedirs(dst, exist_ok=True)
    src = "win:C:/oss-ida/out/%s/" % ed
    files = ["all_ida.c", "all_ida.asm", "functions_ida.tsv", "data_xrefs.tsv", "strings_ida.tsv",
             "vtables_ida.tsv", "fingerprints.jsonl", "export_summary.txt", "export_log.txt", "ida.log"]
    for f in files:
        subprocess.run(["scp", "-q", "-o", "BatchMode=yes", src + f, dst + "/"], check=False)
    s = os.path.join(dst, "export_summary.txt")
    return open(s).read().strip() if os.path.exists(s) else "(no summary)"

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("cmd", choices=["export", "fetch", "status"])
    ap.add_argument("editions", nargs="*")
    ap.add_argument("--kb", action="store_true")
    ap.add_argument("--noasm", action="store_true")
    a = ap.parse_args()
    eds = a.editions or list(EDITIONS)
    if a.cmd == "status":
        print(ssh(r"Get-ChildItem C:\oss-ida -Recurse -Include *.i64,export_summary.txt | Select FullName,Length,LastWriteTime | ft -AutoSize"))
        return
    if a.cmd == "export":
        stage()
        kb_win = None
        if a.kb:
            kbl = os.path.join(LOCAL, "knowledge.json")
            subprocess.run([sys.executable, os.path.join(ROOT, "tools/ida/gen_knowledge.py"), "-o", kbl], check=True)
            kb_win = WIN + r"\knowledge.json"; scp_to(kbl, kb_win)
        with cf.ThreadPoolExecutor(len(eds)) as ex:
            for ed, out in ex.map(lambda e: export_one(e, kb_win, a.noasm), eds):
                print("[%s] %s" % (ed, out))
    for ed in eds:
        print("[%s] %s" % (ed, fetch(ed)))

if __name__ == "__main__":
    main()
