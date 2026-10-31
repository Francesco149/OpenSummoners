# tests/Makefile — host-side (Linux) unit tests for OpenSummoners ports.
#
# Built with the system gcc (not the mingw-w64 cross compiler) + ASan +
# UBSan.  The ports under test are pure C with no Win32 surface, so they
# compile and execute under the sanitizer without any wine/WSLInterop
# dance.  The drop-in's Win32 layer (main.c, dev_hooks.c) is not exercised
# here.
#
#   make -C tests run         # build + run all tests
#   make -C tests run F=pd    # filter by name substring
#   make -C tests clean
#
# Run inside `nix develop` so the right gcc is on PATH.
#
# Build style: single TU per invocation, matching the drop-in's
# preference (no .o caching, no stale-build footguns — see
# memory/feedback_single_tu.md).

CC      ?= gcc
SAN     := -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer
CFLAGS  := -std=c11 -g -O1 -Wall -Wextra -Wno-unused-parameter \
           -I../src $(SAN)
LDFLAGS := $(SAN) -lm

SRCS_PORTED := ../src/asset_register.c ../src/bitmap_session.c ../src/input.c ../src/rng.c \
               ../src/glyph_text.c ../src/map_data.c ../src/map_grid.c ../src/map_decode.c \
               ../src/map_render.c ../src/draw_pool.c ../src/camera_follow.c ../src/anim_clip.c \
               ../src/render_id.c ../src/actor_render.c ../src/actor_spawn.c ../src/party.c \
               ../src/base_stat_table.c ../src/butterfly.c ../src/collision.c ../src/entity_move.c \
               ../src/char_ai.c ../src/engine_calls.c ../src/char_physics.c ../src/char_integrate.c \
               ../src/char_special.c ../src/npc_ai.c ../src/char_apply.c ../src/world_tick.c \
               ../src/entity_fx.c ../src/actor_update.c ../src/particle_tick.c ../src/char_behave.c \
               ../src/behave_ops.c ../src/sound_fx.c ../src/entity_life.c ../src/char_state.c \
               ../src/actor_ops.c ../src/particle_spawn.c ../src/grid_refresh.c ../src/particle_init.c \
               ../src/gimmick.c ../src/event.c ../src/room_loop.c ../src/input_dik.c ../src/room_load.c \
               ../src/world_alloc.c ../src/sprite_slot.c ../src/device_init.c ../src/char_def.c \
               ../src/char_init.c ../src/party_member.c ../src/scene_loop.c ../src/arche_pose.c \
               ../src/script_engine.c ../src/scene_cmd.c ../src/char_status.c ../src/room_tick.c \
               ../src/ui_core.c ../src/ui_text.c ../src/ui_menu.c ../src/ui_run.c ../src/scene_box.c \
               ../src/scene_cast.c ../src/scene_run.c ../src/screen_fx.c ../src/hud_view.c \
               ../src/char_attack.c ../src/sana_skills.c ../src/char_hit.c ../src/char_monster.c \
               ../src/arche_sword.c ../src/skill_cast.c ../src/shots.c ../src/behave_helpers.c \
               ../src/char_handlers.c ../src/script_helpers.c ../src/chapter_scripts.c ../src/msvc_crt.c

SRCS_TESTS := test_main.c host_stubs.c test_asset_register.c test_bitmap_session.c test_input.c \
              test_glyph_text.c test_map_data.c test_map_grid.c test_map_decode.c test_map_render.c \
              test_draw_pool.c test_camera_follow.c test_anim_clip.c test_render_id.c test_actor_render.c \
              test_actor_spawn.c test_party.c test_collision.c test_entity_move.c test_char_ai.c \
              test_char_physics.c test_msvc_crt.c

OUT := build/run_tests

.PHONY: all run clean

all: $(OUT)

$(OUT): $(SRCS_PORTED) $(SRCS_TESTS) t.h
	@mkdir -p build
	@rm -f $@
	$(CC) $(CFLAGS) $(SRCS_PORTED) $(SRCS_TESTS) -o $@ $(LDFLAGS)

run: $(OUT)
	./$(OUT) $(F)

clean:
	rm -rf build
