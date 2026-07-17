# tools/sotes_trainer/Makefile — the injected EN-SE trainer DLL.
# Invoke from inside `nix develop`.  Single-TU-ish: trainer.c + the standalone
# sotes_save reader (tools/sotes_save), no other deps.  -static -static-libgcc is
# REQUIRED — a default mingw build imports libgcc_s_sjlj-1.dll, absent in the game's
# DLL search path, so LoadLibrary would fail (README).
# Hard-assign (not ?=): the nix dev shell exports CC=gcc, which lacks winsock2.h.
CC      = i686-w64-mingw32-gcc
CFLAGS ?= -O2 -s -Wall -Wextra -static -static-libgcc -I../sotes_save
OUT    ?= ../../build/sotes_trainer.dll
SRCS   := trainer.c ../sotes_save/sotes_save.c
LIBS   := -lws2_32 -luser32   # user32: launcher-dialog auto-dismiss (EnumWindows/BM_CLICK)

.PHONY: all clean
all: $(OUT)

$(OUT): $(SRCS) ../sotes_save/sotes_save.h
	@mkdir -p $(dir $@)
	$(CC) -shared $(CFLAGS) -o $@ $(SRCS) $(LIBS)

clean:
	rm -f $(OUT)
