# tools/ennse_voice/Makefile — EN-SE JP-voice patch (a mod_loader mod).
#
# Builds the mod DLL that adds Japanese dialogue voice to the retail English special
# edition (sotes_en.exe).  Loaded by ../mod_loader (the generic proxy version.dll) from
# the game's mods\ folder.  See README.md for install/how-it-works.
#
#   nix develop --command make -C tools/ennse_voice     # -> build/ennse_voice.dll

CC    := i686-w64-mingw32-gcc
BUILD := ../../build
OUT   := $(BUILD)/ennse_voice.dll

$(OUT): ennse_voice.c | $(BUILD)
	$(CC) -shared -O2 -s -o $@ ennse_voice.c

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -f $(OUT)

.PHONY: clean
