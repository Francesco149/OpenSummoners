# tools/mod_loader/Makefile — generic Fortune Summoners mod loader (proxy version.dll).
#
# Builds the drop-in version.dll that forwards the real version.dll (realver.dll) and loads
# every mods\*.dll beside the game exe.  GENERIC — carries no game addresses (the mods do),
# so it never needs re-deriving on a game update.  See README.md.
#
#   nix develop --command make -C tools/mod_loader     # -> build/version.dll

CC    := i686-w64-mingw32-gcc
BUILD := ../../build
OUT   := $(BUILD)/version.dll

$(OUT): mod_loader.c version.def | $(BUILD)
	$(CC) -shared -O2 -s -o $@ mod_loader.c version.def

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -f $(OUT)

.PHONY: clean
