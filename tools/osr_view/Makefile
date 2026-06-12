# tools/osr_view/Makefile — the Trace Studio v2 native frame scrubber.
#
# A Win32 GUI app (mingw32, single-TU like the port/proxy) that reconstructs .osr
# frames on demand via the shared recon_apply core (DDraw blits + GDI text) and
# scrubs them instantly.  Reuses the port's zdd.c renderer.
#
#   nix develop --command make -C tools/osr_view     # -> build/osr_view.exe
#   (run on Windows)  build/osr_view.exe C:\oss-osr\retail.osr

CC      := i686-w64-mingw32-gcc
CFLAGS  := -std=c11 -O2 -g -Wall -Wextra -Wno-unused-parameter \
           -I../../src -DUNICODE=0 -D_UNICODE=0 -DWINVER=0x0501 -D_WIN32_WINNT=0x0501
LDFLAGS := -mwindows -static-libgcc
LIBS    := -lddraw -ldxguid -lgdi32 -luser32 -lkernel32 -lwinmm

OUT  := ../../build/osr_view.exe
SRC  := osr_view.c \
        ../../src/recon_apply.c \
        ../../src/zdd.c ../../src/zdd_win32.c \
        ../../src/bitmap_session.c ../../src/bitmap_session_win32.c \
        ../../src/call_trace.c ../../src/render_id.c

.PHONY: all clean
all: $(OUT)

$(OUT): $(SRC)
	@mkdir -p ../../build
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(SRC) $(LIBS)

clean:
	rm -f $(OUT)
