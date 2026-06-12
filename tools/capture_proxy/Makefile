# tools/capture_proxy/Makefile — build the trace-studio-v2 capture proxy DLL.
# Invoke from inside `nix develop`.  Single-TU, like the port build.
#
#   build/ddraw_proxy.dll  — the proxy; the capture driver drops a copy named
#                            ddraw.dll into the game dir next to the unpacked exe.

CC = i686-w64-mingw32-gcc

CFLAGS  ?= -std=c11 -O2 -g \
           -Wall -Wextra -Wno-unused-parameter \
           -DUNICODE=0 -D_UNICODE=0 \
           -DWINVER=0x0501 -D_WIN32_WINNT=0x0501
# -static-libgcc so the DLL has no libgcc runtime dependency in the game dir.
LDFLAGS ?= -shared -static-libgcc -Wl,--enable-stdcall-fixup
LIBS    ?= -lddraw -ldxguid -lgdi32 -luser32 -lkernel32 -lwinmm

SRCS := ddraw_proxy.c
DEF  := ddraw_proxy.def
OUT  ?= ../../build/ddraw_proxy.dll

.PHONY: all clean
all: $(OUT)

.PHONY: $(OUT)
$(OUT): $(SRCS) $(DEF)
	@mkdir -p $(dir $@)
	@rm -f $@
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS) $(DEF) $(LIBS)

clean:
	rm -f $(OUT)
