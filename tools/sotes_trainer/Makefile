# tools/sotes_trainer/Makefile — the injected EN-SE trainer DLL + its Dear ImGui UI.
#
# Two languages: the C trainer core (trainer.c + the standalone sotes_save reader + vendored
# MinHook) and the C++ ImGui UI (trainer_ui.cpp + Dear ImGui core/backends from the flake's
# $IMGUI_DIR, the tools/osr_view pattern).  Objects cache in obj/ so only edited TUs rebuild
# (ImGui is big); the final DLL links with g++ so libstdc++ comes in.  Invoke inside `nix develop`.
#
#   nix develop --command make -C tools/sotes_trainer      # -> build/sotes_trainer.dll
#
# -static -static-libgcc -static-libstdc++ is REQUIRED — a default mingw build imports
# libgcc_s_sjlj-1.dll / libstdc++-6.dll, absent in the game's DLL search path, so LoadLibrary
# would fail (README).
CC    := i686-w64-mingw32-gcc
CXX   := i686-w64-mingw32-g++
IMGUI ?= $(IMGUI_DIR)
OBJ   := obj
OUT   ?= ../../build/sotes_trainer.dll

MINHOOK := minhook/src/hook.c minhook/src/buffer.c minhook/src/trampoline.c \
           minhook/src/hde/hde32.c minhook/src/hde/hde64.c
CSRC    := trainer.c ../sotes_save/sotes_save.c $(MINHOOK)
CXXSRC  := trainer_ui.cpp \
           $(IMGUI)/imgui.cpp $(IMGUI)/imgui_draw.cpp $(IMGUI)/imgui_tables.cpp $(IMGUI)/imgui_widgets.cpp \
           $(IMGUI)/backends/imgui_impl_win32.cpp $(IMGUI)/backends/imgui_impl_dx11.cpp

CFLAGS   := -O2 -Wall -Wextra -I../sotes_save -Iminhook/include
CXXFLAGS := -std=c++17 -O2 -Wall -I. -I../sotes_save -I$(IMGUI) -I$(IMGUI)/backends \
            -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0601
LDFLAGS  := -shared -static -static-libgcc -static-libstdc++ -s
# user32: launcher-dialog dismiss + the UI window; ws2_32: the socket; d3d11/dxgi/...: the UI.
LIBS     := -lws2_32 -luser32 -lgdi32 -lkernel32 -lwinmm -ldwmapi -ld3d11 -ldxgi -ld3dcompiler -limm32

COBJ   := $(addprefix $(OBJ)/, $(notdir $(CSRC:.c=.o)))
CXXOBJ := $(addprefix $(OBJ)/, $(notdir $(CXXSRC:.cpp=.o)))

.PHONY: all clean
all: $(OUT)

$(OUT): $(COBJ) $(CXXOBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) -o $@ $(COBJ) $(CXXOBJ) $(LIBS)

# explicit per-source rules (sources live in several dirs; flatten into obj/)
$(OBJ)/trainer.o: trainer.c trainer_core.h ../sotes_save/sotes_save.h ; @mkdir -p $(OBJ); $(CC) $(CFLAGS) -c $< -o $@
$(OBJ)/sotes_save.o: ../sotes_save/sotes_save.c                        ; @mkdir -p $(OBJ); $(CC) $(CFLAGS) -c $< -o $@
$(OBJ)/%.o: minhook/src/%.c                                            ; @mkdir -p $(OBJ); $(CC) $(CFLAGS) -c $< -o $@
$(OBJ)/%.o: minhook/src/hde/%.c                                        ; @mkdir -p $(OBJ); $(CC) $(CFLAGS) -c $< -o $@
$(OBJ)/trainer_ui.o: trainer_ui.cpp trainer_core.h                     ; @mkdir -p $(OBJ); $(CXX) $(CXXFLAGS) -c $< -o $@
$(OBJ)/%.o: $(IMGUI)/%.cpp                                             ; @mkdir -p $(OBJ); $(CXX) $(CXXFLAGS) -c $< -o $@
$(OBJ)/%.o: $(IMGUI)/backends/%.cpp                                    ; @mkdir -p $(OBJ); $(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ) $(OUT)
