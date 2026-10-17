#!/usr/bin/env bash
# Build rhost.exe into build/rhost/. Host links at 0x200000 without ASLR so retail's
# 0x400000.. range is ours (a .bss hole reserves it; rhost.x puts .bss FIRST, so the code size never pushes it). Port code for hooks is compiled with
# the port's own -std (src/Makefile CFLAGS).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd); S=$ROOT/src; H=$ROOT/tools/rhost; B=$ROOT/build/rhost
CC=${RHOST_CC:-i686-w64-mingw32-gcc}
LDF=(-Wl,--image-base,0x200000 -Wl,--large-address-aware -Wl,--disable-dynamicbase -Wl,-T,"$ROOT/tools/rhost/rhost.x" -static-libgcc)
PORT_STD=${PORT_STD:-$(sed -n 's/^CFLAGS *?= *\(-std=[a-z0-9]*\).*/\1/p' "$S/Makefile")}
mkdir -p "$B"
PORT_OBJS=()
for f in rng input collision map_grid entity_move map_data map_decode camera_follow char_ai engine_calls char_physics char_integrate npc_ai char_apply world_tick entity_fx actor_update particle_tick char_behave behave_ops sound_fx entity_life char_state actor_ops particle_spawn grid_refresh particle_init gimmick event room_loop input_dik room_load world_alloc sprite_slot device_init char_def char_init party_member scene_loop arche_pose script_engine scene_cmd char_status room_tick ui_core ui_text ui_menu ui_run scene_box scene_cast scene_run screen_fx hud_view char_attack sana_skills char_hit char_monster arche_sword skill_cast shots behave_helpers char_handlers script_helpers chapter_scripts render_platform render_draw glyph_text; do
    $CC $PORT_STD -O2 -fno-omit-frame-pointer -I"$S" -include "$H/rh_hostalloc.h" -c -o "$B/$f.o" "$S/$f.c"
    PORT_OBJS+=("$B/$f.o")
done
# the host heap on a fixed region (rh_hostalloc.h, force-included everywhere but its own implementation)
$CC -std=gnu11 -O2 -Wall -I"$H" -c -o "$B/rh_hostalloc.o" "$H/rh_hostalloc.c"
LINK=($CC -std=gnu11 -O2 -Wall -Wno-unused-function -Wno-format-truncation -Wno-cast-function-type -I"$S" -I"$H" -include "$H/rh_hostalloc.h" "${LDF[@]}" -o "$B/rhost.exe"
    "$H"/rhost.c "$H"/rh_heap.c "$H"/rh_files.c "$H"/rh_state.c "$H"/rh_trace.c "$H"/rh_hooks.c "$H"/rh_handles.c
    "$H"/rh_shot.c "$H"/rh_dinput.c "$H"/rh_media.c "$H"/rh_gdi.c "$H"/rh_scrub.c "$B/rh_hostalloc.o" "${PORT_OBJS[@]}" -luser32 -lgdi32 -lwinmm -lole32 -luuid -lddraw)
"${LINK[@]}"
# a FIXED SizeOfImage: the system allocations wine / Windows place right after the image (heaps retail's
# pointers lead into) must not move when rhost grows — pad .bss up to IMG_SIZE, relink
IMG_SIZE=0x5800000
img_size() { python3 -c "import struct,sys; b=open(sys.argv[1],'rb').read(4096); pe=struct.unpack_from('<I',b,0x3c)[0]; print(struct.unpack_from('<I',b,pe+80)[0])" "$1"; }
CUR=$(img_size "$B/rhost.exe")
PAD=$(( IMG_SIZE - CUR ))
if (( PAD < 0 )); then echo "rhost.exe SizeOfImage $(printf 0x%x $CUR) > $IMG_SIZE: raise IMG_SIZE (re-bless)" >&2; exit 1; fi
if (( PAD > 0 )); then
    for try in 1 2 3; do
        echo "unsigned char rh_img_pad[$PAD] __attribute__((used, aligned(4096)));" > "$B/rh_pad.c"
        $CC -c -o "$B/rh_pad.o" "$B/rh_pad.c"
        "${LINK[@]}" "$B/rh_pad.o"
        NOW=$(img_size "$B/rhost.exe")
        (( NOW == IMG_SIZE )) && break
        PAD=$(( PAD + IMG_SIZE - NOW ))
    done
    (( NOW == IMG_SIZE )) || { echo "rhost.exe: cannot pin SizeOfImage ($(printf 0x%x $NOW))" >&2; exit 1; }
fi
HOLE=$(i686-w64-mingw32-nm "$B/rhost.exe" | awk '$3=="_g_retail_hole"{print $1}')
if (( 0x$HOLE > 0x400000 )); then echo "rhost.exe: .text/.data reach past 0x400000 (hole at 0x$HOLE): retail cannot map" >&2; exit 1; fi
echo "built $B/rhost.exe (port std: $PORT_STD, SizeOfImage pinned $IMG_SIZE, retail hole from 0x$HOLE)"
