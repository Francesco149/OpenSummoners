#!/usr/bin/env bash
# accept-batch.sh LIST [--gates] — file a batch of finished driver runs, bless only what needs it, optionally gate.
# LIST: one "RUN NAME TASK" per line (TASK = tools/drive/tasks/TASK.md). Per line: rd accept --no-gates (check, verify,
# verify --real, file), then det -n 3 --calibrate (volatile list).
# Bless (2026-11-23: a d10c batch whose only run FAILED verify --real still re-blessed + gated the whole corpus, ~11 h):
#   no run accepted                     → stop (exit 2), nothing blessed;
#   volatile.<ed>.txt unchanged         → bless ONLY the accepted scenarios (state_sem of the rest cannot move);
#   volatile changed by --calibrate     → bless the whole corpus (the new masked slots change every state_sem).
#   Windows (-j 3) and wine (-j 4) bless IN PARALLEL (different machines).
# R12 guard (parity-ledger R12: the Windows real device's pixels flake under load, PURE retail included — the
# 2026-11-23 bless baked 7 flaked fb values in): pure retail's fb cannot change on a re-bless, so any existing scenario
# whose Windows fb moved vs git HEAD is re-blessed alone (up to 2 more times); one that still moves → exit 3, look at it.
# A real retail-pixel change (an rhost render change) is accepted with ACCEPT_FB_CHANGE=1.
# --gates: then tools/drive/gates.sh over the same scope (hybrid + standalone, Windows ∥ wine).
# No `timeout` on rd: accept runs verify --real over ssh to the Windows box (never kill those mid-command).
# Bash on purpose (zsh does not word-split $var); the list is read on fd 3 so the commands cannot eat it.
# Waiting on this from a Claude session: wait for its EXIT (a background task), never for a marker line — an early
# failure must wake the session while its context is still cached.
set -u
cd "$(dirname "$0")/../.."
export RHOST_WIN="${RHOST_WIN:-galaxybook}"
LIST=$1; GATES=${2:-}
RH="nix develop --command python3 tools/rhost/rhost.py"
VOL=tools/rhost/volatile.enold.txt
vol0=$(sha1sum "$VOL" tools/rhost/volatile.*.txt 2>/dev/null | sha1sum)
accepted=()
while read -r run name task <&3; do
  [ -z "${run:-}" ] && continue
  echo "#### $name"
  ./rd accept "$run" "$name" --task "tools/drive/tasks/$task.md" --no-gates </dev/null > "runs/accept/.$name.accept.log" 2>&1
  rc=$?
  grep -E "PASS|FAIL|MATCH|MISMATCH|report|not accepted" "runs/accept/.$name.accept.log"
  if [ $rc -ne 0 ] || [ ! -d "tests/scenarios/$name" ]; then echo "NOT ACCEPTED: $name (rc $rc; log runs/accept/.$name.accept.log)"; continue; fi
  $RH det "$name" -n 3 --calibrate </dev/null 2>&1 | grep -E "PASS|FAIL|calibrated"
  accepted+=("$name")
done 3< "$LIST"
if [ ${#accepted[@]} -eq 0 ]; then echo "NOTHING ACCEPTED: no bless, no gates"; exit 2; fi
vol1=$(sha1sum "$VOL" tools/rhost/volatile.*.txt 2>/dev/null | sha1sum)
if [ "$vol0" = "$vol1" ]; then
  ONLY="^($(IFS='|'; echo "${accepted[*]}"))\$"; SCOPE=(--only "$ONLY"); echo "#### volatile unchanged: bless $ONLY"
else
  ONLY=""; SCOPE=(); echo "#### volatile changed: bless the whole corpus"
fi
echo "#### bless win ∥ wine"
$RH corpus --bless "${SCOPE[@]}" -j 3 </dev/null > runs/accept/.bless-win.log 2>&1 &
pw=$!
$RH corpus --bless "${SCOPE[@]}" --target wine -j 4 </dev/null > runs/accept/.bless-wine.log 2>&1 &
pn=$!
wait $pw; rw=$?; wait $pn; rn=$?
grep -hE "CORPUS|blessed| (fail|stalled|timeout) " runs/accept/.bless-win.log runs/accept/.bless-wine.log
[ $rw -ne 0 ] || [ $rn -ne 0 ] && { echo "BLESS FAILED (win $rw, wine $rn)"; exit 1; }
# R12 guard: existing scenarios whose Windows fb moved vs HEAD
moved() { python3 - <<'EOF'
import json, subprocess, glob, os
for f in sorted(glob.glob("tests/scenarios/*/expected.json")):
    try: old = json.loads(subprocess.run(["git", "show", f"HEAD:{f}"], capture_output=True, text=True, check=True).stdout)
    except Exception: continue      # a new scenario: nothing to compare
    if old.get("fb") and old.get("fb") != json.load(open(f)).get("fb"):
        print(os.path.basename(os.path.dirname(f)))
EOF
}
if [ -z "${ACCEPT_FB_CHANGE:-}" ]; then
  for try in 1 2; do
    m=$(moved | paste -sd'|'); [ -z "$m" ] && break
    echo "#### R12 guard (try $try): Windows fb moved vs HEAD for: $m — re-blessing them alone"
    $RH corpus --bless --only "^($m)\$" -j 3 </dev/null 2>&1 | grep -E "CORPUS|blessed| (fail|stalled|timeout) "
  done
  m=$(moved | paste -sd'|')
  [ -n "$m" ] && { echo "FB STILL MOVED vs HEAD: $m (R12 flake twice, or a real rhost render change: ACCEPT_FB_CHANGE=1)"; exit 3; }
fi
echo "#### blessed: ${accepted[*]} (scope: ${ONLY:-all})"
if [ "$GATES" = --gates ]; then
  bash tools/drive/gates.sh ${ONLY:+"$ONLY"}; exit $?
fi
echo ALLDONE
