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
MODS="rng input collision map_grid entity_move map_data map_decode camera_follow char_ai engine_calls char_physics char_integrate char_special npc_ai char_apply world_tick entity_fx actor_update particle_tick char_behave behave_ops sound_fx entity_life char_state actor_ops particle_spawn grid_refresh particle_init gimmick event room_loop input_dik room_load world_alloc sprite_slot device_init char_def char_init party_member scene_loop arche_pose script_engine scene_cmd char_status room_tick ui_core ui_text ui_menu ui_run scene_box scene_cast scene_run screen_fx hud_view char_attack sana_skills char_hit char_monster arche_sword skill_cast shots behave_helpers char_handlers script_helpers chapter_scripts render_platform render_draw title_menu field_menu save_game ai_skill script_io glyph_text frame_load file_stream input_dev sound_dev zdd_dev win_app asset_load win_main menu_helpers menu_screens char_types char_handlers2 actor_behave sjis_text gamepad ending title_extra leftovers static_init engine_rest story_scripts"
HOST="rhost rh_heap rh_files rh_state rh_trace rh_hooks rh_handles rh_shot rh_dinput rh_media rh_gdi rh_scrub"
# every translation unit in parallel; each object cached under a hash of its PREPROCESSED source + its flags (headers
# included, so an edit anywhere it reads recompiles it; an untouched one is reused — rh_hooks.c alone takes ~30 s, the
# generated script modules ~40 s), then the links (the SizeOfImage pad below relinks) only link
PFLAGS="$PORT_STD -O2 -fno-omit-frame-pointer -I$S -include $H/rh_hostalloc.h"
HFLAGS="-std=gnu11 -O2 -Wall -Wno-unused-function -Wno-format-truncation -Wno-cast-function-type -I$S -I$H -include $H/rh_hostalloc.h"
mkdir -p "$B/cache"
export CC PFLAGS HFLAGS S H B
{ for f in $MODS; do echo "$S/$f.c P"; done; for f in $HOST; do echo "$H/$f.c H"; done; } | xargs -P "$(nproc)" -L 1 bash -c '
    src=$0; if [ "$1" = P ]; then fl=$PFLAGS; else fl=$HFLAGS; fi; f=$(basename "$src" .c)
    key=$( { echo "$CC $fl"; $CC $fl -E "$src"; } | sha1sum | cut -c1-20)
    obj="$B/cache/$f-$key.o"
    if [ ! -f "$obj" ]; then $CC $fl -c -o "$obj.tmp.$$" "$src" && mv "$obj.tmp.$$" "$obj" || exit 255; fi
    cp "$obj" "$B/$f.o"'
for f in $MODS; do PORT_OBJS+=("$B/$f.o"); done
HOST_OBJS=(); for f in $HOST; do HOST_OBJS+=("$B/$f.o"); done
# the host heap on a fixed region (rh_hostalloc.h, force-included everywhere but its own implementation)
$CC -std=gnu11 -O2 -Wall -I"$H" -c -o "$B/rh_hostalloc.o" "$H/rh_hostalloc.c"
LINK=($CC "${LDF[@]}" -o "$B/rhost.exe" "${HOST_OBJS[@]}" "$B/rh_hostalloc.o" "${PORT_OBJS[@]}" -luser32 -lgdi32 -lwinmm -lole32 -luuid -lddraw)
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
