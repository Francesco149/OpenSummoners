#!/usr/bin/env bash
# Build rhost.exe into build/rhost/. Host links at 0x200000 without ASLR so retail's
# 0x400000.. range is ours (a .bss hole reserves it). Port code for hooks is compiled with
# the port's own -std (src/Makefile CFLAGS).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd); S=$ROOT/src; H=$ROOT/tools/rhost; B=$ROOT/build/rhost
CC=${RHOST_CC:-i686-w64-mingw32-gcc}
LDF=(-Wl,--image-base,0x200000 -Wl,--large-address-aware -Wl,--disable-dynamicbase -static-libgcc)
PORT_STD=${PORT_STD:-$(sed -n 's/^CFLAGS *?= *\(-std=[a-z0-9]*\).*/\1/p' "$S/Makefile")}
mkdir -p "$B"
PORT_OBJS=()
for f in rng input; do
    $CC $PORT_STD -O2 -I"$S" -c -o "$B/$f.o" "$S/$f.c"
    PORT_OBJS+=("$B/$f.o")
done
$CC -std=gnu11 -O2 -Wall -Wno-unused-function -Wno-format-truncation -Wno-cast-function-type -I"$S" -I"$H" "${LDF[@]}" -o "$B/rhost.exe" \
    "$H"/rhost.c "$H"/rh_heap.c "$H"/rh_files.c "$H"/rh_state.c "$H"/rh_trace.c "$H"/rh_hooks.c "$H"/rh_handles.c \
    "$H"/rh_shot.c "$H"/rh_dinput.c "$H"/rh_media.c "$H"/rh_gdi.c "$H"/rh_scrub.c "${PORT_OBJS[@]}" -luser32 -lgdi32 -lwinmm -lole32 -luuid -lddraw
echo "built $B/rhost.exe (port std: $PORT_STD)"
