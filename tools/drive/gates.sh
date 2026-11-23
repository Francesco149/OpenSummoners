#!/usr/bin/env bash
# gates.sh [ONLY_REGEX] — the hybrid + standalone corpus checks on Windows (-j 3) and wine (-j 4) IN PARALLEL (separate
# machines; serially the four passes took ~6.5 h on 2026-11-23, the Windows pair alone ~4.5 h). ONLY_REGEX scopes them
# (`corpus --only`) — enough after filing new scenarios with an unchanged binary; run unscoped after a code change.
# Windows hybrid fb-only REGRESSIONs (frame / tick / RNG / state equal) are parity-ledger R12 until a quiet re-run of
# just those scenarios says otherwise — the summary lists them for that re-run.
# Exit 0 = every pass PASS. Logs: runs/accept/.gate-<target>-<mode>.log
set -u
cd "$(dirname "$0")/../.."
export RHOST_WIN="${RHOST_WIN:-win-wifi}"
ONLY=${1:-}
SC=(); [ -n "$ONLY" ] && SC=(--only "$ONLY")
side() {   # side TARGET-ARGS... : hybrid then standalone on one machine
  local t=$1; shift
  for m in hybrid standalone; do
    nix develop --command python3 tools/rhost/rhost.py corpus --check --$m "${SC[@]}" "$@" </dev/null > "runs/accept/.gate-$t-$m.log" 2>&1
    echo "$t $m rc=$?" >> "runs/accept/.gate-$t.rc"
  done
}
rm -f runs/accept/.gate-win.rc runs/accept/.gate-wine.rc
side win -j 3 & a=$!
side wine --target wine -j 4 & b=$!
wait $a $b
fail=0
for t in wine win; do for m in hybrid standalone; do
  echo "#### $t --$m"
  grep -E "CORPUS|CHECK|DIFF|differ|REGRESSION|not blessing|gap| (fail|stalled|timeout) |FAIL" "runs/accept/.gate-$t-$m.log"
  grep -q "CHECK: PASS" "runs/accept/.gate-$t-$m.log" || fail=1
done; done
exit $fail
