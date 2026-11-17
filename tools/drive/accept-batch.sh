#!/usr/bin/env bash
# accept-batch.sh LIST — file a batch of finished driver runs with ONE re-bless.
# LIST: one "RUN NAME TASK" per line (TASK = tools/drive/tasks/TASK.md). Per line: rd accept --no-gates (check, verify,
# verify --real, file), then det -n 3 --calibrate (volatile list). Then one corpus re-bless: Windows (-j 3), wine (-j 8: the bunker's
# bounded pool; maximus headless when it is down). Then run the gates yourself: rhost.py corpus --check --hybrid / --standalone.
# No `timeout` on rd: accept runs verify --real over ssh to the Windows box (never kill those mid-command).
# Bash on purpose (zsh does not word-split $var); the list is read on fd 3 so the commands cannot eat it.
set -u
cd "$(dirname "$0")/../.."
export RHOST_WIN="${RHOST_WIN:-win-wifi}"
while read -r run name task <&3; do
  [ -z "${run:-}" ] && continue
  echo "#### $name"
  ./rd accept "$run" "$name" --task "tools/drive/tasks/$task.md" --no-gates </dev/null 2>&1 \
    | grep -E "PASS|FAIL|MATCH|MISMATCH|report|not accepted"
  [ -d "tests/scenarios/$name" ] && nix develop --command python3 tools/rhost/rhost.py det "$name" -n 3 --calibrate </dev/null 2>&1 \
    | grep -E "PASS|FAIL|calibrated"
done 3< "$1"
echo "#### bless win";  nix develop --command python3 tools/rhost/rhost.py corpus --bless -j 3 </dev/null 2>&1 | grep -E "CORPUS|blessed| (fail|stalled|timeout) "
echo "#### bless wine"; nix develop --command python3 tools/rhost/rhost.py corpus --bless --target wine -j 8 </dev/null 2>&1 | grep -E "CORPUS|blessed| (fail|stalled|timeout) "
echo ALLDONE
