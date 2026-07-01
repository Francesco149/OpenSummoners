# tools/ennse_voice/Makefile — EN-SE JP-voice patch (proxy version.dll).
#
# Builds the drop-in version.dll that adds Japanese dialogue voice to the retail
# English special edition (sotes_en.exe).  See README.md for install/how-it-works.
#
#   nix develop --command make -C tools/ennse_voice     # -> build/version.dll

CC    := i686-w64-mingw32-gcc
BUILD := ../../build
OUT   := $(BUILD)/version.dll

$(OUT): version_proxy.c version.def | $(BUILD)
	$(CC) -shared -O2 -s -o $@ version_proxy.c version.def

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -f $(OUT)

.PHONY: clean
