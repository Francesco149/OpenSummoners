# OpenSummoners — 32-bit Win32 PE build via i686-w64-mingw32-gcc.
# Invoke from inside `nix develop`.  The mingw32 toolchain is on PATH.
#
# Single-translation-unit build per output: one gcc invocation compiles
# every .c in this directory directly into the .exe.  No intermediate
# .o / .d files; every `make` is a full rebuild.  C compiles fast enough
# that this is cheap, and it makes a whole category of "stale .o" bugs
# disappear — explicitly per the user's standing instruction for this repo.
#
# Two outputs from the same .c set:
#   opensummoners.exe       — GUI subsystem (-mwindows).  Distributable.
#                             No console window on launch; stdio is unwired
#                             to anything visible.
#   opensummoners-debug.exe — Console subsystem.  Same code, different PE
#                             header subsystem field.  WSL/cmd wires
#                             stdin/out/err to the launching shell's TTY,
#                             so fprintf actually shows up.

CC = i686-w64-mingw32-gcc

# WINVER 0x0501 = Windows XP.  sotes.exe targets that era; staying
# compatible avoids accidentally pulling in modern API decls.
CFLAGS  ?= -std=c11 -O2 -g \
           -Wall -Wextra -Wno-unused-parameter \
           -DUNICODE=0 -D_UNICODE=0 \
           -DWINVER=0x0501 -D_WIN32_WINNT=0x0501
LDFLAGS ?= -static-libgcc
LIBS    ?= -lwinmm -lgdi32 -luser32 -lkernel32 \
           -ladvapi32 -lshell32 -lole32 \
           -lddraw -ldxguid

SRCS := $(wildcard *.c)

OUT       ?= ../build/opensummoners.exe
OUT_DEBUG ?= ../build/opensummoners-debug.exe

.PHONY: all clean
all: $(OUT) $(OUT_DEBUG)

# Force every invocation to recompile.  .PHONY on the outputs would work,
# but we keep them as file rules so make still creates ../build/ only as
# needed.  The `rm -f $@` at the top of the recipe busts the Windows
# kernel's binary-image cache (WSL2 reuses the old mapping for the same
# path/inode otherwise — see the OpenMare PROGRESS log for the autopsy).
.PHONY: $(OUT) $(OUT_DEBUG)

$(OUT): $(SRCS)
	@mkdir -p $(dir $@)
	@rm -f $@
	$(CC) -mwindows $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS) $(LIBS)

$(OUT_DEBUG): $(SRCS)
	@mkdir -p $(dir $@)
	@rm -f $@
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS) $(LIBS)

clean:
	rm -f $(OUT) $(OUT_DEBUG) *.o *.d
