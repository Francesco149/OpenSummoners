#!/usr/bin/env bash
# tools/ci/build_all.sh — the local "does EVERYTHING build?" gate.
#
# Run before every push that touches src/ or tools/ (MANDATORY — see CLAUDE.md).
# res_explorer AND osr_view LINK the port's own src/*.c, so a new src/ dependency
# silently DRIFTS them: e.g. party.c gaining a base_stat_table.c dep broke the
# res_explorer link *after* a push (undefined BASE_STAT_TABLE).  This catches that
# locally instead of in CI-after-the-fact.
#
# Supersets ci.yml (which gates src + res_explorer + ennse_voice + host suite):
# also builds the dev tools (osr_view / capture_proxy / launcher).  Everything
# cross-compiles via mingw in the nix dev shell — NO game assets, no Windows needed.
set -euo pipefail
cd "$(dirname "$0")/../.."

step() { printf '\n\033[1m== %s ==\033[0m\n' "$*"; }
dev()  { nix develop --command "$@"; }

step "port (opensummoners.exe)";          dev make -C src
step "resource explorer (res_explorer)";  dev make -C tools/res_explorer
step "EN-SE voice patch (version.dll)";    dev make -C tools/ennse_voice
step "osr_view (trace studio)";            dev make -C tools/osr_view
step "capture_proxy (inject.exe)";         dev make -C tools/capture_proxy
step "launcher";                           dev make -C tools/launcher
step "save inspector (sotes_save_dump)";   dev make -C tools/sotes_save
step "EN-SE trainer (sotes_trainer.dll)";  dev make -C tools/sotes_trainer
step "asset gate (no embedded game bytes)"
dev python3 tools/ci/no_proprietary_bytes.py \
    build/opensummoners.exe build/res_explorer.exe build/version.dll
step "host unit suite (ASan/UBSan)";       dev make -C tests run

printf '\n\033[32m✓ ALL BUILDS + TESTS GREEN\033[0m\n'
