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
LDFLAGS := $(SAN)

SRCS_PORTED  := ../src/pixel_drawer.c ../src/asset_register.c \
                ../src/bitmap_session.c ../src/wnd_proc.c \
                ../src/zdd.c ../src/cs_dispatch.c \
                ../src/app_pump.c ../src/title_scene.c \
                ../src/call_trace.c ../src/input.c \
                ../src/obj_container.c ../src/menu_list.c \
                ../src/title_render.c ../src/title_sink.c \
                ../src/title_drive.c ../src/input_trace.c \
                ../src/rng.c ../src/title_particles.c \
                ../src/app_flow.c ../src/glyph_text.c \
                ../src/glyph_render.c ../src/newgame_menu.c \
                ../src/newgame_scene.c ../src/newgame_drive.c \
                ../src/newgame_box.c ../src/newgame_cursor.c \
                ../src/glyph_wrap.c ../src/newgame_picker.c \
                ../src/prologue_stone.c ../src/prologue_drive.c \
                ../src/game_drive.c ../src/game_world.c \
                ../src/game_map.c ../src/world_tables_data.c
SRCS_TESTS   := test_main.c test_pixel_drawer.c test_asset_register.c \
                test_bitmap_session.c test_wnd_proc.c test_zdd.c \
                test_cs_dispatch.c test_app_pump.c test_title_scene.c \
                test_call_trace.c test_input.c test_obj_container.c \
                test_menu_list.c test_title_render.c test_title_sink.c \
                test_title_drive.c test_input_trace.c test_title_particles.c \
                test_app_flow.c test_glyph_text.c test_glyph_render.c \
                test_newgame_menu.c test_newgame_scene.c \
                test_newgame_drive.c test_newgame_box.c \
                test_newgame_cursor.c test_glyph_wrap.c \
                test_newgame_picker.c test_prologue_stone.c \
                test_prologue_drive.c test_game_drive.c \
                test_game_world.c test_game_map.c

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
