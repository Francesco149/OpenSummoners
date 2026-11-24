#!/usr/bin/env python3
"""Daily playable build: the `nightly` release on GitHub is never more than a day behind master.

Owner rule (2026-11-24, after a 3-week gap; modelled on ../openrecet's tools/ci/daily_build.py): the
rolling `nightly` pre-release must carry the latest port build — opensummoners-standalone.exe, the drop-in
replacement for sotes.exe — with every change since the previous build, and must not be older than a day.

DUE when master differs from the sha the newest nightly run built AND that run started more than
DAILY_BUILD_MAX_AGE_H hours ago (default 24).  Then: `tools/ci/build_all.sh` (MANDATORY before a push,
CLAUDE.md) → fast-forward push of master → `gh workflow run nightly.yml` (builds, gates no-embedded-assets,
clobbers the `nightly` release assets).

Called by tools/git-hooks/post-commit in the background after every commit (log .git/daily-build.log) and at
session start (CLAUDE.md).  Idempotent and cheap when nothing is due.

  python3 tools/ci/daily_build.py            # check, act if due
  python3 tools/ci/daily_build.py --dry-run  # say what it would do
  python3 tools/ci/daily_build.py --force    # ignore the age rule (still needs new commits unless --rebuild)

Rules: a build running / queued = not due; a FAILED newest build of the same sha is not retried (look at it:
gh run view ID --log-failed); only a clean fast-forward push of `master`, never force, never another branch;
build_all red = no push (log says so).
"""
import argparse
import datetime as dt
import json
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
WORKFLOW = "nightly.yml"


def sh(*args, check=True, capture=True):
    r = subprocess.run(args, cwd=ROOT, capture_output=capture, text=True)
    if check and r.returncode != 0:
        raise SystemExit(f"daily-build: {' '.join(args)} failed: {(r.stderr or r.stdout or '').strip()[-2000:]}")
    return r


def log(msg):
    stamp = dt.datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %z")
    print(f"[{stamp}] daily-build: {msg}", flush=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--force", action="store_true", help="ignore the age rule")
    ap.add_argument("--rebuild", action="store_true", help="dispatch even if master == the last build's sha")
    a = ap.parse_args()
    max_age = dt.timedelta(hours=float(os.environ.get("DAILY_BUILD_MAX_AGE_H", "24")))

    if sh("git", "rev-parse", "--abbrev-ref", "HEAD").stdout.strip() != "master":
        log("not on master; skipped")
        return 0
    head = sh("git", "rev-parse", "master").stdout.strip()
    runs = json.loads(sh("gh", "run", "list", "--workflow", WORKFLOW, "-L", "10",
                         "--json", "createdAt,headSha,status,conclusion,databaseId").stdout or "[]")
    now = dt.datetime.now(dt.timezone.utc)
    if runs:
        r = runs[0]                                    # newest first
        created = dt.datetime.fromisoformat(r["createdAt"].replace("Z", "+00:00"))
        age = now - created
        if r["status"] != "completed":
            log(f"build {r['databaseId']} ({r['headSha'][:8]}) is {r['status']}; not due")
            return 0
        failed = r["conclusion"] != "success"
        if failed and r["headSha"] == head and not a.rebuild:
            log(f"build {r['databaseId']} of {head[:8]} FAILED ({r['conclusion']}); not retrying the same sha "
                f"(gh run view {r['databaseId']} --log-failed)")
            return 1
        if r["headSha"] == head and not a.rebuild:
            log(f"nightly already built {head[:8]} ({age.total_seconds() / 3600:.1f} h ago); not due")
            return 0
        if not failed and age < max_age and not a.force:
            log(f"last build {r['databaseId']} ({r['headSha'][:8]}) is {age.total_seconds() / 3600:.1f} h old "
                f"(< {max_age.total_seconds() / 3600:.0f} h); not due")
            return 0

    sh("git", "fetch", "-q", "origin", "master")
    if sh("git", "merge-base", "--is-ancestor", "origin/master", "master", check=False).returncode != 0:
        log("origin/master is not an ancestor of master (diverged); not pushing — needs a human")
        return 1
    ahead = int(sh("git", "rev-list", "--count", "origin/master..master").stdout.strip())
    if a.dry_run:
        log(f"would run build_all, push master ({ahead} commits ahead, {head[:8]}) and dispatch {WORKFLOW}")
        return 0

    log(f"due: build_all for {head[:8]}")
    b = sh("nix", "develop", "--command", "bash", "tools/ci/build_all.sh", check=False)
    if b.returncode != 0:
        log(f"build_all RED (rc {b.returncode}); not pushing:\n{(b.stdout + b.stderr)[-3000:]}")
        return 1
    if sh("git", "rev-parse", "master").stdout.strip() != head:
        log("master moved during build_all; the next commit's hook takes it")
        return 0
    if ahead:
        sh("git", "push", "origin", "master:master")
        log(f"pushed master ({ahead} commits, {head[:8]})")
    sh("gh", "workflow", "run", WORKFLOW, "--ref", "master")
    log(f"dispatched {WORKFLOW} for {head[:8]}; release: https://github.com/Francesco149/OpenSummoners/releases/tag/nightly")
    return 0


if __name__ == "__main__":
    sys.exit(main())
