# tools/ennse_voice/Makefile — EN-SE JP-voice patch (standalone proxy version.dll).
#
# Builds the drop-in version.dll that adds the Japanese dialogue + deluxe combat voices to
# the retail English special edition (sotes_en.exe).  The game imports version.dll, so this
# loads before the exe entry; it forwards the 17 real version.dll exports to realver.dll (a
# renamed copy of the system version.dll) via version.def, and seeds the voice bank during
# the engine's own bank loader.  Pure voice patch — no mod loading.  See README.md.
#
#   nix develop --command make -C tools/ennse_voice     # -> build/version.dll

CC    := i686-w64-mingw32-gcc
BUILD := ../../build
OUT   := $(BUILD)/version.dll

$(OUT): ennse_voice.c version.def | $(BUILD)
	$(CC) -shared -O2 -s -o $@ ennse_voice.c version.def

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -f $(OUT)

.PHONY: clean
