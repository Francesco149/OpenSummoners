# tools/res_explorer/Makefile — the SotES resource explorer (ImGui + DX11).
#
#   nix develop --command make -C tools/res_explorer    # -> build/res_explorer.exe
#   (run on Windows)  build\res_explorer.exe
#
# 32-bit PE to match the game DLLs (LoadLibraryEx needs the same bitness).
# IMGUI_DIR comes from the nix flake (pkgs.imgui sources), same as osr_view.
# Reuses the port's own decoders: src/bitmap_session.c (sprites) and
# src/map_data.c (maps). bitmap_session_win32.c is NOT linked — res_core.cpp
# provides the bs_* primitives with the 1041-language fallback.

CC   := i686-w64-mingw32-gcc
CXX  := i686-w64-mingw32-g++
OBJ  := obj

CDEFS   := -DWINVER=0x0601 -D_WIN32_WINNT=0x0601
CFLAGS  := -std=c11   -O2 -g -Wall -Wextra -Wno-unused-parameter -I../../src $(CDEFS)
CXXFLAGS:= -std=c++17 -O2 -g -Wall -Wno-unused-parameter \
           -I. -I../../src -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends \
           -DUNICODE -D_UNICODE $(CDEFS)
# --large-address-aware: 4 GB VA on 64-bit hosts — the game's resource DLLs
# alone map ~560 MB as datafiles.
LDFLAGS := -mwindows -static -static-libgcc -static-libstdc++ -Wl,--large-address-aware
LIBS    := -ld3d11 -ldxgi -ld3dcompiler -lgdi32 -luser32 -lkernel32 -lwinmm \
           -lcomdlg32 -lshell32 -lole32 -ldwmapi

CSRC   := ../../src/bitmap_session.c ../../src/map_data.c
CXXSRC := res_explorer.cpp res_core.cpp \
          $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_draw.cpp \
          $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp \
          $(IMGUI_DIR)/backends/imgui_impl_win32.cpp \
          $(IMGUI_DIR)/backends/imgui_impl_dx11.cpp

COBJ   := $(addprefix $(OBJ)/, $(notdir $(CSRC:.c=.o)))
CXXOBJ := $(addprefix $(OBJ)/, $(notdir $(CXXSRC:.cpp=.o)))

OUT := ../../build/res_explorer.exe

.PHONY: all clean
all: $(OUT)

$(OBJ)/%.o: ../../src/%.c
	@mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJ)/%.o: %.cpp res_core.h
	@mkdir -p $(OBJ)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(OBJ)/%.o: $(IMGUI_DIR)/%.cpp
	@mkdir -p $(OBJ)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(OBJ)/%.o: $(IMGUI_DIR)/backends/%.cpp
	@mkdir -p $(OBJ)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT): $(COBJ) $(CXXOBJ)
	@mkdir -p ../../build
	$(CXX) $(LDFLAGS) -o $@ $(COBJ) $(CXXOBJ) $(LIBS)

clean:
	rm -rf $(OBJ) $(OUT)
