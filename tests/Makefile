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
                ../src/obj_container.c
SRCS_TESTS   := test_main.c test_pixel_drawer.c test_asset_register.c \
                test_bitmap_session.c test_wnd_proc.c test_zdd.c \
                test_cs_dispatch.c test_app_pump.c test_title_scene.c \
                test_call_trace.c test_input.c test_obj_container.c

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
