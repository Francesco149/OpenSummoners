# tools/capture_proxy/Makefile — build the trace-studio-v2 capture proxy.
# Invoke from inside `nix develop`.  Single-TU, like the port build.
#
#   build/ddraw_proxy.dll  — the proxy DLL (engine-VA hooks + osr writer + harness).
#   build/inject.exe       — the launcher: CreateProcess(SUSPENDED) the unpacked exe,
#                            force-load the proxy DLL by full path (app-dir ddraw shadowing
#                            is unreliable — System32 ddraw wins, ckpt 164), then resume.

CC = i686-w64-mingw32-gcc

CFLAGS  ?= -std=c11 -O2 -g \
           -Wall -Wextra -Wno-unused-parameter \
           -I../../src \
           -DUNICODE=0 -D_UNICODE=0 \
           -DWINVER=0x0501 -D_WIN32_WINNT=0x0501
# -static-libgcc so the DLL has no libgcc runtime dependency in the game dir.
LDFLAGS ?= -shared -static-libgcc -Wl,--enable-stdcall-fixup
LIBS    ?= -lddraw -ldxguid -lgdi32 -luser32 -lkernel32 -lwinmm

SRCS := ddraw_proxy.c
DEF  := ddraw_proxy.def
OUT  ?= ../../build/ddraw_proxy.dll
INJ  ?= ../../build/inject.exe

.PHONY: all clean
all: $(OUT) $(INJ)

.PHONY: $(OUT)
$(OUT): $(SRCS) $(DEF)
	@mkdir -p $(dir $@)
	@rm -f $@
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS) $(DEF) $(LIBS)

.PHONY: $(INJ)
$(INJ): inject.c
	@mkdir -p $(dir $@)
	@rm -f $@
	$(CC) $(CFLAGS) -o $@ inject.c

clean:
	rm -f $(OUT) $(INJ)
