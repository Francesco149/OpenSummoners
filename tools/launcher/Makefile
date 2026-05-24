# tools/launcher/Makefile — build opensummoners-launcher.exe.
#
# A Job-Object wrapper for any .exe we spawn (retail under Frida, our
# own drop-in, smoke-test runs).  See opensummoners-launcher.c for the why.
#
# Built with the same mingw-w64 32-bit toolchain that builds src/, but
# as a separate output because the launcher is a dev tool not part of
# the distributable.  Invoke from inside `nix develop`.  Same single-TU
# pattern as src/ — no .o / .d files.

CC = i686-w64-mingw32-gcc

CFLAGS  ?= -std=c11 -O2 -g \
           -Wall -Wextra -Wno-unused-parameter \
           -DWINVER=0x0500 -D_WIN32_WINNT=0x0500
LDFLAGS ?= -static-libgcc
LIBS    ?= -lkernel32 -luser32

SRCS := opensummoners-launcher.c

OUT ?= ../../build/opensummoners-launcher.exe

.PHONY: all clean
all: $(OUT)

.PHONY: $(OUT)
$(OUT): $(SRCS)
	@mkdir -p $(dir $@)
	@rm -f $@
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS) $(LIBS)

clean:
	rm -f $(OUT)
