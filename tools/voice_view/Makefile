# tools/voice_view/Makefile — SotES resource browser + audio preview.
#
# A tiny native Win32 GUI (mingw-cross, 32-bit to match the game DLLs) that
# opens a resource DLL, lists its `WAVE`/`DATA`/… resources with each clip's
# audio format + duration, and plays them (PlaySound for PCM/WAV, MCI for
# ASF/WMA).  Proves sotesx_s.dll holds the JP voice bank before we patch the
# EN SE to load it.  Also has headless --list/--dump modes for verification.
#
#   nix develop --command make -C tools/voice_view        # -> build/voice_view.exe
#   (run on Windows)  build\voice_view.exe                 # GUI (auto-prompts)
#   (run on Windows)  build\voice_view.exe <path\to.dll>   # GUI, opens that DLL

CC    := i686-w64-mingw32-gcc
BUILD := ../../build
OUT   := $(BUILD)/voice_view.exe

CFLAGS  := -std=c11 -O2 -g -Wall -Wextra -Wno-unused-parameter \
           -DUNICODE -D_UNICODE -DWINVER=0x0601 -D_WIN32_WINNT=0x0601
LDFLAGS := -mwindows -static -static-libgcc
LIBS    := -lcomdlg32 -lwinmm -lgdi32 -luser32 -lkernel32 -lshell32

$(OUT): voice_view.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LIBS)

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -f $(OUT)

.PHONY: clean
