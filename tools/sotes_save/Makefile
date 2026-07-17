# tools/sotes_save/Makefile — the standalone .sdt inspector (host tool).
# Invoke from inside `nix develop`.  Native (host) build: it reads the user's own
# savedataNN.sdt files at inspection time.  The library (sotes_save.c) is dep-free and
# also compiled into the trainer DLL (tools/sotes_trainer) + reusable by a save editor.
CC     ?= cc
CFLAGS ?= -O2 -Wall -Wextra
OUT    ?= ../../build/sotes_save_dump
SRCS   := sotes_save_dump.c sotes_save.c

.PHONY: all clean
all: $(OUT)

$(OUT): $(SRCS) sotes_save.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

clean:
	rm -f $(OUT)
