# tools/osr_view/Makefile — the Trace Studio v2 native frame viewer.
#
# The DEFAULT target is the Dear ImGui + DX11 viewer (osr_view.exe): a native
# Windows PE (mingw-cross) that reconstructs .osr frames with the real DDraw
# blits + GDI text (the C osr_scrub engine) and presents them in an ImGui UI for
# instant scrub + (coming) draw-call + state drill-in.  IMGUI_DIR is provided by
# the nix flake (pkgs.imgui sources).  A raw-GDI fallback (osr_view_gdi.exe) is
# kept for environments without DX11.
#
#   nix develop --command make -C tools/osr_view          # -> build/osr_view.exe
#   (run on Windows)  build/osr_view.exe C:\oss-osr\retail.osr

CC   := i686-w64-mingw32-gcc
CXX  := i686-w64-mingw32-g++
OBJ  := obj

# DDraw/Win32 era defines for the C engine (matches the port); DX11 needs a newer
# _WIN32_WINNT but DDraw is unaffected, so the whole viewer builds at 0x0601.
CDEFS   := -DUNICODE=0 -D_UNICODE=0 -DWINVER=0x0601 -D_WIN32_WINNT=0x0601
CFLAGS  := -std=c11   -O2 -g -Wall -Wextra -Wno-unused-parameter -I../../src $(CDEFS)
CXXFLAGS:= -std=c++17 -O2 -g -Wall -Wno-unused-parameter \
           -I../../src -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends \
           -DUNICODE -D_UNICODE -DWINVER=0x0601 -D_WIN32_WINNT=0x0601
LDFLAGS := -mwindows -static -static-libgcc -static-libstdc++
LIBS    := -ld3d11 -ldxgi -ld3dcompiler -lddraw -ldxguid -lgdi32 -luser32 \
           -lkernel32 -lwinmm -ldwmapi

# the C reconstruction engine (shared with the port's recon_apply).  osr_emit.c is
# linked because zdd.c hard-references its sink taps (osr_emit_blit/evict/clear/dc_*,
# the M5 port emitter); it self-gates on g_oe_fp (NULL here = no-op), so it adds no
# behaviour to the viewer, only the symbols.
CSRC := osr_scrub.c ../../src/recon_apply.c \
        ../../src/zdd.c ../../src/zdd_win32.c \
        ../../src/bitmap_session.c ../../src/bitmap_session_win32.c \
        ../../src/call_trace.c ../../src/render_id.c ../../src/osr_emit.c
# the C++ UI + Dear ImGui
CXXSRC := osr_view_imgui.cpp \
          $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_draw.cpp \
          $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp \
          $(IMGUI_DIR)/backends/imgui_impl_win32.cpp \
          $(IMGUI_DIR)/backends/imgui_impl_dx11.cpp

COBJ   := $(addprefix $(OBJ)/, $(notdir $(CSRC:.c=.o)))
CXXOBJ := $(addprefix $(OBJ)/, $(notdir $(CXXSRC:.cpp=.o)))

OUT     := ../../build/osr_view.exe
OUT_GDI := ../../build/osr_view_gdi.exe

.PHONY: all gdi clean
all: $(OUT)

# explicit per-source rules (sources live in several dirs; flatten into obj/)
$(OBJ)/osr_scrub.o: osr_scrub.c                       ; @mkdir -p $(OBJ); $(CC) $(CFLAGS) -c $< -o $@
$(OBJ)/%.o: ../../src/%.c                             ; @mkdir -p $(OBJ); $(CC) $(CFLAGS) -c $< -o $@
$(OBJ)/osr_view_imgui.o: osr_view_imgui.cpp           ; @mkdir -p $(OBJ); $(CXX) $(CXXFLAGS) -c $< -o $@
$(OBJ)/%.o: $(IMGUI_DIR)/%.cpp                         ; @mkdir -p $(OBJ); $(CXX) $(CXXFLAGS) -c $< -o $@
$(OBJ)/%.o: $(IMGUI_DIR)/backends/%.cpp               ; @mkdir -p $(OBJ); $(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT): $(COBJ) $(CXXOBJ)
	@mkdir -p ../../build
	$(CXX) $(LDFLAGS) -o $@ $(COBJ) $(CXXOBJ) $(LIBS)

# raw-GDI fallback viewer (no DX11 / ImGui)
gdi: $(OUT_GDI)
$(OUT_GDI): osr_view.c $(CSRC)
	@mkdir -p ../../build
	$(CC) -mwindows -static-libgcc $(CFLAGS) -o $@ osr_view.c \
	  ../../src/recon_apply.c ../../src/zdd.c ../../src/zdd_win32.c \
	  ../../src/bitmap_session.c ../../src/bitmap_session_win32.c \
	  ../../src/call_trace.c ../../src/render_id.c ../../src/osr_emit.c \
	  -lddraw -ldxguid -lgdi32 -luser32 -lkernel32 -lwinmm

# console profiler (build/osr_prof.exe)
OUT_PROF := ../../build/osr_prof.exe
prof: $(OUT_PROF)
$(OUT_PROF): osr_prof.c osr_scrub.c $(CSRC)
	@mkdir -p ../../build
	$(CC) $(CFLAGS) -o $@ osr_prof.c osr_scrub.c \
	  ../../src/recon_apply.c ../../src/zdd.c ../../src/zdd_win32.c \
	  ../../src/bitmap_session.c ../../src/bitmap_session_win32.c \
	  ../../src/call_trace.c ../../src/render_id.c ../../src/osr_emit.c \
	  -lddraw -ldxguid -lgdi32 -luser32 -lkernel32 -lwinmm

clean:
	rm -rf $(OBJ) $(OUT) $(OUT_GDI) $(OUT_PROF)
