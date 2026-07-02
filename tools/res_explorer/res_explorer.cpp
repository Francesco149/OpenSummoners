// tools/res_explorer/res_explorer.cpp - the SotES resource explorer UI
// (Dear ImGui + DX11, modeled on tools/osr_view). See README.md for the UX map.
//
//   res_explorer.exe                                GUI (auto-loads the game install)
//   res_explorer.exe <dll-or-dir>                   GUI, opens that target
//   res_explorer.exe --list <dll> <out.txt>         headless enumerate (voice_view compat)
//   res_explorer.exe --dump <dll> <type> <id> <out> headless raw extract
//   res_explorer.exe --shot <out.png> [<target> [TYPE:ID]]   render + screenshot

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#include <d3d11.h>
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <stdio.h>
#include <math.h>

#include <string>
#include <vector>
#include <algorithm>
#include <atomic>
#include <thread>

#include "res_core.h"
#include "stb_image_write.h"   // decls only; implementation lives in res_core.cpp

// map inspector: the port's own map pipeline, host-side (see MAP INSPECTOR below)
extern "C" {
#include "map_grid.h"
#include "map_decode.h"
#include "map_render.h"
#include "asset_register.h"
#include "actor_spawn.h"
#include "actor_render.h"
}

#include <map>
#include <set>

// asset_register_win32.c taps font creation into the OSR recorder; this tool
// never records, so stub the tap instead of linking the osr_emit chain.
extern "C" void osr_emit_font_create(void *, const void *) {}

// ── DX11 boilerplate (stock ImGui win32+dx11 example, as in osr_view) ───────
static ID3D11Device*           g_dev  = nullptr;
static ID3D11DeviceContext*    g_ctx  = nullptr;
static IDXGISwapChain*         g_swap = nullptr;
static ID3D11RenderTargetView* g_rtv  = nullptr;
static ID3D11SamplerState*     g_nearest = nullptr;

static void CreateRenderTarget()
{
    ID3D11Texture2D* back = nullptr;
    g_swap->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) { g_dev->CreateRenderTargetView(back, nullptr, &g_rtv); back->Release(); }
}
static void CleanupRenderTarget() { if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; } }

static bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    const D3D_FEATURE_LEVEL lvls[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL fl;
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            lvls, 2, D3D11_SDK_VERSION, &sd, &g_swap, &g_dev, &fl, &g_ctx) != S_OK) {
        if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
                lvls, 2, D3D11_SDK_VERSION, &sd, &g_swap, &g_dev, &fl, &g_ctx) != S_OK)
            return false;
    }
    CreateRenderTarget();
    D3D11_SAMPLER_DESC smp;
    ZeroMemory(&smp, sizeof smp);
    smp.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    smp.AddressU = smp.AddressV = smp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    g_dev->CreateSamplerState(&smp, &g_nearest);
    return true;
}
static void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_nearest) { g_nearest->Release(); g_nearest = nullptr; }
    if (g_swap) { g_swap->Release(); g_swap = nullptr; }
    if (g_ctx)  { g_ctx->Release();  g_ctx = nullptr; }
    if (g_dev)  { g_dev->Release();  g_dev = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wp, lp)) return true;
    switch (msg) {
    case WM_SIZE:
        if (g_dev && wp != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_swap->ResizeBuffers(0, (UINT)LOWORD(lp), (UINT)HIWORD(lp),
                                  DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hWnd, msg, wp, lp);
}

// nearest-neighbour sampling for pixel art: swap the sampler inside the draw
// list (the DX11 backend re-binds its own state only on ResetRenderState).
static void cb_sampler_nearest(const ImDrawList*, const ImDrawCmd*)
{
    if (g_nearest) g_ctx->PSSetSamplers(0, 1, &g_nearest);
}
static void draw_nearest_begin(ImDrawList* dl) { dl->AddCallback(cb_sampler_nearest, nullptr); }
static void draw_nearest_end(ImDrawList* dl)   { dl->AddCallback(ImDrawCallback_ResetRenderState, nullptr); }

// ── texture panel ───────────────────────────────────────────────────────────
struct Tex {
    ID3D11Texture2D* tex = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    int w = 0, h = 0;
    bool ensure(int W, int H)
    {
        if (tex && w == W && h == H) return true;
        release();
        D3D11_TEXTURE2D_DESC d;
        ZeroMemory(&d, sizeof d);
        d.Width = W; d.Height = H; d.MipLevels = 1; d.ArraySize = 1;
        d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        d.SampleDesc.Count = 1;
        d.Usage = D3D11_USAGE_DYNAMIC;
        d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        d.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (g_dev->CreateTexture2D(&d, nullptr, &tex) != S_OK) return false;
        if (g_dev->CreateShaderResourceView(tex, nullptr, &srv) != S_OK) return false;
        w = W; h = H;
        return true;
    }
    void upload(const uint32_t* rgba)
    {
        D3D11_MAPPED_SUBRESOURCE m;
        if (g_ctx->Map(tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &m) != S_OK) return;
        for (int y = 0; y < h; y++)
            memcpy((uint8_t*)m.pData + (size_t)y * m.RowPitch,
                   rgba + (size_t)y * w, (size_t)w * 4);
        g_ctx->Unmap(tex, 0);
    }
    void release()
    {
        if (srv) { srv->Release(); srv = nullptr; }
        if (tex) { tex->Release(); tex = nullptr; }
        w = h = 0;
    }
};

// ── theme ───────────────────────────────────────────────────────────────────
static const ImVec4 ACCENT      (0.91f, 0.70f, 0.29f, 1.00f);  // FS gold
static const ImVec4 ACCENT_DIM  (0.91f, 0.70f, 0.29f, 0.55f);
static ImU32 kind_color(RxKind k)
{
    switch (k) {
        case RK_SPRITE:    return IM_COL32(199, 146, 234, 255);
        case RK_IMAGE_RAW: return IM_COL32(130, 170, 255, 255);
        case RK_BMP:       return IM_COL32(130, 170, 255, 255);
        case RK_MAP:       return IM_COL32(247, 140, 108, 255);
        case RK_WAV:       return IM_COL32(195, 232, 141, 255);
        case RK_WMA:       return IM_COL32(137, 221, 255, 255);
        case RK_STRINGS:   return IM_COL32(255, 203, 107, 255);
        case RK_VERSION:   return IM_COL32(160, 160, 170, 255);
        default:           return IM_COL32(140, 140, 150, 255);
    }
}

static void apply_theme()
{
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 0.0f;
    s.ChildRounding = 6.0f;
    s.FrameRounding = 4.0f;
    s.GrabRounding = 4.0f;
    s.TabRounding = 4.0f;
    s.PopupRounding = 4.0f;
    s.ScrollbarRounding = 6.0f;
    s.FramePadding = ImVec2(8, 4);
    s.ItemSpacing = ImVec2(8, 5);
    s.CellPadding = ImVec2(6, 3);
    s.WindowPadding = ImVec2(10, 8);
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]         = ImVec4(0.09f, 0.09f, 0.11f, 1.00f);
    c[ImGuiCol_ChildBg]          = ImVec4(0.11f, 0.11f, 0.13f, 1.00f);
    c[ImGuiCol_PopupBg]          = ImVec4(0.11f, 0.11f, 0.13f, 0.98f);
    c[ImGuiCol_FrameBg]          = ImVec4(0.16f, 0.16f, 0.19f, 1.00f);
    c[ImGuiCol_FrameBgHovered]   = ImVec4(0.22f, 0.21f, 0.22f, 1.00f);
    c[ImGuiCol_FrameBgActive]    = ImVec4(0.28f, 0.25f, 0.20f, 1.00f);
    c[ImGuiCol_TitleBgActive]    = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
    c[ImGuiCol_Header]           = ImVec4(0.91f, 0.70f, 0.29f, 0.28f);
    c[ImGuiCol_HeaderHovered]    = ImVec4(0.91f, 0.70f, 0.29f, 0.40f);
    c[ImGuiCol_HeaderActive]     = ImVec4(0.91f, 0.70f, 0.29f, 0.55f);
    c[ImGuiCol_Button]           = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
    c[ImGuiCol_ButtonHovered]    = ImVec4(0.91f, 0.70f, 0.29f, 0.45f);
    c[ImGuiCol_ButtonActive]     = ImVec4(0.91f, 0.70f, 0.29f, 0.70f);
    c[ImGuiCol_CheckMark]        = ACCENT;
    c[ImGuiCol_SliderGrab]       = ACCENT_DIM;
    c[ImGuiCol_SliderGrabActive] = ACCENT;
    c[ImGuiCol_Tab]              = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);
    c[ImGuiCol_TabHovered]       = ImVec4(0.91f, 0.70f, 0.29f, 0.40f);
    c[ImGuiCol_TabActive]        = ImVec4(0.91f, 0.70f, 0.29f, 0.30f);
    c[ImGuiCol_TableHeaderBg]    = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);
    c[ImGuiCol_TableRowBgAlt]    = ImVec4(1.00f, 1.00f, 1.00f, 0.02f);
    c[ImGuiCol_SeparatorHovered] = ACCENT_DIM;
    c[ImGuiCol_SeparatorActive]  = ACCENT;
    c[ImGuiCol_NavHighlight]     = ACCENT;
    c[ImGuiCol_TextSelectedBg]   = ImVec4(0.91f, 0.70f, 0.29f, 0.30f);
}

static ImFont* g_font_ui = nullptr;
static ImFont* g_font_mono = nullptr;

static void load_fonts()
{
    ImGuiIO& io = ImGui::GetIO();
    const char* ui = "C:\\Windows\\Fonts\\segoeui.ttf";
    const char* jp1 = "C:\\Windows\\Fonts\\meiryo.ttc";
    const char* jp2 = "C:\\Windows\\Fonts\\msgothic.ttc";
    const char* mono = "C:\\Windows\\Fonts\\consola.ttf";
    if (GetFileAttributesA(ui) != INVALID_FILE_ATTRIBUTES)
        g_font_ui = io.Fonts->AddFontFromFileTTF(ui, 17.0f);
    if (!g_font_ui) g_font_ui = io.Fonts->AddFontDefault();
    ImFontConfig cfg; cfg.MergeMode = true;
    const char* jp = GetFileAttributesA(jp1) != INVALID_FILE_ATTRIBUTES ? jp1 :
                     GetFileAttributesA(jp2) != INVALID_FILE_ATTRIBUTES ? jp2 : nullptr;
    if (jp) io.Fonts->AddFontFromFileTTF(jp, 18.0f, &cfg, io.Fonts->GetGlyphRangesJapanese());
    if (GetFileAttributesA(mono) != INVALID_FILE_ATTRIBUTES)
        g_font_mono = io.Fonts->AddFontFromFileTTF(mono, 15.0f);
    if (!g_font_mono) g_font_mono = g_font_ui;
}

// ── audio players ───────────────────────────────────────────────────────────
// PCM path: waveOut straight from the locked resource (pause/seek/volume/loop).
struct PcmPlayer {
    HWAVEOUT h = nullptr;
    WAVEHDR hdr;
    const BYTE* pcm = nullptr;      // wave data bytes (inside the locked blob)
    DWORD datasz = 0, seekoff = 0;
    WAVEFORMATEX fmt;
    bool active = false, paused = false, loop = false;
    float volume = 1.0f;

    void close()
    {
        if (h) {
            waveOutReset(h);
            if (hdr.dwFlags & WHDR_PREPARED) waveOutUnprepareHeader(h, &hdr, sizeof hdr);
            waveOutClose(h);
            h = nullptr;
        }
        active = paused = false;
    }
    void select(const BYTE* data, const RxWave* wv)
    {
        close();
        pcm = data + wv->dataoff;
        datasz = wv->datasz;
        memset(&fmt, 0, sizeof fmt);
        fmt.wFormatTag = WAVE_FORMAT_PCM;
        fmt.nChannels = wv->ch;
        fmt.nSamplesPerSec = wv->sr;
        fmt.wBitsPerSample = wv->bits;
        fmt.nBlockAlign = (WORD)(wv->ch * wv->bits / 8);
        fmt.nAvgBytesPerSec = wv->sr * fmt.nBlockAlign;
        seekoff = 0;
    }
    bool play_from(DWORD off)
    {
        close();
        if (!pcm || !datasz || off >= datasz) return false;
        if (waveOutOpen(&h, WAVE_MAPPER, &fmt, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
            return false;
        set_volume(volume);
        memset(&hdr, 0, sizeof hdr);
        hdr.lpData = (LPSTR)(pcm + off);
        hdr.dwBufferLength = datasz - off;
        waveOutPrepareHeader(h, &hdr, sizeof hdr);
        if (waveOutWrite(h, &hdr, sizeof hdr) != MMSYSERR_NOERROR) { close(); return false; }
        seekoff = off;
        active = true; paused = false;
        return true;
    }
    void toggle_pause()
    {
        if (!active) { play_from(0); return; }
        if (paused) { waveOutRestart(h); paused = false; }
        else        { waveOutPause(h);   paused = true; }
    }
    void stop() { close(); }
    void set_volume(float v)
    {
        volume = v;
        if (h) {
            DWORD u = (DWORD)(v * 0xFFFF);
            waveOutSetVolume(h, u | (u << 16));
        }
    }
    // playback byte position within the wave data
    DWORD pos_bytes()
    {
        if (!h) return 0;
        MMTIME t; t.wType = TIME_BYTES;
        waveOutGetPosition(h, &t, sizeof t);
        DWORD p = seekoff + (t.wType == TIME_BYTES ? t.u.cb : 0);
        return p > datasz ? datasz : p;
    }
    void update()   // finished? loop or stop
    {
        if (active && (hdr.dwFlags & WHDR_DONE)) {
            if (loop) play_from(0);
            else close();
        }
    }
};

// MCI path: WMA/ASF + non-PCM RIFF staged to a temp file, played via the
// system codec (play/pause/seek/position in ms).
struct MciPlayer {
    bool open_ = false, paused = false;
    wchar_t path[MAX_PATH] = {0};
    int length_ms = 0;

    void close()
    {
        if (open_) {
            mciSendStringW(L"close rxclip", NULL, 0, NULL);
            open_ = false; paused = false;
        }
        if (path[0]) { DeleteFileW(path); path[0] = 0; }
    }
    bool select(const BYTE* p, DWORD n, bool is_wav)
    {
        close();
        wchar_t dir[MAX_PATH];
        GetTempPathW(MAX_PATH, dir);
        _snwprintf(path, MAX_PATH, L"%lsres_explorer_preview.%ls", dir,
                   is_wav ? L"wav" : L"wma");
        HANDLE h = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                               CREATE_ALWAYS, 0, NULL);
        if (h == INVALID_HANDLE_VALUE) { path[0] = 0; return false; }
        DWORD wr; WriteFile(h, p, n, &wr, NULL); CloseHandle(h);
        wchar_t cmd[MAX_PATH + 64];
        _snwprintf(cmd, MAX_PATH + 64, L"open \"%ls\" alias rxclip", path);
        if (mciSendStringW(cmd, NULL, 0, NULL) != 0) { path[0] = 0; return false; }
        mciSendStringW(L"set rxclip time format milliseconds", NULL, 0, NULL);
        wchar_t buf[64] = {0};
        mciSendStringW(L"status rxclip length", buf, 64, NULL);
        length_ms = _wtoi(buf);
        open_ = true;
        return true;
    }
    void play_from(int ms)
    {
        if (!open_) return;
        wchar_t cmd[96];
        _snwprintf(cmd, 96, L"play rxclip from %d", ms);
        mciSendStringW(cmd, NULL, 0, NULL);
        paused = false;
    }
    void toggle_pause()
    {
        if (!open_) return;
        if (is_playing()) { mciSendStringW(L"pause rxclip", NULL, 0, NULL); paused = true; }
        else if (paused)  { mciSendStringW(L"resume rxclip", NULL, 0, NULL); paused = false; }
        else play_from(0);
    }
    void stop() { if (open_) mciSendStringW(L"stop rxclip", NULL, 0, NULL); paused = false; }
    int pos_ms()
    {
        if (!open_) return 0;
        wchar_t buf[64] = {0};
        mciSendStringW(L"status rxclip position", buf, 64, NULL);
        return _wtoi(buf);
    }
    bool is_playing()
    {
        if (!open_) return false;
        wchar_t buf[64] = {0};
        mciSendStringW(L"status rxclip mode", buf, 64, NULL);
        return !lstrcmpW(buf, L"playing");
    }
};

// ── app state ───────────────────────────────────────────────────────────────
static RxWorld g_world;
static std::vector<int> g_view;          // filtered+sorted entry indices
static int  g_sel = -1;                  // selected entry index (into g_world.entries)
static char g_filter[128] = "";
static int  g_kind_filter = -1;          // -1 = all
static int  g_mod_filter = -1;
static bool g_view_dirty = true;
static char g_status[256] = "Open a game DLL, or click 'Load game install'.";
static bool g_focus_filter = false;

// per-selection preview state
static RxImage   g_img;
static bool      g_img_ok = false;
static Tex       g_img_tex;
static bool      g_key_on = true;
static bool      g_flip_on = false;
static bool      g_grid_on = false;
static int       g_cell_w = 32, g_cell_h = 32;
static float     g_zoom = 0.0f;          // 0 = fit
static ImVec2    g_pan = ImVec2(0, 0);
static int       g_bg_mode = 0;          // 0 checker, 1 dark, 2 black, 3 magenta
static bool      g_img_opts_dirty = false;

static const BYTE* g_blob = nullptr;     // locked bytes of the selection
static DWORD       g_blob_sz = 0;

static PcmPlayer g_pcm;
static MciPlayer g_mci;
static bool      g_use_mci = false;      // selection plays via MCI (wma / non-PCM wav)
static std::vector<float> g_peaks;
static bool      g_peaks_ok = false;

static map_data  g_map;
static bool      g_map_ok = false;
static Tex       g_map_tex;
static bool      g_plane_on[8] = { true, true, true, true, true, true, true, true };
static bool      g_obj_on = true;
static bool      g_map_opts_dirty = false;

static std::vector<char> g_strings;
static bool      g_strings_ok = false;

static void set_status(const char* fmt, ...);
static void mi_reset_world();   // map inspector: drop caches on module reload

// install loading runs on a worker thread (mapping + classifying ~560 MB of
// DLLs takes seconds - the window must paint, not freeze blank).
static std::atomic<int> g_loading(0);      // 0 idle, 1 running, 2 ready to adopt
static RxWorld*         g_load_result = nullptr;
static int              g_load_count = 0;
static std::thread      g_load_thread;

static void start_install_load()
{
    if (g_loading.load() != 0) return;
    g_loading.store(1);
    g_load_thread = std::thread([]() {
        RxWorld* w = new RxWorld;
        g_load_count = rx_load_game_install(w, nullptr, nullptr);
        g_load_result = w;
        g_loading.store(2);
    });
}

static void select_entry(int idx);

static void adopt_load_if_ready()
{
    if (g_loading.load() != 2) return;
    g_load_thread.join();
    select_entry(-1);
    mi_reset_world();
    rx_close_all(&g_world);
    g_world = std::move(*g_load_result);
    delete g_load_result;
    g_load_result = nullptr;
    g_loading.store(0);
    g_view_dirty = true;
    set_status(g_load_count ? "Loaded %d module(s) from the detected game install."
                            : "No game install found - use Open DLL...", g_load_count);
}

static void set_status(const char* fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    vsnprintf(g_status, sizeof g_status, fmt, ap);
    va_end(ap);
}

// ── selection / preview build ───────────────────────────────────────────────
static void build_map_texture()
{
    if (!g_map_ok || !g_map.dim0 || !g_map.dim1) return;
    int w = (int)g_map.dim0, h = (int)g_map.dim1;
    std::vector<uint32_t> px((size_t)w * h, 0xff17181cu);
    for (uint32_t z = 0; z < g_map.dim2 && z < 8; z++) {
        if (!g_plane_on[z]) continue;
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                map_cell c;
                if (map_data_cell(&g_map, x, y, z, &c) != 0 || c.tile_id == 0) continue;
                // golden-ratio hue from tile_id (stable, well-spread)
                float hue = fmodf((float)c.tile_id * 0.61803399f, 1.0f);
                float r, g, b;
                ImGui::ColorConvertHSVtoRGB(hue, 0.55f, 0.60f + 0.25f * (z / 3.0f), r, g, b);
                px[(size_t)y * w + x] = IM_COL32((int)(r * 255), (int)(g * 255),
                                                 (int)(b * 255), 255);
            }
    }
    if (g_map_tex.ensure(w, h)) g_map_tex.upload(px.data());
}

static void rebuild_image_texture()
{
    if (!g_img_ok) return;
    std::vector<uint32_t> disp;
    rx_image_display(&g_img, g_key_on, g_flip_on, &disp);
    if (g_img_tex.ensure(g_img.w, g_img.h)) g_img_tex.upload(disp.data());
}

static void select_entry(int idx)
{
    if (idx == g_sel) return;
    g_sel = idx;
    // teardown
    g_pcm.stop(); g_mci.close();
    g_img_ok = false; g_peaks_ok = false; g_strings_ok = false;
    if (g_map_ok) { map_data_free(&g_map); g_map_ok = false; }
    g_blob = nullptr; g_blob_sz = 0;
    g_zoom = 0.0f; g_pan = ImVec2(0, 0); g_grid_on = false;
    if (idx < 0) return;

    const RxEntry* e = &g_world.entries[idx];
    g_blob = rx_lock(&g_world, e, &g_blob_sz);

    switch (e->kind) {
    case RK_SPRITE: case RK_IMAGE_RAW: case RK_BMP:
        g_img_ok = rx_decode_image(&g_world, e, &g_img) != 0;
        if (g_img_ok) {
            g_flip_on = g_img.default_flip != 0;
            g_key_on = true;
            rebuild_image_texture();
        }
        break;
    case RK_WAV:
        if (g_blob) {
            g_use_mci = (e->wave.tag != 0x0001);
            if (!g_use_mci) {
                g_pcm.select(g_blob, &e->wave);
                g_peaks_ok = rx_wave_peaks(g_blob, &e->wave, 1024, &g_peaks) != 0;
            } else {
                g_mci.select(g_blob, g_blob_sz, true);
            }
        }
        break;
    case RK_WMA:
        g_use_mci = true;
        if (g_blob) g_mci.select(g_blob, g_blob_sz, false);
        break;
    case RK_MAP:
        g_map_ok = rx_parse_map(&g_world, e, &g_map) != 0;
        if (g_map_ok) build_map_texture();
        break;
    case RK_STRINGS:
        g_strings_ok = rx_decode_strings(&g_world, e, &g_strings) != 0;
        break;
    default: break;
    }
}

// ── filtering / sorting ─────────────────────────────────────────────────────
static bool match_filter(const RxEntry& e)
{
    if (g_kind_filter >= 0 && (int)e.kind != g_kind_filter) return false;
    if (g_mod_filter >= 0 && e.module_idx != g_mod_filter) return false;
    if (!g_filter[0]) return true;
    char hay[320];
    _snprintf(hay, sizeof hay, "%s %s %s %s %s", e.id_label, e.type_label,
              rx_kind_name(e.kind), e.info, g_world.modules[e.module_idx].label);
    // case-insensitive substring
    char needle[128];
    lstrcpynA(needle, g_filter, sizeof needle);
    CharLowerA(hay); CharLowerA(needle);
    return strstr(hay, needle) != nullptr;
}

static const ImGuiTableSortSpecs* g_sort = nullptr;
static bool view_less(int ia, int ib)
{
    const RxEntry& a = g_world.entries[ia];
    const RxEntry& b = g_world.entries[ib];
    if (g_sort)
        for (int i = 0; i < g_sort->SpecsCount; i++) {
            const ImGuiTableColumnSortSpecs& sp = g_sort->Specs[i];
            int d = 0;
            switch (sp.ColumnIndex) {
                case 0: d = a.module_idx - b.module_idx; break;
                case 1: d = strcmp(a.type_label, b.type_label); break;
                case 2: d = (a.name.isStr || b.name.isStr)
                              ? strcmp(a.id_label, b.id_label)
                              : (int)a.name.id - (int)b.name.id; break;
                case 3: d = (int)a.kind - (int)b.kind; break;
                case 4: d = a.size < b.size ? -1 : a.size > b.size ? 1 : 0; break;
                case 5: d = strcmp(a.info, b.info); break;
            }
            if (d) return sp.SortDirection == ImGuiSortDirection_Ascending ? d < 0 : d > 0;
        }
    if (a.module_idx != b.module_idx) return a.module_idx < b.module_idx;
    int t = strcmp(a.type_label, b.type_label);
    if (t) return t < 0;
    if (!a.name.isStr && !b.name.isStr) return a.name.id < b.name.id;
    return strcmp(a.id_label, b.id_label) < 0;
}

static void rebuild_view()
{
    g_view.clear();
    for (int i = 0; i < (int)g_world.entries.size(); i++)
        if (match_filter(g_world.entries[i])) g_view.push_back(i);
    std::stable_sort(g_view.begin(), g_view.end(), view_less);
    g_view_dirty = false;
}

// ── open helpers ────────────────────────────────────────────────────────────
static void open_target(const wchar_t* path)
{
    mi_reset_world();   // pool slots keep HMODULE pointers; re-register lazily
    DWORD attr = GetFileAttributesW(path);
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        int n = rx_load_game_install(&g_world, path, nullptr);
        set_status("Loaded %d module(s) from folder.", n);
    } else {
        int m = rx_open_module(&g_world, path);
        if (m >= 0) set_status("Loaded %s (%d resources).",
                               g_world.modules[m].label, g_world.modules[m].n_entries);
        else set_status("LoadLibraryEx failed (not a 32-bit PE?).");
    }
    g_view_dirty = true;
}

static void ui_open_dll(HWND hwnd)
{
    wchar_t path[MAX_PATH] = {0};
    OPENFILENAMEW ofn; memset(&ofn, 0, sizeof ofn);
    ofn.lStructSize = sizeof ofn; ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"Game PEs (*.dll;*.exe)\0*.dll;*.exe\0All files\0*.*\0";
    ofn.lpstrFile = path; ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = L"C:\\Program Files (x86)\\Steam\\steamapps\\common";
    ofn.lpstrTitle = L"Open a SotES resource DLL / exe";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameW(&ofn)) open_target(path);
}

static bool pick_folder(HWND hwnd, wchar_t* out)
{
    BROWSEINFOW bi; memset(&bi, 0, sizeof bi);
    bi.hwndOwner = hwnd;
    bi.lpszTitle = L"Export destination folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return false;
    BOOL ok = SHGetPathFromIDListW(pidl, out);
    CoTaskMemFree(pidl);
    return ok;
}

static bool save_dialog(HWND hwnd, const wchar_t* filter, const wchar_t* defext,
                        const char* suggested, wchar_t* out)
{
    wchar_t sug[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, suggested, -1, sug, MAX_PATH);
    lstrcpynW(out, sug, MAX_PATH);
    OPENFILENAMEW ofn; memset(&ofn, 0, sizeof ofn);
    ofn.lStructSize = sizeof ofn; ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = out; ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = defext;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    return GetSaveFileNameW(&ofn) != 0;
}

static void suggest_name(const RxEntry* e, const char* ext, char* out, int cap)
{
    char modstem[64];
    lstrcpynA(modstem, g_world.modules[e->module_idx].label, sizeof modstem);
    char* dot = strrchr(modstem, '.');
    if (dot) *dot = 0;
    _snprintf(out, cap, "%s_%s_%s.%s", modstem, e->type_label, e->id_label, ext);
}

// ── canvas (shared zoom/pan image view) ─────────────────────────────────────
// returns hovered pixel (or -1,-1); draws tex scaled by g_zoom (0 = fit).
static void canvas_view(Tex* tex, int* hover_x, int* hover_y, float min_fit_zoom_cap)
{
    *hover_x = *hover_y = -1;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x < 32 || avail.y < 32 || !tex->srv) { ImGui::Dummy(avail); return; }

    float fit = std::min(avail.x / tex->w, avail.y / tex->h);
    if (fit > min_fit_zoom_cap) fit = min_fit_zoom_cap;
    float zoom = (g_zoom <= 0.0f) ? fit : g_zoom;

    ImGui::InvisibleButton("##canvas", avail,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
    bool hovered = ImGui::IsItemHovered();
    ImVec2 p0 = ImGui::GetItemRectMin();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(p0, ImVec2(p0.x + avail.x, p0.y + avail.y), true);

    // background
    ImU32 bgc = g_bg_mode == 2 ? IM_COL32(0, 0, 0, 255) :
                g_bg_mode == 3 ? IM_COL32(255, 0, 255, 255) : IM_COL32(23, 24, 28, 255);
    dl->AddRectFilled(p0, ImVec2(p0.x + avail.x, p0.y + avail.y), bgc);

    float iw = tex->w * zoom, ih = tex->h * zoom;
    ImVec2 img0(p0.x + (avail.x - iw) * 0.5f + g_pan.x,
                p0.y + (avail.y - ih) * 0.5f + g_pan.y);
    ImVec2 img1(img0.x + iw, img0.y + ih);

    if (g_bg_mode == 0) {   // checkerboard under the image only
        const float cs = 12.0f;
        int nx = (int)(iw / cs) + 1, ny = (int)(ih / cs) + 1;
        for (int cy = 0; cy < ny; cy++)
            for (int cx = 0; cx < nx; cx++) {
                if ((cx + cy) & 1) continue;
                ImVec2 a(img0.x + cx * cs, img0.y + cy * cs);
                ImVec2 b(std::min(a.x + cs, img1.x), std::min(a.y + cs, img1.y));
                if (a.x < img1.x && a.y < img1.y)
                    dl->AddRectFilled(a, b, IM_COL32(40, 41, 46, 255));
            }
        dl->AddRectFilled(img0, img1, IM_COL32(30, 31, 35, 100));
    }

    bool nearest = zoom >= 2.0f;
    if (nearest) draw_nearest_begin(dl);
    dl->AddImage((ImTextureID)tex->srv, img0, img1);
    if (nearest) draw_nearest_end(dl);
    dl->AddRect(ImVec2(img0.x - 1, img0.y - 1), ImVec2(img1.x + 1, img1.y + 1),
                IM_COL32(90, 90, 100, 255));

    // grid overlay
    if (g_grid_on && g_cell_w > 0 && g_cell_h > 0) {
        for (int x = g_cell_w; x < tex->w; x += g_cell_w)
            dl->AddLine(ImVec2(img0.x + x * zoom, img0.y), ImVec2(img0.x + x * zoom, img1.y),
                        IM_COL32(255, 214, 107, 90));
        for (int y = g_cell_h; y < tex->h; y += g_cell_h)
            dl->AddLine(ImVec2(img0.x, img0.y + y * zoom), ImVec2(img1.x, img0.y + y * zoom),
                        IM_COL32(255, 214, 107, 90));
    }

    ImGuiIO& io = ImGui::GetIO();
    if (hovered) {
        // wheel zoom about the cursor
        if (io.MouseWheel != 0.0f) {
            float oldz = zoom;
            float mult = powf(1.25f, io.MouseWheel);
            zoom = std::max(0.05f, std::min(32.0f, zoom * mult));
            g_zoom = zoom;
            // keep the pixel under the cursor stationary
            ImVec2 m = io.MousePos;
            g_pan.x = m.x - (m.x - g_pan.x - (p0.x + (avail.x - iw) * 0.5f)) * (zoom / oldz)
                      - (p0.x + (avail.x - tex->w * zoom) * 0.5f);
            g_pan.y = m.y - (m.y - g_pan.y - (p0.y + (avail.y - ih) * 0.5f)) * (zoom / oldz)
                      - (p0.y + (avail.y - tex->h * zoom) * 0.5f);
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            g_pan.x += io.MouseDelta.x;
            g_pan.y += io.MouseDelta.y;
        }
        int px = (int)floorf((io.MousePos.x - img0.x) / zoom);
        int py = (int)floorf((io.MousePos.y - img0.y) / zoom);
        if (px >= 0 && py >= 0 && px < tex->w && py < tex->h) { *hover_x = px; *hover_y = py; }
        // hovered grid cell highlight
        if (g_grid_on && *hover_x >= 0 && g_cell_w > 0 && g_cell_h > 0) {
            int cx = *hover_x / g_cell_w, cy = *hover_y / g_cell_h;
            ImVec2 a(img0.x + cx * g_cell_w * zoom, img0.y + cy * g_cell_h * zoom);
            ImVec2 b(a.x + g_cell_w * zoom, a.y + g_cell_h * zoom);
            dl->AddRect(a, b, IM_COL32(255, 214, 107, 255), 0, 0, 2.0f);
        }
    }
    dl->PopClipRect();
}

// ── per-kind preview panels ─────────────────────────────────────────────────
static void panel_image(HWND hwnd, const RxEntry* e)
{
    if (!g_img_ok) { ImGui::TextDisabled("decode failed"); return; }

    // toolbar
    if (ImGui::Button("Fit")) { g_zoom = 0; g_pan = ImVec2(0, 0); }
    ImGui::SameLine();
    if (ImGui::Button("1:1")) { g_zoom = 1; g_pan = ImVec2(0, 0); }
    ImGui::SameLine();
    if (ImGui::Button("4x")) { g_zoom = 4; g_pan = ImVec2(0, 0); }
    ImGui::SameLine(0, 14);
    if (ImGui::Checkbox("Colorkey", &g_key_on)) g_img_opts_dirty = true;
    ImGui::SameLine();
    if (ImGui::Checkbox("Flip V", &g_flip_on)) g_img_opts_dirty = true;
    ImGui::SameLine(0, 14);
    ImGui::Checkbox("Grid", &g_grid_on);
    if (g_grid_on) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90);
        ImGui::DragInt("##cw", &g_cell_w, 0.2f, 1, 512, "w:%d");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90);
        ImGui::DragInt("##ch", &g_cell_h, 0.2f, 1, 512, "h:%d");
        ImGui::SameLine();
        if (ImGui::SmallButton("32")) { g_cell_w = g_cell_h = 32; }
        ImGui::SameLine();
        if (ImGui::SmallButton("32x48")) { g_cell_w = 32; g_cell_h = 48; }
        ImGui::SameLine();
        if (ImGui::SmallButton("48")) { g_cell_w = g_cell_h = 48; }
    }
    ImGui::SameLine(0, 14);
    ImGui::SetNextItemWidth(110);
    ImGui::Combo("##bg", &g_bg_mode, "checker\0dark\0black\0magenta\0");

    if (g_img_opts_dirty) { rebuild_image_texture(); g_img_opts_dirty = false; }

    // canvas + palette side strip
    float pal_w = g_img.has_palette ? 190.0f : 0.0f;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("##imgcanvas", ImVec2(avail.x - pal_w, avail.y - 24), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    int hx, hy;
    canvas_view(&g_img_tex, &hx, &hy, 8.0f);
    ImGui::EndChild();

    if (g_img.has_palette) {
        ImGui::SameLine();
        ImGui::BeginChild("##pal", ImVec2(pal_w - 8, avail.y - 24), true);
        ImGui::TextDisabled("palette");
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        const float sw = 10.0f;
        for (int i = 0; i < 256; i++) {
            int cx = i % 16, cy = i / 16;
            ImVec2 a(p.x + cx * sw, p.y + cy * sw);
            ImVec2 b(a.x + sw - 1, a.y + sw - 1);
            dl->AddRectFilled(a, b, g_img.palette[i]);
        }
        ImGui::Dummy(ImVec2(16 * sw, 16 * sw + 4));
        ImVec2 mp = ImGui::GetMousePos();
        int pi = -1;
        if (mp.x >= p.x && mp.y >= p.y && mp.x < p.x + 16 * sw && mp.y < p.y + 16 * sw)
            pi = ((int)((mp.y - p.y) / sw)) * 16 + (int)((mp.x - p.x) / sw);
        if (pi >= 0 && pi < 256) {
            uint32_t c = g_img.palette[pi];
            ImGui::Text("[%d] %02X %02X %02X", pi, c & 0xff, (c >> 8) & 0xff, (c >> 16) & 0xff);
        }
        ImGui::EndChild();
    }

    // status line under the canvas
    if (hx >= 0) {
        int sy = g_flip_on ? (g_img.h - 1 - hy) : hy;   // memory row
        if (g_img.bpp == 8 && sy >= 0 && sy < g_img.h) {
            uint8_t idx = g_img.raw[(size_t)sy * g_img.w + hx];
            uint32_t c = g_img.palette[idx];
            ImGui::Text("(%d,%d)  idx %u  rgb %02X%02X%02X", hx, hy, idx,
                        c & 0xff, (c >> 8) & 0xff, (c >> 16) & 0xff);
        } else if (sy >= 0 && sy < g_img.h) {
            uint32_t c = g_img.rgba[(size_t)sy * g_img.w + hx];
            ImGui::Text("(%d,%d)  rgb %02X%02X%02X", hx, hy,
                        c & 0xff, (c >> 8) & 0xff, (c >> 16) & 0xff);
        }
        if (g_grid_on && g_cell_w > 0 && g_cell_h > 0) {
            ImGui::SameLine();
            ImGui::TextDisabled(" cell (%d,%d)", hx / g_cell_w, hy / g_cell_h);
        }
    } else {
        ImGui::TextDisabled("%dx%d %dbpp%s - wheel zoom, drag pan", g_img.w, g_img.h,
                            g_img.bpp, e->kind == RK_SPRITE ? " (lizsoft sheet)" : "");
    }
    (void)hwnd;
}

static void fmt_time(double s, char* out, int cap)
{
    int m = (int)(s / 60);
    _snprintf(out, cap, "%d:%05.2f", m, s - m * 60);
}

static void panel_audio(const RxEntry* e)
{
    const RxWave* wv = &e->wave;
    bool mci = g_use_mci;

    // waveform / banner
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float wf_h = std::max(120.0f, avail.y - 110.0f);
    ImGui::BeginChild("##wave", ImVec2(avail.x, wf_h), true,
                      ImGuiWindowFlags_NoScrollbar);
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 sz = ImGui::GetContentRegionAvail();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    double dur = e->kind == RK_WMA || mci ? g_mci.length_ms / 1000.0 : wv->dur;
    double pos = 0;
    if (!mci && g_pcm.active)
        pos = wv->byterate ? (double)g_pcm.pos_bytes() / wv->byterate : 0;
    else if (mci)
        pos = g_mci.pos_ms() / 1000.0;

    if (g_peaks_ok && sz.x > 8) {
        int n = (int)(g_peaks.size() / 2);
        float mid = p0.y + sz.y * 0.5f;
        float hh = sz.y * 0.48f;
        for (int x = 0; x < (int)sz.x; x++) {
            int c = (int)((float)x / sz.x * n);
            if (c >= n) c = n - 1;
            float lo = g_peaks[c * 2], hi = g_peaks[c * 2 + 1];
            dl->AddLine(ImVec2(p0.x + x, mid - hi * hh), ImVec2(p0.x + x, mid - lo * hh + 1),
                        IM_COL32(195, 232, 141, 160));
        }
        dl->AddLine(ImVec2(p0.x, mid), ImVec2(p0.x + sz.x, mid), IM_COL32(255, 255, 255, 25));
    } else {
        const char* label = e->kind == RK_WMA ? "ASF/WMA stream - decoded by the system codec"
                                              : "compressed WAVE - decoded by the system codec";
        ImVec2 ts = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2(p0.x + (sz.x - ts.x) / 2, p0.y + (sz.y - ts.y) / 2),
                    IM_COL32(137, 221, 255, 180), label);
    }
    // playhead + click-to-seek
    if (dur > 0) {
        float xx = p0.x + (float)(pos / dur) * sz.x;
        dl->AddLine(ImVec2(xx, p0.y), ImVec2(xx, p0.y + sz.y), IM_COL32(232, 179, 75, 255), 2.0f);
        ImGui::InvisibleButton("##seek", sz);
        if (ImGui::IsItemClicked()) {
            double t = (ImGui::GetMousePos().x - p0.x) / sz.x * dur;
            if (t < 0) t = 0;
            if (mci) g_mci.play_from((int)(t * 1000));
            else {
                DWORD off = (DWORD)(t * wv->byterate);
                if (wv->ch && wv->bits) off -= off % (wv->ch * wv->bits / 8);
                g_pcm.play_from(off);
            }
        }
    }
    ImGui::EndChild();

    // transport
    bool playing = mci ? g_mci.is_playing() : (g_pcm.active && !g_pcm.paused);
    if (ImGui::Button(playing ? "Pause" : "Play", ImVec2(80, 0))) {
        if (mci) g_mci.toggle_pause();
        else g_pcm.toggle_pause();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop", ImVec2(60, 0))) { if (mci) g_mci.stop(); else g_pcm.stop(); }
    ImGui::SameLine();
    if (!mci) {
        ImGui::Checkbox("Loop", &g_pcm.loop);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        float v = g_pcm.volume;
        if (ImGui::SliderFloat("##vol", &v, 0.0f, 1.0f, "vol %.2f")) g_pcm.set_volume(v);
        ImGui::SameLine();
    }
    char t0[32], t1[32];
    fmt_time(pos, t0, sizeof t0);
    fmt_time(dur, t1, sizeof t1);
    ImGui::Text("%s / %s", t0, t1);
    ImGui::TextDisabled("%s  %lu Hz  %u-bit  %s  (space = play/pause, click waveform = seek)",
                        rx_wave_tag_name(wv->ok ? wv->tag : 0x161),
                        (unsigned long)(wv->ok ? wv->sr : 0), wv->ok ? wv->bits : 0,
                        wv->ok ? (wv->ch == 1 ? "mono" : "stereo") : "stream");
}

static void panel_map()
{
    if (!g_map_ok) { ImGui::TextDisabled("parse failed"); return; }
    char name[0x21];
    map_data_name(&g_map, name);
    ImGui::Text("%s", name);
    ImGui::SameLine();
    ImGui::TextDisabled("%ux%ux%u, %u objects", g_map.dim0, g_map.dim1, g_map.dim2,
                        g_map.count);
    ImGui::SameLine();
    if (g_map.consumed == 0 || g_sel < 0 ||
        g_map.consumed == g_world.entries[g_sel].size)
        ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.5f, 1), "parses exactly");
    else
        ImGui::TextColored(ImVec4(0.95f, 0.6f, 0.4f, 1), "%u/%lu bytes",
                           (unsigned)g_map.consumed,
                           (unsigned long)g_world.entries[g_sel].size);

    for (uint32_t z = 0; z < g_map.dim2 && z < 8; z++) {
        char lbl[16]; _snprintf(lbl, sizeof lbl, "P%u", z);
        if (z) ImGui::SameLine();
        if (ImGui::Checkbox(lbl, &g_plane_on[z])) g_map_opts_dirty = true;
    }
    ImGui::SameLine(0, 14);
    ImGui::Checkbox("Objects", &g_obj_on);
    ImGui::SameLine(0, 14);
    ImGui::TextDisabled("EFFECT");
    ImGui::SameLine(); ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.3f, 1), "o");
    ImGui::SameLine(0, 10); ImGui::TextDisabled("STRUCT");
    ImGui::SameLine(); ImGui::TextColored(ImVec4(0.4f, 0.6f, 1.0f, 1), "o");
    ImGui::SameLine(0, 10); ImGui::TextDisabled("CHAR");
    ImGui::SameLine(); ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1), "o");
    ImGui::SameLine(0, 10); ImGui::TextDisabled("DEVICE");
    ImGui::SameLine(); ImGui::TextColored(ImVec4(0.95f, 0.4f, 0.4f, 1), "o");
    ImGui::SameLine(0, 10); ImGui::TextDisabled("PLACEHOLDER");
    ImGui::SameLine(); ImGui::TextColored(ImVec4(0.75f, 0.5f, 0.95f, 1), "o");

    if (g_map_opts_dirty) { build_map_texture(); g_map_opts_dirty = false; }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("##mapcanvas", ImVec2(avail.x, avail.y - 24), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    // reuse the image canvas; fit cap high so small maps blow up nicely
    int hx, hy;
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 cavail = ImGui::GetContentRegionAvail();
    canvas_view(&g_map_tex, &hx, &hy, 24.0f);

    // object markers on top (canvas math replicated: fit-or-zoom + center + pan)
    if (g_obj_on && g_map_tex.srv) {
        float fit = std::min(cavail.x / g_map_tex.w, cavail.y / g_map_tex.h);
        if (fit > 24.0f) fit = 24.0f;
        float zoom = (g_zoom <= 0.0f) ? fit : g_zoom;
        ImVec2 img0(p0.x + (cavail.x - g_map_tex.w * zoom) * 0.5f + g_pan.x,
                    p0.y + (cavail.y - g_map_tex.h * zoom) * 0.5f + g_pan.y);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        for (uint32_t i = 0; i < g_map.count; i++) {
            RxMapObject o = rx_map_object(&g_map.layers[i]);
            ImU32 col = o.type >= 90000 ? IM_COL32(191, 128, 242, 230) :
                        o.type >= 80000 ? IM_COL32(242, 102, 102, 230) :
                        o.type >= 70000 ? IM_COL32(102, 230, 128, 230) :
                        o.type >= 60000 ? IM_COL32(102, 153, 255, 230) :
                                          IM_COL32(242, 191, 77, 230);
            // object x/y are tile-pixel coords; cells are 32px tiles
            float fx = img0.x + (o.x / 32.0f) * zoom;
            float fy = img0.y + (o.y / 32.0f) * zoom;
            dl->AddCircleFilled(ImVec2(fx, fy), std::max(2.5f, zoom * 0.18f), col);
            dl->AddCircle(ImVec2(fx, fy), std::max(2.5f, zoom * 0.18f),
                          IM_COL32(0, 0, 0, 200));
            if (ImGui::IsMouseHoveringRect(ImVec2(fx - 5, fy - 5), ImVec2(fx + 5, fy + 5)))
                ImGui::SetTooltip("object %u  type %u (%s)\nsub %u  at (%u, %u)",
                                  o.id, o.type, rx_map_object_category(o.type),
                                  o.subtype, o.x, o.y);
        }
    }
    ImGui::EndChild();

    if (hx >= 0) {
        char parts[128] = "";
        for (uint32_t z = 0; z < g_map.dim2 && z < 8; z++) {
            map_cell c;
            if (map_data_cell(&g_map, hx, hy, z, &c) == 0 && c.tile_id) {
                char one[48];
                _snprintf(one, sizeof one, "  P%u tile %u shape %u", z, c.tile_id, c.shape);
                lstrcatA(parts, one);
            }
        }
        ImGui::Text("cell (%d,%d)%s", hx, hy, parts[0] ? parts : "  (empty)");
    } else {
        ImGui::TextDisabled("hover a cell - colors hash the tile id; markers are the object layer");
    }
}

// ═══ MAP INSPECTOR ═══════════════════════════════════════════════════════════
// Renders a map resource the way the ENGINE does, by running the port's own
// pipeline host-side: map_data_parse -> map_decode (per-tile-id dispatch) ->
// runtime render grid -> map_render_tile geometry -> composite with the real
// sprite-bank sheets (bank registry = the port's ar_register_* tables, driven
// with zdd=NULL and settings=<sotesd HMODULE>).  Coverage is honest: cells
// whose tile id the port hasn't RE'd yet render as hatched placeholders and
// are counted; object layers draw as markers with an inspect panel (their
// spawn/renderer — characters, particles — is engine-side and unported, so
// inspection shows the raw record + a placeholder note).

struct MiSheet {
    bool ok = false;
    RxImage img;
    std::vector<uint32_t> up;   // upright RGBA (flip applied), brightness applied
    int W = 0, H = 0, cw = 0, ch = 0, cols = 0, rows = 0;
    uint32_t key = 0x00ff00ffu; // RGB of the colorkey
    uint16_t res_id = 0;
};

struct MiNode {                 // one draw node + provenance for inspection
    mr_tile t;
    int32_t col, row;
    int     slot;
};

struct MiActorNode {            // one object-layer actor draw (describe result)
    uint16_t bank;
    int16_t  frame;
    int32_t  px, py;            // canvas px (world/100 + descriptor offset)
    uint32_t layer;
    uint8_t  band;              // 0 STRUCTURE, 1 EFFECT, 2 CHARACTER
};

static struct {
    bool built = false, ok = false;
    int  built_for = -1;                 // g_sel the build belongs to
    map_data md {};
    bool md_ok = false;
    uint8_t *grid = nullptr;
    std::vector<MiNode> nodes;           // layer-sorted (stable walk order)
    std::vector<MiActorNode> anodes;     // object-layer actors, layer-sorted
    actor_spawn_pool *pool_struct = nullptr;   // kept for the inspect panel
    actor_spawn_pool *pool_effect = nullptr;
    actor_spawn_pool *pool_char = nullptr;
    int16_t flip_table[1024] = {0};
    Tex tex;
    int px_w = 0, px_h = 0;
    int drawn = 0, unresolved = 0, actors_drawn = 0;
    bool ov_actors = true;
    Tex selobj_tex;                      // resolved object sprite preview
    int selobj_w = 0, selobj_h = 0;
    int selobj_for = -1;                 // which sel_obj the resolve belongs to
    bool selobj_ok = false;
    MiActorNode selobj_node {};
    char selobj_msg[160] = "";
    std::map<uint32_t, int> unknown_ids; // tile_id -> cell count (unported arms)
    int cfg_param4 = 1;                  // room[0x43]: 1=town/house, 4=errands
    // registry
    int  sotesd_mod = -1;
    bool reg_done = false;
    // view state
    float zoom = 0.0f; ImVec2 pan = ImVec2(0, 0);
    bool ov_obj = false, ov_col = false, ov_wall = false, ov_grid = false,
         ov_blend = false, ov_unknown = true;
    int  sel_cx = -1, sel_cy = -1, sel_obj = -1;
    Tex  sel_tex;                        // 4-slot source-cel strip for the panel
    std::map<uint16_t, MiSheet> sheets;
    char status[160] = "";
} g_mi;

static void mi_clear()
{
    if (g_mi.md_ok) { map_data_free(&g_mi.md); g_mi.md_ok = false; }
    if (g_mi.grid) { map_grid_free(g_mi.grid); g_mi.grid = nullptr; }
    g_mi.nodes.clear();
    g_mi.anodes.clear();
    delete g_mi.pool_struct; g_mi.pool_struct = nullptr;
    delete g_mi.pool_effect; g_mi.pool_effect = nullptr;
    delete g_mi.pool_char;   g_mi.pool_char = nullptr;
    g_mi.sheets.clear();
    g_mi.unknown_ids.clear();
    g_mi.tex.release();
    g_mi.sel_tex.release();
    g_mi.selobj_tex.release();
    g_mi.built = g_mi.ok = false;
    g_mi.built_for = -1;
    g_mi.drawn = g_mi.unresolved = g_mi.actors_drawn = 0;
    g_mi.sel_cx = g_mi.sel_cy = -1; g_mi.sel_obj = -1;
    g_mi.zoom = 0.0f; g_mi.pan = ImVec2(0, 0);
}

static void mi_reset_world()
{
    mi_clear();
    g_mi.reg_done = false;      // pool slots point into freed modules
    g_mi.sotesd_mod = -1;
}

// find a loaded module by basename; prefer the one sharing the dir of `like`.
static int mi_find_module(const char *base, int like)
{
    int found = -1;
    for (int i = 0; i < (int)g_world.modules.size(); i++) {
        if (lstrcmpiA(g_world.modules[i].label, base)) continue;
        if (found < 0) found = i;
        if (like >= 0) {
            wchar_t a[MAX_PATH], b[MAX_PATH];
            lstrcpynW(a, g_world.modules[i].path, MAX_PATH);
            lstrcpynW(b, g_world.modules[like].path, MAX_PATH);
            wchar_t *sa = wcsrchr(a, L'\\'), *sb = wcsrchr(b, L'\\');
            if (sa && sb) {
                *sa = 0; *sb = 0;
                if (!_wcsicmp(a, b)) return i;   // same dir wins outright
            }
        }
    }
    return found;
}

// Run the port's registration tables once so ar_pool_get_slot(bank) resolves.
// zdd = NULL (no DDraw hooks fire — frames are never built through the pool
// here); settings doubles as the HMODULE the lazy decoder reads (see
// asset_register.h: slot->settings is the FindResource module).
static bool mi_ensure_registry(int map_mod)
{
    if (g_mi.reg_done && g_mi.sotesd_mod >= 0) return true;
    int sd = mi_find_module("sotesd.dll", map_mod);
    if (sd < 0) {
        lstrcpynA(g_mi.status, "sotesd.dll not loaded - open the game install "
                  "to resolve tile sprite banks", sizeof g_mi.status);
        return false;
    }
    void *h = (void *)g_world.modules[sd].mod;
    ar_state_init();   // builds the slot/info POINTER tables — everything
                       // below (and ar_pool_get_slot) dereferences them
    // NOT ar_register_palette_ramps: it decodes ramp palettes at registration
    // time (engine-coupled) and the ramp pool (idx 1..12) holds no tile banks;
    // a map that ever references one shows up in the unresolved counter.

    ar_register_group3_sprites(NULL, 3, h);

    ar_register_main_sprites(NULL, 4, h, h);   // sotesp_module = sotesd in retail boot too

    ar_register_game_sprites(NULL, 5, h);

    g_mi.sotesd_mod = sd;
    g_mi.reg_done = true;
    return true;
}

static const RxEntry *mi_find_entry(int mod, uint16_t resid)
{
    const RxModule &m = g_world.modules[mod];
    for (int i = m.first_entry; i < m.first_entry + m.n_entries; i++) {
        const RxEntry &e = g_world.entries[i];
        if (!e.name.isStr && e.name.id == resid && !strcmp(e.type_label, "DATA"))
            return &e;
    }
    return nullptr;
}

// decode + upright + per-slot 24bpp brightness/LUT pass (FUN_004184a0's
// transform: LUT then channel scale /1000, magenta skipped — quirk #46 channel
// order: f_0c scales R (byte2), f_10 G, f_14 B).
static MiSheet *mi_sheet(uint16_t bank)
{
    auto it = g_mi.sheets.find(bank);
    if (it != g_mi.sheets.end()) return &it->second;
    MiSheet &sh = g_mi.sheets[bank];
    ar_sprite_slot *slot = ar_pool_get_slot(bank);
    if (!slot || !slot->resource_id) return &sh;
    sh.res_id = slot->resource_id;
    const RxEntry *e = mi_find_entry(g_mi.sotesd_mod, slot->resource_id);
    if (!e || !rx_decode_image(&g_world, e, &sh.img)) return &sh;
    rx_image_display(&sh.img, /*key*/0, /*flip*/sh.img.default_flip, &sh.up);
    sh.W = sh.img.w; sh.H = sh.img.h;
    sh.cw = slot->width ? (int)slot->width : sh.W;
    sh.ch = slot->height ? (int)slot->height : sh.H;
    if (sh.cw > sh.W) sh.cw = sh.W;
    if (sh.ch > sh.H) sh.ch = sh.H;
    sh.cols = sh.cw ? sh.W / sh.cw : 0;
    sh.rows = sh.ch ? sh.H / sh.ch : 0;
    sh.key = slot->colorkey ? (slot->colorkey & 0xffffff) : 0x00ff00ffu;
    // 8bpp keying is by PALETTE INDEX: the display-depth conversion
    // (bs_convert_*, driven by the slicer's format hook) substitutes pixels
    // whose index == slot->colorkey with magenta, and the cel builder keys
    // magenta (quirk #47).  Reproduce the substitution here so the blit's
    // magenta test keys them (group3 scenery/tile banks use index 0; the
    // 0x1ffffff sentinel and out-of-range values match nothing — faithful
    // to bs_convert's exact `idx == colorkey` compare).
    if (sh.img.bpp == 8 && slot->colorkey != 0x1ffffffu) {
        uint32_t ck = slot->colorkey;
        for (int y = 0; y < sh.H; y++) {
            const uint8_t *idx = sh.img.raw.data() +
                (size_t)(sh.img.default_flip ? (sh.H - 1 - y) : y) * sh.W;
            uint32_t *row = sh.up.data() + (size_t)y * sh.W;
            for (int x = 0; x < sh.W; x++)
                if ((uint32_t)idx[x] == ck) row[x] = 0xffff00ffu;
        }
        sh.key = 0x00ff00ffu;
    }
    if (slot->f_08 && sh.img.bpp == 24) {
        const uint8_t *lut = (const uint8_t *)(uintptr_t)slot->f_18;
        for (uint32_t &c : sh.up) {
            uint32_t r = c & 0xff, g = (c >> 8) & 0xff, b = (c >> 16) & 0xff;
            if (r == 0xff && g == 0x00 && b == 0xff) continue;
            if (lut) { r = lut[r]; g = lut[g]; b = lut[b]; }
            if (slot->f_0c) r = r * slot->f_0c / 1000;
            if (slot->f_10) g = g * slot->f_10 / 1000;
            if (slot->f_14) b = b * slot->f_14 / 1000;
            c = 0xff000000u | ((b & 0xff) << 16) | ((g & 0xff) << 8) | (r & 0xff);
        }
    }
    sh.ok = (sh.cols > 0 && sh.rows > 0);
    return &sh;
}

// blit one draw node's 32x32 source rect out of its bank sheet.  Frame index
// order comes from ar_sprite_slice's trim/build loop: base_y walks
// `bottom_ptr - (base_y+row)*stride` with bottom_ptr = the LAST memory row =
// the VISUAL TOP of the bottom-up DIB — so frame 0 is the visual TOP-LEFT
// cell and frames run left-to-right, top-to-bottom in the UPRIGHT image.
// (The first cut mirrored the row order; single-row roof banks hid it, the
// multi-row ground/house atlases mangled — USER-caught.)  The node's src
// rect offsets are cel-surface coords = upright top-left, too.
static void mi_blit_rect(std::vector<uint32_t> &cv, const MiSheet *sh,
                         int sx, int sy, int w, int h, int dx, int dy)
{
    for (int y = 0; y < h; y++) {
        int yy = sy + y, dyy = dy + y;
        if (yy < 0 || yy >= sh->H || dyy < 0 || dyy >= g_mi.px_h) continue;
        const uint32_t *src = sh->up.data() + (size_t)yy * sh->W;
        uint32_t *dst = cv.data() + (size_t)dyy * g_mi.px_w;
        for (int x = 0; x < w; x++) {
            int xx = sx + x, dxx = dx + x;
            if (xx < 0 || xx >= sh->W || dxx < 0 || dxx >= g_mi.px_w) continue;
            uint32_t c = src[xx];
            if ((c & 0x00ffffffu) == 0x00ff00ffu) continue;         // magenta
            if ((((c & 0xff) << 16) | (c & 0xff00) | ((c >> 16) & 0xff))
                == sh->key && sh->key != 0x00ff00ffu) continue;     // slot key
            dst[dxx] = c;
        }
    }
}

static void mi_blit_node(std::vector<uint32_t> &cv, const mr_tile &t)
{
    MiSheet *sh = mi_sheet(t.bank);
    if (!sh->ok) { g_mi.unresolved++; return; }
    int f = t.frame;
    if (f >= sh->cols * sh->rows) { g_mi.unresolved++; return; }
    int vrow = f / sh->cols, vcol = f % sh->cols;
    mi_blit_rect(cv, sh, vcol * sh->cw + t.src_x, vrow * sh->ch + t.src_y,
                 t.w, t.h, t.dst_x / 100, t.dst_y / 100);
    g_mi.drawn++;
}

// one object-layer actor: a full cel at world/100 + descriptor offset (the
// map_present mode-0 projection with the camera at the canvas origin).
static void mi_blit_actor(std::vector<uint32_t> &cv, const MiActorNode &an)
{
    MiSheet *sh = mi_sheet(an.bank);
    if (!sh->ok) { g_mi.unresolved++; return; }
    int f = an.frame;
    if (f < 0 || f >= sh->cols * sh->rows) { g_mi.unresolved++; return; }
    int vrow = f / sh->cols, vcol = f % sh->cols;
    mi_blit_rect(cv, sh, vcol * sh->cw, vrow * sh->ch, sh->cw, sh->ch,
                 an.px, an.py);
    g_mi.actors_drawn++;
}

// spawn the map's object-layer bands through the port's own spawn pass
// (0x58d460 dispatch: STRUCTURE fully map-driven, EFFECT/CHARACTER via the
// captured def tables) and describe each visible actor into an MiActorNode.
static void mi_spawn_actors()
{
    g_mi.pool_struct = new actor_spawn_pool();
    g_mi.pool_effect = new actor_spawn_pool();
    g_mi.pool_char   = new actor_spawn_pool();
    actor_spawn_effect_fill_flip_table(g_mi.flip_table,
                                       sizeof g_mi.flip_table / sizeof *g_mi.flip_table);
    actor_spawn_struct_from_map(g_mi.pool_struct, &g_mi.md);
    actor_spawn_effect_from_map(g_mi.pool_effect, &g_mi.md, NULL);
    actor_spawn_from_map(g_mi.pool_char, &g_mi.md);

    const actor_spawn_pool *pools[3] =
        { g_mi.pool_struct, g_mi.pool_effect, g_mi.pool_char };
    for (int b = 0; b < 3; b++) {
        const actor_spawn_pool *p = pools[b];
        if (!p) continue;
        for (int i = 0; i < p->count; i++) {
            if (p->actors[i].skip) continue;
            actor_desc d;
            if (!actor_render_describe(&p->actors[i], &p->states[i],
                                       g_mi.flip_table, &d))
                continue;                       // invisible volume / bank 0
            MiActorNode an;
            an.bank = d.bank;
            an.frame = d.frame;
            // actor_emit_part: node offset = descriptor off + render-state
            // dst_base (the EFFECT band's per-code anchor; 0 for the others —
            // omitting it shifted the townsfolk by exactly the anchor).
            an.px = p->states[i].world_x / 100 + d.off_x + p->states[i].dst_base_x;
            an.py = p->states[i].world_y / 100 + d.off_y + p->states[i].dst_base_y;
            an.layer = p->actors[i].layer;
            an.band = (uint8_t)b;
            g_mi.anodes.push_back(an);
        }
    }
    std::stable_sort(g_mi.anodes.begin(), g_mi.anodes.end(),
                     [](const MiActorNode &a, const MiActorNode &b)
                     { return a.layer < b.layer; });
}

static void mi_bank_dims_cb(void *, uint16_t bank, int32_t *w, int32_t *h)
{
    ar_sprite_slot *s = ar_pool_get_slot(bank);
    if (s) { *w = (int32_t)s->width; *h = (int32_t)s->height; }
    else   { *w = 0; *h = 0; }
}

static void mi_build()
{
    mi_clear();
    g_mi.built = true;
    g_mi.built_for = g_sel;
    if (g_sel < 0) return;
    const RxEntry *e = &g_world.entries[g_sel];
    if (e->kind != RK_MAP) return;
    if (!mi_ensure_registry(e->module_idx)) return;

    if (!rx_parse_map(&g_world, e, &g_mi.md)) {
        lstrcpynA(g_mi.status, "map parse failed", sizeof g_mi.status);
        return;
    }
    g_mi.md_ok = true;
    int d0 = (int)g_mi.md.dim0, d1 = (int)g_mi.md.dim1;
    if (d0 <= 0 || d1 <= 0 || d0 > 0x80 || d1 > 0x80) {
        lstrcpynA(g_mi.status, "map dims out of runtime-grid range", sizeof g_mi.status);
        return;
    }

    g_mi.grid = map_grid_alloc();
    if (!g_mi.grid) return;
    map_decode_cfg cfg;
    map_decode_cfg_init(&cfg, MAP_DECODE_SCENE_PARAM3, g_mi.cfg_param4);
    map_decode(&g_mi.md, g_mi.grid, &cfg, mi_bank_dims_cb, nullptr);

    // collect draw nodes in retail walk order (rows outer, cols inner, slots
    // 0..3), then stable-sort by layer = the draw_pool flush order.
    for (int row = 0; row < d1; row++)
        for (int col = 0; col < d0; col++)
            for (int s = 0; s < 4; s++) {
                MiNode n; n.col = col; n.row = row; n.slot = s;
                if (map_render_tile(g_mi.grid, col, row, s, &n.t))
                    g_mi.nodes.push_back(n);
            }
    std::stable_sort(g_mi.nodes.begin(), g_mi.nodes.end(),
                     [](const MiNode &a, const MiNode &b) { return a.t.layer < b.t.layer; });

    // honest coverage: a cell with a tile id but no grid writes = unported arm
    for (int y = 0; y < d1; y++)
        for (int x = 0; x < d0; x++) {
            uint32_t idmax = 0;
            for (int z = 0; z < (int)g_mi.md.dim2; z++) {
                map_cell c;
                if (map_data_cell(&g_mi.md, x, y, z, &c) == 0 && c.tile_id)
                    idmax = c.tile_id;   // any id on the cell
            }
            if (!idmax) continue;
            uint32_t idx = map_render_grid_index(x, y);
            bool touched = false;
            for (int s = 0; s < 4 && !touched; s++) {
                mr_tile t;
                touched = map_render_tile(g_mi.grid, x, y, s, &t) != 0;
            }
            if (!touched && map_grid_obj_class(g_mi.grid, x, y) == 0 &&
                map_grid_flag(g_mi.grid, x, y) == 0) {
                (void)idx;
                for (int z = 0; z < (int)g_mi.md.dim2; z++) {
                    map_cell c;
                    if (map_data_cell(&g_mi.md, x, y, z, &c) == 0 && c.tile_id)
                        g_mi.unknown_ids[c.tile_id]++;
                }
            }
        }

    // object-layer actors (props/structures/townsfolk) — the engine's frame
    // draws them interleaved with the tiles by draw_pool LAYER, so merge the
    // two layer-sorted lists (ties: tiles first, matching the walk-then-bands
    // emit order of the per-frame draw driver).
    if (g_mi.ov_actors) mi_spawn_actors();

    // composite
    g_mi.px_w = d0 * 32; g_mi.px_h = d1 * 32;
    std::vector<uint32_t> cv((size_t)g_mi.px_w * g_mi.px_h, 0xff101014u);
    {
        size_t ti = 0, ai = 0;
        while (ti < g_mi.nodes.size() || ai < g_mi.anodes.size()) {
            bool take_tile;
            if (ti >= g_mi.nodes.size()) take_tile = false;
            else if (ai >= g_mi.anodes.size()) take_tile = true;
            else take_tile = g_mi.nodes[ti].t.layer <= g_mi.anodes[ai].layer;
            if (take_tile) mi_blit_node(cv, g_mi.nodes[ti++].t);
            else           mi_blit_actor(cv, g_mi.anodes[ai++]);
        }
    }
    // hatch unported cells
    if (!g_mi.unknown_ids.empty())
        for (int y = 0; y < d1; y++)
            for (int x = 0; x < d0; x++) {
                bool unk = false;
                for (int z = 0; z < (int)g_mi.md.dim2 && !unk; z++) {
                    map_cell c;
                    if (map_data_cell(&g_mi.md, x, y, z, &c) == 0 && c.tile_id &&
                        g_mi.unknown_ids.count(c.tile_id)) {
                        mr_tile t; bool touched = false;
                        for (int s = 0; s < 4 && !touched; s++)
                            touched = map_render_tile(g_mi.grid, x, y, s, &t) != 0;
                        unk = !touched;
                    }
                }
                if (!unk) continue;
                for (int py = 0; py < 32; py++)
                    for (int px = 0; px < 32; px++)
                        if (((px + py) & 7) < 2)
                            cv[(size_t)(y * 32 + py) * g_mi.px_w + x * 32 + px] =
                                0xff2a2aE0u;   // faint red hatch
            }
    if (g_mi.tex.ensure(g_mi.px_w, g_mi.px_h)) g_mi.tex.upload(cv.data());
    g_mi.ok = true;
    _snprintf(g_mi.status, sizeof g_mi.status,
              "%d tiles + %d actors drawn, %d unresolved, %d unported tile id(s)",
              g_mi.drawn, g_mi.actors_drawn, g_mi.unresolved,
              (int)g_mi.unknown_ids.size());
}

// build the selected cell's 4-slot source-cel strip (128x32, shown zoomed)
static void mi_build_sel_tex()
{
    if (g_mi.sel_cx < 0 || !g_mi.grid) return;
    std::vector<uint32_t> strip((size_t)128 * 32, 0xff17181cu);
    int save_w = g_mi.px_w, save_h = g_mi.px_h;
    g_mi.px_w = 128; g_mi.px_h = 32;
    for (int s = 0; s < 4; s++) {
        mr_tile t;
        if (!map_render_tile(g_mi.grid, g_mi.sel_cx, g_mi.sel_cy, s, &t)) continue;
        mr_tile local = t;
        local.dst_x = s * 32 * 100;   // strip slot position (world units /100)
        local.dst_y = 0;
        int save_d = g_mi.drawn, save_u = g_mi.unresolved;
        mi_blit_node(strip, local);
        g_mi.drawn = save_d; g_mi.unresolved = save_u;
    }
    g_mi.px_w = save_w; g_mi.px_h = save_h;
    if (g_mi.sel_tex.ensure(128, 32)) g_mi.sel_tex.upload(strip.data());
}

static const char *mi_blend_name(uint32_t va)
{
    switch (va) {
        case MD_BLEND_5cc390: return "MD_BLEND_5cc390";
        case MD_BLEND_5cc3b0: return "MD_BLEND_5cc3b0";
        case MD_BLEND_5cc3d0: return "MD_BLEND_5cc3d0";
        case MD_BLEND_5cc3f0: return "MD_BLEND_5cc3f0";
        case MD_BLEND_5cc410: return "MD_BLEND_5cc410";
        case MD_BLEND_5cc430: return "MD_BLEND_5cc430";
        default: return nullptr;
    }
}

static void mi_inspect_cell_panel()
{
    ImGui::Text("cell (%d, %d)", g_mi.sel_cx, g_mi.sel_cy);
    ImGui::Separator();
    for (int z = 0; z < (int)g_mi.md.dim2; z++) {
        map_cell c;
        if (map_data_cell(&g_mi.md, g_mi.sel_cx, g_mi.sel_cy, z, &c) != 0) continue;
        if (!c.tile_id) { ImGui::TextDisabled("P%d: empty", z); continue; }
        bool unk = g_mi.unknown_ids.count(c.tile_id) != 0;
        ImGui::Text("P%d: tile 0x%x%s", z, c.tile_id, unk ? "  [UNPORTED]" : "");
        if (unk)
            ImGui::TextColored(ImVec4(0.95f, 0.5f, 0.4f, 1),
                               "   decode arm not RE'd yet (map_decode.c)");
        ImGui::TextDisabled("   co_id 0x%x  f08 %u  arg0c %u  shape %u  a14 %u a18 %u",
                            c.f00, c.f08, c.arg_0c, c.shape, c.arg_14, c.arg_18);
    }
    ImGui::Separator();
    ImGui::TextDisabled("render grid (region A sub-slots)");
    bool any = false;
    for (int s = 0; s < 4; s++) {
        mr_tile t;
        if (!map_render_tile(g_mi.grid, g_mi.sel_cx, g_mi.sel_cy, s, &t)) continue;
        any = true;
        ar_sprite_slot *slot = ar_pool_get_slot(t.bank);
        ImGui::Text("S%d: bank 0x%x (res %u) frame %u", s, t.bank,
                    slot ? slot->resource_id : 0, t.frame);
        ImGui::TextDisabled("    layer %u  subtile (%d, %d) px", t.layer,
                            t.src_x, t.src_y);
    }
    if (!any) ImGui::TextDisabled("  (no tile draws)");
    if (any && g_mi.sel_tex.srv) {
        ImGui::TextDisabled("source cels (S0..S3):");
        draw_nearest_begin(ImGui::GetWindowDrawList());
        ImGui::Image((ImTextureID)g_mi.sel_tex.srv, ImVec2(128 * 3, 32 * 3));
        draw_nearest_end(ImGui::GetWindowDrawList());
    }
    ImGui::Separator();
    uint16_t cls = map_grid_obj_class(g_mi.grid, g_mi.sel_cx, g_mi.sel_cy);
    uint32_t slope = map_grid_obj_slope(g_mi.grid, g_mi.sel_cx, g_mi.sel_cy);
    int16_t wall = map_grid_flag(g_mi.grid, g_mi.sel_cx, g_mi.sel_cy);
    ImGui::TextDisabled("collision (region B/D)");
    if (cls || slope || wall) {
        ImGui::Text("class %u%s", cls,
                    cls == 10 ? " (solid wall)" : cls == 1 ? " (slope test)" : "");
        if (slope) ImGui::Text("slope profile 0x%x (engine .rdata)", slope);
        if (wall) ImGui::Text("wall flag %d", wall);
    } else ImGui::TextDisabled("  none");
    const uint8_t *cc = g_mi.grid + MG_REGION_C +
        (size_t)map_render_grid_index(g_mi.sel_cx, g_mi.sel_cy) * 0x0c;
    uint32_t c0, c4; uint16_t c8;
    memcpy(&c0, cc, 4); memcpy(&c4, cc + 4, 4); memcpy(&c8, cc + 8, 2);
    if (c0 || c4 || c8) {
        const char *bn = mi_blend_name(c4);
        ImGui::TextDisabled("blend/overlay (region C)");
        ImGui::Text("%08x %08x %04x%s%s", c0, c4, c8, bn ? "  " : "", bn ? bn : "");
        ImGui::TextDisabled("  drawn by the (unported) 490f30 region-C arm");
    }
}

static void mi_inspect_obj_panel()
{
    const map_layer *l = &g_mi.md.layers[g_mi.sel_obj];
    RxMapObject o = rx_map_object(l);
    ImGui::Text("object #%d  id %u", g_mi.sel_obj, o.id);
    ImGui::Text("type %u  (%s)", o.type, rx_map_object_category(o.type));
    if (o.type >= 70000 && o.type <= 79999)
        ImGui::TextDisabled("(the engine's own band name — \"Character Object\";"
                            " it holds NPCs AND static props / emitter fixtures)");
    ImGui::Text("subtype %u   at (%u, %u) px", o.subtype, o.x, o.y);
    ImGui::TextDisabled("sub-array counts a/b/c/d: %u / %u / %u / %u",
                        l->n_a, l->n_b, l->n_c, l->n_d);
    ImGui::Separator();
    ImGui::TextDisabled("layer header (0x3c B)");
    ImGui::PushFont(g_font_mono);
    for (int r = 0; r < 4; r++) {
        char line[80]; int n = 0;
        for (int i = 0; i < 15 && r * 15 + i < 0x3c; i++)
            n += _snprintf(line + n, sizeof line - n, "%02x ", l->hdr[r * 15 + i]);
        line[n] = 0;
        ImGui::TextUnformatted(line);
    }
    ImGui::PopFont();
    ImGui::Separator();

    // resolve the object through the port's spawn pass: match the spawned
    // actor whose logical map position equals this record's (x, y).  EFFECT
    // world pos is (map - dst_anchor)*100 with dst_base = the anchor, so
    // map == world/100 + dst_base uniformly across the bands.
    if (g_mi.selobj_for != g_mi.sel_obj) {
        g_mi.selobj_for = g_mi.sel_obj;
        g_mi.selobj_ok = false;
        g_mi.selobj_msg[0] = 0;
        const actor_spawn_pool *pools[3] =
            { g_mi.pool_struct, g_mi.pool_effect, g_mi.pool_char };
        int found_band = -1, found_i = -1;
        for (int b = 0; b < 3 && found_band < 0; b++) {
            const actor_spawn_pool *p = pools[b];
            if (!p) continue;
            for (int i = 0; i < p->count; i++) {
                if (p->states[i].world_x / 100 + p->states[i].dst_base_x == (int)o.x &&
                    p->states[i].world_y / 100 + p->states[i].dst_base_y == (int)o.y) {
                    found_band = b; found_i = i;
                    break;
                }
            }
        }
        if (found_band >= 0) {
            const actor_spawn_pool *p = pools[found_band];
            actor_desc d;
            if (!p->actors[found_i].skip &&
                actor_render_describe(&p->actors[found_i], &p->states[found_i],
                                      g_mi.flip_table, &d)) {
                g_mi.selobj_ok = true;
                g_mi.selobj_node.bank = d.bank;
                g_mi.selobj_node.frame = d.frame;
                g_mi.selobj_node.layer = p->actors[found_i].layer;
                g_mi.selobj_node.band = (uint8_t)found_band;
                MiSheet *sh = mi_sheet(d.bank);
                if (sh->ok && d.frame >= 0 && d.frame < sh->cols * sh->rows) {
                    // single-cel preview texture
                    std::vector<uint32_t> cel((size_t)sh->cw * sh->ch, 0xff17181cu);
                    int vrow = d.frame / sh->cols, vcol = d.frame % sh->cols;
                    for (int y = 0; y < sh->ch; y++) {
                        const uint32_t *src = sh->up.data() +
                            (size_t)(vrow * sh->ch + y) * sh->W + vcol * sh->cw;
                        for (int x = 0; x < sh->cw; x++) {
                            uint32_t c = src[x];
                            if ((c & 0x00ffffffu) != 0x00ff00ffu)
                                cel[(size_t)y * sh->cw + x] = c;
                        }
                    }
                    g_mi.selobj_w = sh->cw; g_mi.selobj_h = sh->ch;
                    if (g_mi.selobj_tex.ensure(sh->cw, sh->ch))
                        g_mi.selobj_tex.upload(cel.data());
                }
            } else {
                lstrcpynA(g_mi.selobj_msg, "spawns as an INVISIBLE volume - "
                          "collision / trigger / spawn marker (zero sprite bank, "
                          "exactly retail's end state)", sizeof g_mi.selobj_msg);
            }
        } else if (o.type >= 50000 && o.type <= 59999) {
            lstrcpynA(g_mi.selobj_msg, "not in the captured EFFECT def table "
                      "(wandering/animated - PORT-DEBT effect-sprite-table)",
                      sizeof g_mi.selobj_msg);
        } else if (o.type >= 60000 && o.type <= 69999) {
            lstrcpynA(g_mi.selobj_msg, "code not in the 0x438a60 STRUCTURE def "
                      "switch (retail's default: draws nothing)", sizeof g_mi.selobj_msg);
        } else if (o.type == 0x15f9a || o.type == 0x15f9b) {
            // not a spawn: the map decoder's trailing FUN_0058cb30 pass turned
            // this into a region-E link-anchor record — show it.
            if (g_mi.grid) {
                int cx = (int)o.x / 32, cy = (int)o.y / 32;
                const uint8_t *rec = g_mi.grid + MG_REGION_E +
                    (size_t)(cx * (int)MG_ROW_PITCH + cy) * MG_REGION_E_STRIDE;
                uint16_t n;   memcpy(&n, rec + 0x00, 2);
                uint32_t fl;  memcpy(&fl, rec + 0x04, 4);
                int32_t lx = 0, ly = 0;
                if (n) { memcpy(&lx, rec + 0x08, 4); memcpy(&ly, rec + 0x0c, 4); }
                _snprintf(g_mi.selobj_msg, sizeof g_mi.selobj_msg,
                          "region-E link anchor (map-decode 0x58cb30 pass, not a "
                          "spawn): flag %u, %u link(s)%s%s cell (%d,%d)",
                          fl, n, n ? ", first ->" : "", n ? "" : ";", lx, ly);
            } else {
                lstrcpynA(g_mi.selobj_msg, "region-E link anchor (map-decode "
                          "0x58cb30 pass, not a spawn)", sizeof g_mi.selobj_msg);
            }
        } else if (o.type >= 80000 && o.type <= 89999) {
            lstrcpynA(g_mi.selobj_msg, "DEVICE band - spawn/renderer not ported "
                      "yet (PORT-DEBT; no shipped map places DEVICE objects)",
                      sizeof g_mi.selobj_msg);
        } else {
            lstrcpynA(g_mi.selobj_msg, "no spawned actor matched this record",
                      sizeof g_mi.selobj_msg);
        }
    }

    if (g_mi.selobj_ok) {
        ar_sprite_slot *slot = ar_pool_get_slot(g_mi.selobj_node.bank);
        ImGui::Text("renders: bank 0x%x (res %u) frame %d, layer %u",
                    g_mi.selobj_node.bank, slot ? slot->resource_id : 0,
                    g_mi.selobj_node.frame, g_mi.selobj_node.layer);
        if (g_mi.selobj_tex.srv) {
            float sc = std::min(3.0f, 220.0f / std::max(1, g_mi.selobj_w));
            draw_nearest_begin(ImGui::GetWindowDrawList());
            ImGui::Image((ImTextureID)g_mi.selobj_tex.srv,
                         ImVec2(g_mi.selobj_w * sc, g_mi.selobj_h * sc));
            draw_nearest_end(ImGui::GetWindowDrawList());
        }
        ImGui::TextDisabled("(spawned by the port's 0x58d460 dispatch port,"
                            " described by FUN_0044d160)");
    } else {
        ImGui::TextWrapped("Renders: %s", g_mi.selobj_msg);
    }
}

static void mi_draw_tab(HWND hwnd)
{
    (void)hwnd;
    if (!g_mi.built || g_mi.built_for != g_sel) mi_build();
    if (!g_mi.ok) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.95f, 0.7f, 0.3f, 1), "%s", g_mi.status);
        return;
    }

    // toolbar
    if (ImGui::Button("Fit")) { g_mi.zoom = 0; g_mi.pan = ImVec2(0, 0); }
    ImGui::SameLine();
    if (ImGui::Button("1:1")) { g_mi.zoom = 1; g_mi.pan = ImVec2(0, 0); }
    ImGui::SameLine(0, 12);
    if (ImGui::Checkbox("Actors", &g_mi.ov_actors)) { mi_build(); return; }
    ImGui::SameLine();
    ImGui::Checkbox("Objects", &g_mi.ov_obj); ImGui::SameLine();
    ImGui::Checkbox("Collision", &g_mi.ov_col); ImGui::SameLine();
    ImGui::Checkbox("Walls", &g_mi.ov_wall); ImGui::SameLine();
    ImGui::Checkbox("Blend", &g_mi.ov_blend); ImGui::SameLine();
    ImGui::Checkbox("Grid##mi", &g_mi.ov_grid); ImGui::SameLine();
    ImGui::Checkbox("Unported", &g_mi.ov_unknown);
    ImGui::SameLine(0, 12);
    ImGui::SetNextItemWidth(120);
    int cfg_idx = (g_mi.cfg_param4 == 4) ? 1 : 0;
    if (ImGui::Combo("##micfg", &cfg_idx, "town/house\0errands\0")) {
        g_mi.cfg_param4 = cfg_idx ? 4 : 1;
        mi_build();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", g_mi.status);

    // canvas | inspect split
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float panel_w = 340.0f;
    ImGui::BeginChild("##micanvas", ImVec2(avail.x - panel_w, avail.y), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImVec2 cav = ImGui::GetContentRegionAvail();
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##micv", cav,
            ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
        bool hovered = ImGui::IsItemHovered();
        ImDrawList *dl = ImGui::GetWindowDrawList();
        dl->PushClipRect(p0, ImVec2(p0.x + cav.x, p0.y + cav.y), true);
        dl->AddRectFilled(p0, ImVec2(p0.x + cav.x, p0.y + cav.y), IM_COL32(16, 16, 20, 255));

        float fit = std::min(cav.x / g_mi.px_w, cav.y / g_mi.px_h);
        if (fit > 6.0f) fit = 6.0f;
        float zoom = (g_mi.zoom <= 0.0f) ? fit : g_mi.zoom;
        ImVec2 img0(p0.x + (cav.x - g_mi.px_w * zoom) * 0.5f + g_mi.pan.x,
                    p0.y + (cav.y - g_mi.px_h * zoom) * 0.5f + g_mi.pan.y);
        ImVec2 img1(img0.x + g_mi.px_w * zoom, img0.y + g_mi.px_h * zoom);
        bool nearest = zoom >= 2.0f;
        if (nearest) draw_nearest_begin(dl);
        dl->AddImage((ImTextureID)g_mi.tex.srv, img0, img1);
        if (nearest) draw_nearest_end(dl);
        dl->AddRect(ImVec2(img0.x - 1, img0.y - 1), ImVec2(img1.x + 1, img1.y + 1),
                    IM_COL32(90, 90, 100, 255));

        float cs = 32.0f * zoom;   // one cell on screen
        int d0 = (int)g_mi.md.dim0, d1 = (int)g_mi.md.dim1;

        if (g_mi.ov_grid && cs >= 6)
            for (int x = 1; x < d0; x++)
                dl->AddLine(ImVec2(img0.x + x * cs, img0.y), ImVec2(img0.x + x * cs, img1.y),
                            IM_COL32(255, 255, 255, 22));
        if (g_mi.ov_grid && cs >= 6)
            for (int y = 1; y < d1; y++)
                dl->AddLine(ImVec2(img0.x, img0.y + y * cs), ImVec2(img1.x, img0.y + y * cs),
                            IM_COL32(255, 255, 255, 22));

        // per-cell overlays
        for (int y = 0; y < d1; y++)
            for (int x = 0; x < d0; x++) {
                ImVec2 a(img0.x + x * cs, img0.y + y * cs);
                ImVec2 b(a.x + cs, a.y + cs);
                if (g_mi.ov_col) {
                    uint16_t cls = map_grid_obj_class(g_mi.grid, x, y);
                    if (cls == 10)
                        dl->AddRectFilled(a, b, IM_COL32(230, 60, 60, 60));
                    else if (cls == 1)
                        dl->AddRectFilled(a, b, IM_COL32(60, 220, 90, 60));
                    else if (cls)
                        dl->AddRectFilled(a, b, IM_COL32(230, 200, 60, 60));
                    if (cls && map_grid_obj_slope(g_mi.grid, x, y))
                        dl->AddLine(ImVec2(a.x, b.y), ImVec2(b.x, a.y),
                                    IM_COL32(60, 220, 90, 180), 2.0f);
                }
                if (g_mi.ov_wall && map_grid_flag(g_mi.grid, x, y))
                    dl->AddRect(a, b, IM_COL32(240, 150, 40, 200), 0, 0, 2.0f);
                if (g_mi.ov_blend) {
                    const uint8_t *cc = g_mi.grid + MG_REGION_C +
                        (size_t)map_render_grid_index(x, y) * 0x0c;
                    uint32_t c0, c4; memcpy(&c0, cc, 4); memcpy(&c4, cc + 4, 4);
                    if (c0 || c4)
                        dl->AddRect(a, b, IM_COL32(80, 220, 255, 200));
                }
            }

        // object markers
        if (g_mi.ov_obj)
            for (uint32_t i = 0; i < g_mi.md.count; i++) {
                RxMapObject o = rx_map_object(&g_mi.md.layers[i]);
                float fx = img0.x + o.x * zoom, fy = img0.y + o.y * zoom;
                ImU32 col = o.type >= 90000 ? IM_COL32(191, 128, 242, 240) :
                            o.type >= 80000 ? IM_COL32(242, 102, 102, 240) :
                            o.type >= 70000 ? IM_COL32(102, 230, 128, 240) :
                            o.type >= 60000 ? IM_COL32(102, 153, 255, 240) :
                                              IM_COL32(242, 191, 77, 240);
                float rr = std::max(3.0f, zoom * 5.0f);
                dl->AddCircleFilled(ImVec2(fx, fy), rr, col);
                dl->AddCircle(ImVec2(fx, fy), rr, IM_COL32(0, 0, 0, 220));
                if ((int)i == g_mi.sel_obj)
                    dl->AddCircle(ImVec2(fx, fy), rr + 3, IM_COL32(255, 255, 255, 255), 0, 2.0f);
                if (hovered && ImGui::IsMouseHoveringRect(ImVec2(fx - rr - 2, fy - rr - 2),
                                                          ImVec2(fx + rr + 2, fy + rr + 2))) {
                    ImGui::SetTooltip("obj %u  type %u (%s)  sub %u", o.id, o.type,
                                      rx_map_object_category(o.type), o.subtype);
                    if (ImGui::IsMouseClicked(0)) {
                        g_mi.sel_obj = (int)i;
                        g_mi.sel_cx = g_mi.sel_cy = -1;
                    }
                }
            }

        // selected cell outline
        if (g_mi.sel_cx >= 0) {
            ImVec2 a(img0.x + g_mi.sel_cx * cs, img0.y + g_mi.sel_cy * cs);
            dl->AddRect(a, ImVec2(a.x + cs, a.y + cs), IM_COL32(255, 214, 107, 255),
                        0, 0, 2.5f);
        }

        ImGuiIO &io = ImGui::GetIO();
        if (hovered) {
            if (io.MouseWheel != 0.0f) {
                float oldz = zoom;
                zoom = std::max(0.05f, std::min(24.0f, zoom * powf(1.25f, io.MouseWheel)));
                g_mi.zoom = zoom;
                ImVec2 m = io.MousePos;
                g_mi.pan.x = m.x - (m.x - img0.x) * (zoom / oldz)
                             - (p0.x + (cav.x - g_mi.px_w * zoom) * 0.5f);
                g_mi.pan.y = m.y - (m.y - img0.y) * (zoom / oldz)
                             - (p0.y + (cav.y - g_mi.px_h * zoom) * 0.5f);
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
                (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && io.MouseDelta.x * io.MouseDelta.x + io.MouseDelta.y * io.MouseDelta.y > 0.5f)) {
                g_mi.pan.x += io.MouseDelta.x;
                g_mi.pan.y += io.MouseDelta.y;
            }
            int cx = (int)floorf((io.MousePos.x - img0.x) / cs);
            int cy = (int)floorf((io.MousePos.y - img0.y) / cs);
            if (cx >= 0 && cy >= 0 && cx < d0 && cy < d1) {
                dl->AddRect(ImVec2(img0.x + cx * cs, img0.y + cy * cs),
                            ImVec2(img0.x + (cx + 1) * cs, img0.y + (cy + 1) * cs),
                            IM_COL32(255, 255, 255, 90));
                if (ImGui::IsMouseClicked(0) && ImGui::GetIO().MouseDownDurationPrev[0] < 0.25f) {
                    // plain click (not the tail of a drag) selects the cell —
                    // unless an object marker consumed it above
                    if (g_mi.sel_obj < 0 || !ImGui::IsMouseClicked(0)) {}
                    g_mi.sel_cx = cx; g_mi.sel_cy = cy; g_mi.sel_obj = -1;
                    mi_build_sel_tex();
                }
            }
        }
        dl->PopClipRect();
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("##miinspect", ImVec2(panel_w - 8, avail.y), true);
    if (g_mi.sel_obj >= 0 && g_mi.sel_obj < (int)g_mi.md.count) {
        mi_inspect_obj_panel();
    } else if (g_mi.sel_cx >= 0) {
        mi_inspect_cell_panel();
    } else {
        ImGui::TextDisabled("click a cell or object marker to inspect");
        ImGui::Spacing();
        ImGui::TextWrapped("Rendering uses the port's own pipeline: map_decode "
                           "(FUN_00587e00 dispatch) -> runtime render grid -> "
                           "map_render_tile (FUN_00490f30 geometry), composited "
                           "with the engine-registered sprite banks.");
        if (!g_mi.unknown_ids.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.95f, 0.5f, 0.4f, 1), "unported tile ids:");
            for (auto &kv : g_mi.unknown_ids)
                ImGui::Text("  0x%x  (%d cells)", kv.first, kv.second);
        }
    }
    ImGui::EndChild();
}

static void panel_strings()
{
    if (!g_strings_ok) { ImGui::TextDisabled("decode failed"); return; }
    ImGui::PushFont(g_font_mono);
    ImGui::InputTextMultiline("##str", g_strings.data(), g_strings.size(),
                              ImGui::GetContentRegionAvail(), ImGuiInputTextFlags_ReadOnly);
    ImGui::PopFont();
}

static void panel_hex()
{
    if (!g_blob || !g_blob_sz) { ImGui::TextDisabled("(no data)"); return; }
    ImGui::PushFont(g_font_mono);
    ImGui::BeginChild("##hex");
    ImGuiListClipper clip;
    clip.Begin((int)((g_blob_sz + 15) / 16));
    while (clip.Step())
        for (int row = clip.DisplayStart; row < clip.DisplayEnd; row++) {
            DWORD off = (DWORD)row * 16;
            char line[128];
            int n = _snprintf(line, sizeof line, "%08lX  ", (unsigned long)off);
            for (int i = 0; i < 16; i++) {
                if (off + i < g_blob_sz)
                    n += _snprintf(line + n, sizeof line - n, "%02X ", g_blob[off + i]);
                else
                    n += _snprintf(line + n, sizeof line - n, "   ");
                if (i == 7) n += _snprintf(line + n, sizeof line - n, " ");
            }
            n += _snprintf(line + n, sizeof line - n, " ");
            for (int i = 0; i < 16 && off + i < g_blob_sz; i++) {
                BYTE b = g_blob[off + i];
                line[n++] = (b >= 0x20 && b < 0x7f) ? (char)b : '.';
            }
            line[n] = 0;
            ImGui::TextUnformatted(line);
        }
    ImGui::EndChild();
    ImGui::PopFont();
}

static void panel_info(const RxEntry* e)
{
    const RxModule* m = &g_world.modules[e->module_idx];
    ImGui::Text("module   %s", m->label);
    char pathu[MAX_PATH * 3];
    rx_utf8(m->path, -1, pathu, sizeof pathu);
    ImGui::TextDisabled("         %s", pathu);
    ImGui::Text("type     %s", e->type_label);
    if (!e->name.isStr) ImGui::Text("id       %u  (0x%x)", e->name.id, e->name.id);
    else                ImGui::Text("name     %s", e->id_label);
    ImGui::Text("kind     %s", rx_kind_name(e->kind));
    ImGui::Text("size     %lu bytes", (unsigned long)e->size);
    ImGui::Text("info     %s", e->info);
    switch (e->kind) {
    case RK_SPRITE:
        ImGui::Separator();
        ImGui::TextWrapped("Lizsoft compressed container: self-rebasing pointer table at "
                           "+0x420, signature 0x2711, embedded 256-color palette at +0x20, "
                           "pixels at +0x458+off. Decoded by the engine's own ported "
                           "bs_decode_resource (FUN_005b7800 / FUN_005b7c10).");
        break;
    case RK_MAP:
        ImGui::Separator();
        ImGui::TextWrapped("MSD_SOTES_MAPDATA: tilemap cells (0x1c B each) + object layers "
                           "(0x3c B header + 4 sub-arrays). Parsed by the ported "
                           "map_data_parse (FUN_00587970). Object types (the ENGINE's own band "
                           "names): 5xxxx EFFECT, 6xxxx STRUCTURE, 7xxxx CHARACTER (NPCs "
                           "AND props/emitter fixtures), 8xxxx DEVICE, 9001x placeholder "
                           "link anchors (consumed by the map decoder, not spawned).");
        break;
    case RK_WMA:
        ImGui::Separator();
        ImGui::TextWrapped("ASF/WMA stream (BGM). The engine stages these to a temp file "
                           "and plays them via DirectShow; this viewer does the same via MCI.");
        break;
    default: break;
    }
    if (g_blob && g_blob_sz) {
        ImGui::Separator();
        ImGui::TextDisabled("first bytes");
        ImGui::PushFont(g_font_mono);
        for (DWORD row = 0; row < 4 && row * 16 < g_blob_sz; row++) {
            char line[80];
            int n = 0;
            for (int i = 0; i < 16 && row * 16 + i < g_blob_sz; i++)
                n += _snprintf(line + n, sizeof line - n, "%02X ", g_blob[row * 16 + i]);
            line[n] = 0;
            ImGui::TextUnformatted(line);
        }
        ImGui::PopFont();
    }
}

// ── export bar ──────────────────────────────────────────────────────────────
static void export_bar(HWND hwnd, const RxEntry* e)
{
    char sug[160];
    wchar_t path[MAX_PATH];
    bool img = (e->kind == RK_SPRITE || e->kind == RK_IMAGE_RAW || e->kind == RK_BMP);

    if (img && g_img_ok) {
        if (ImGui::Button("Export PNG...")) {
            suggest_name(e, "png", sug, sizeof sug);
            if (save_dialog(hwnd, L"PNG\0*.png\0", L"png", sug, path))
                set_status(rx_export_png(&g_img, g_key_on, g_flip_on, path)
                           ? "Exported PNG." : "PNG export failed.");
        }
        if (g_grid_on) {
            ImGui::SameLine();
            if (ImGui::Button("Export frames...")) {
                suggest_name(e, "png", sug, sizeof sug);
                if (save_dialog(hwnd, L"PNG\0*.png\0", L"png", sug, path)) {
                    int n = rx_export_frames(&g_img, g_key_on, g_flip_on,
                                             g_cell_w, g_cell_h, path);
                    set_status("Exported %d frame PNGs.", n);
                }
            }
        }
        ImGui::SameLine();
    }
    if (e->kind == RK_WAV) {
        if (ImGui::Button("Export WAV...")) {
            suggest_name(e, "wav", sug, sizeof sug);
            if (save_dialog(hwnd, L"WAV\0*.wav\0", L"wav", sug, path))
                set_status(rx_export_raw(&g_world, e, path) ? "Exported WAV." : "Export failed.");
        }
        ImGui::SameLine();
    }
    if (e->kind == RK_WMA) {
        if (ImGui::Button("Export WMA...")) {
            suggest_name(e, "wma", sug, sizeof sug);
            if (save_dialog(hwnd, L"WMA\0*.wma\0", L"wma", sug, path))
                set_status(rx_export_raw(&g_world, e, path) ? "Exported WMA." : "Export failed.");
        }
        ImGui::SameLine();
    }
    if (e->kind == RK_MAP) {
        if (ImGui::Button("Export JSON...")) {
            suggest_name(e, "json", sug, sizeof sug);
            if (save_dialog(hwnd, L"JSON\0*.json\0", L"json", sug, path))
                set_status(rx_export_map_json(&g_world, e, path)
                           ? "Exported map JSON." : "Export failed.");
        }
        ImGui::SameLine();
    }
    if (e->kind == RK_STRINGS) {
        if (ImGui::Button("Export TXT...")) {
            suggest_name(e, "txt", sug, sizeof sug);
            if (save_dialog(hwnd, L"Text\0*.txt\0", L"txt", sug, path))
                set_status(rx_export_strings_txt(&g_world, e, path)
                           ? "Exported UTF-8 text." : "Export failed.");
        }
        ImGui::SameLine();
    }
    if (ImGui::Button("Export raw...")) {
        suggest_name(e, "bin", sug, sizeof sug);
        if (save_dialog(hwnd, L"Binary\0*.bin\0All\0*.*\0", L"bin", sug, path))
            set_status(rx_export_raw(&g_world, e, path) ? "Exported raw bytes." : "Export failed.");
    }
}

// ── main UI ─────────────────────────────────────────────────────────────────
static void draw_ui(HWND hwnd)
{
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("##main", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

    // ── toolbar ──
    adopt_load_if_ready();
    if (g_loading.load() != 0) {
        ImGui::BeginDisabled();
        ImGui::Button("Loading game install...");
        ImGui::EndDisabled();
    } else if (ImGui::Button("Load game install")) {
        set_status("Loading the game install in the background...");
        start_install_load();
    }
    ImGui::SameLine();
    if (ImGui::Button("Open DLL...")) ui_open_dll(hwnd);
    ImGui::SameLine(0, 16);
    ImGui::SetNextItemWidth(240);
    if (g_focus_filter) { ImGui::SetKeyboardFocusHere(); g_focus_filter = false; }
    if (ImGui::InputTextWithHint("##filter", "filter (ctrl+F)", g_filter, sizeof g_filter))
        g_view_dirty = true;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(110);
    const char* kinds = "All kinds\0Data\0Sprite\0Image\0Bitmap\0Map\0Audio\0Music\0Strings\0Version\0";
    int kf = g_kind_filter + 1;
    if (ImGui::Combo("##kind", &kf, kinds)) { g_kind_filter = kf - 1; g_view_dirty = true; }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(140);
    char modprev[80] = "All modules";
    if (g_mod_filter >= 0 && g_mod_filter < (int)g_world.modules.size())
        lstrcpynA(modprev, g_world.modules[g_mod_filter].label, sizeof modprev);
    if (ImGui::BeginCombo("##mod", modprev)) {
        if (ImGui::Selectable("All modules", g_mod_filter < 0))
            { g_mod_filter = -1; g_view_dirty = true; }
        for (int i = 0; i < (int)g_world.modules.size(); i++) {
            char lbl[96];
            _snprintf(lbl, sizeof lbl, "%s (%d)", g_world.modules[i].label,
                      g_world.modules[i].n_entries);
            if (ImGui::Selectable(lbl, g_mod_filter == i))
                { g_mod_filter = i; g_view_dirty = true; }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Export all filtered...") && !g_view.empty()) {
        wchar_t dir[MAX_PATH];
        if (pick_folder(hwnd, dir)) {
            int n = rx_export_bulk(&g_world, g_view, dir);
            set_status("Bulk-exported %d/%d resources (+manifest.txt).", n, (int)g_view.size());
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%d / %d resources", (int)g_view.size(), (int)g_world.entries.size());

    if (g_view_dirty) rebuild_view();

    // ── split: table | preview ──
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float table_w = std::max(430.0f, avail.x * 0.38f);
    float bottom_h = 26.0f;

    ImGui::BeginChild("##left", ImVec2(table_w, avail.y - bottom_h), true);
    if (ImGui::BeginTable("##restable", 6,
            ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Module", ImGuiTableColumnFlags_WidthFixed, 86);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 56);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 56);
        ImGui::TableSetupColumn("Kind", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 68);
        ImGui::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        ImGuiTableSortSpecs* ss = ImGui::TableGetSortSpecs();
        if (ss && ss->SpecsDirty) { g_sort = ss; rebuild_view(); ss->SpecsDirty = false; }

        ImGuiListClipper clip;
        clip.Begin((int)g_view.size());
        while (clip.Step())
            for (int r = clip.DisplayStart; r < clip.DisplayEnd; r++) {
                int idx = g_view[r];
                const RxEntry& e = g_world.entries[idx];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::PushID(idx);
                if (ImGui::Selectable(g_world.modules[e.module_idx].label,
                                      g_sel == idx,
                                      ImGuiSelectableFlags_SpanAllColumns))
                    select_entry(idx);
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) &&
                    (e.kind == RK_WAV || e.kind == RK_WMA)) {
                    select_entry(idx);
                    if (g_use_mci) g_mci.play_from(0);
                    else g_pcm.play_from(0);
                }
                ImGui::PopID();
                ImGui::TableNextColumn(); ImGui::TextUnformatted(e.type_label);
                ImGui::TableNextColumn(); ImGui::TextUnformatted(e.id_label);
                ImGui::TableNextColumn();
                ImGui::PushStyleColor(ImGuiCol_Text, kind_color(e.kind));
                ImGui::TextUnformatted(rx_kind_name(e.kind));
                ImGui::PopStyleColor();
                ImGui::TableNextColumn();
                if (e.size >= 1024 * 1024) ImGui::Text("%.1f M", e.size / 1048576.0);
                else if (e.size >= 1024)   ImGui::Text("%.1f K", e.size / 1024.0);
                else                       ImGui::Text("%lu", (unsigned long)e.size);
                ImGui::TableNextColumn(); ImGui::TextUnformatted(e.info);
            }
        ImGui::EndTable();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##right", ImVec2(0, avail.y - bottom_h), true);
    if (g_sel >= 0 && g_sel < (int)g_world.entries.size()) {
        const RxEntry* e = &g_world.entries[g_sel];
        ImGui::PushStyleColor(ImGuiCol_Text, kind_color(e->kind));
        ImGui::Text("%s", rx_kind_name(e->kind));
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text("%s %s %s", g_world.modules[e->module_idx].label,
                    e->type_label, e->id_label);
        ImGui::SameLine();
        ImGui::TextDisabled("- %s", e->info);
        ImGui::Separator();
        if (ImGui::BeginTabBar("##tabs")) {
            // maps default to the Inspector (the engine-true render); the
            // schematic stays available under Preview.
            if (e->kind == RK_MAP && ImGui::BeginTabItem("Inspector")) {
                mi_draw_tab(hwnd);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Preview")) {
                ImVec2 pa = ImGui::GetContentRegionAvail();
                ImGui::BeginChild("##prev", ImVec2(pa.x, pa.y - 34), false);
                switch (e->kind) {
                case RK_SPRITE: case RK_IMAGE_RAW: case RK_BMP: panel_image(hwnd, e); break;
                case RK_WAV: case RK_WMA: panel_audio(e); break;
                case RK_MAP: panel_map(); break;
                case RK_STRINGS: panel_strings(); break;
                default:
                    ImGui::TextDisabled("No native preview for this payload - see Hex.");
                    ImGui::Spacing();
                    panel_hex();
                    break;
                }
                ImGui::EndChild();
                export_bar(hwnd, e);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Hex")) { panel_hex(); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Info")) { panel_info(e); ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        }
    } else {
        ImGui::TextDisabled("Select a resource.");
        ImGui::Spacing();
        ImGui::TextWrapped("Load the game install to browse every sprite sheet, map, "
                           "sound effect, BGM stream and voice clip the engine ships "
                           "- previews decode with the engine's own ported code.");
    }
    ImGui::EndChild();

    // ── status bar ──
    ImGui::TextDisabled("%s", g_status);

    ImGui::End();

    // keyboard: list navigation + audio toggle
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) || ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            int cur = -1;
            for (int i = 0; i < (int)g_view.size(); i++)
                if (g_view[i] == g_sel) { cur = i; break; }
            int next = cur + (ImGui::IsKeyPressed(ImGuiKey_DownArrow) ? 1 : -1);
            if (cur < 0) next = 0;
            if (next >= 0 && next < (int)g_view.size()) select_entry(g_view[next]);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Space) && g_sel >= 0) {
            const RxEntry& e = g_world.entries[g_sel];
            if (e.kind == RK_WAV || e.kind == RK_WMA) {
                if (g_use_mci) g_mci.toggle_pause();
                else g_pcm.toggle_pause();
            }
        }
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F)) g_focus_filter = true;

    g_pcm.update();
}

// ── screenshot (--shot) ─────────────────────────────────────────────────────
static void stbw_file(void* ctx, void* data, int size)
{
    fwrite(data, 1, (size_t)size, (FILE*)ctx);
}

static int capture_backbuffer(const wchar_t* out_png)
{
    ID3D11Texture2D* back = nullptr;
    g_swap->GetBuffer(0, IID_PPV_ARGS(&back));
    if (!back) return 0;
    D3D11_TEXTURE2D_DESC d;
    back->GetDesc(&d);
    d.Usage = D3D11_USAGE_STAGING;
    d.BindFlags = 0;
    d.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    d.MiscFlags = 0;
    ID3D11Texture2D* stage = nullptr;
    if (g_dev->CreateTexture2D(&d, nullptr, &stage) != S_OK) { back->Release(); return 0; }
    g_ctx->CopyResource(stage, back);
    back->Release();
    D3D11_MAPPED_SUBRESOURCE m;
    if (g_ctx->Map(stage, 0, D3D11_MAP_READ, 0, &m) != S_OK) { stage->Release(); return 0; }
    std::vector<uint8_t> rows((size_t)d.Width * d.Height * 4);
    for (UINT y = 0; y < d.Height; y++)
        memcpy(rows.data() + (size_t)y * d.Width * 4,
               (uint8_t*)m.pData + (size_t)y * m.RowPitch, (size_t)d.Width * 4);
    g_ctx->Unmap(stage, 0);
    stage->Release();
    // alpha → opaque (ImGui leaves dst alpha undefined)
    for (size_t i = 3; i < rows.size(); i += 4) rows[i] = 0xff;
    FILE* f = _wfopen(out_png, L"wb");
    if (!f) return 0;
    int ok = stbi_write_png_to_func(stbw_file, f, d.Width, d.Height, 4,
                                    rows.data(), d.Width * 4);
    fclose(f);
    return ok;
}

// pick a good default selection for the screenshot: prefer [MODULE:]TYPE:ID if
// given, else the biggest decodable sprite sheet.
static void shot_select(const wchar_t* spec)
{
    if (spec) {
        char s[96];
        rx_utf8(spec, -1, s, sizeof s);
        char* c1 = strchr(s, ':');
        if (c1) {
            char* c2 = strchr(c1 + 1, ':');
            const char* modq = nullptr;
            char* type = s;
            char* idp = c1 + 1;
            if (c2) { *c2 = 0; modq = s; type = c1 + 1; idp = c2 + 1; }
            *c1 = 0;
            int id = atoi(idp);
            for (int i = 0; i < (int)g_world.entries.size(); i++) {
                const RxEntry& e = g_world.entries[i];
                if (e.name.isStr || e.name.id != id) continue;
                if (lstrcmpiA(e.type_label, type)) continue;
                if (modq && lstrcmpiA(g_world.modules[e.module_idx].label, modq))
                    continue;
                select_entry(i);
                return;
            }
        }
    }
    int best = -1; DWORD best_sz = 0;
    for (int i = 0; i < (int)g_world.entries.size(); i++) {
        const RxEntry& e = g_world.entries[i];
        if (e.kind == RK_SPRITE && e.size > best_sz && e.img_w >= 128) {
            best = i; best_sz = e.size;
        }
    }
    if (best >= 0) select_entry(best);
}

// ── main ────────────────────────────────────────────────────────────────────
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow)
{
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv && argc >= 2) {
        if (!lstrcmpW(argv[1], L"--list") && argc >= 4) return rx_cli_list(argv[2], argv[3]);
        if (!lstrcmpW(argv[1], L"--dump") && argc >= 6)
            return rx_cli_dump(argv[2], argv[3], argv[4], argv[5]);
        if (!lstrcmpW(argv[1], L"--export") && argc >= 6)
            return rx_cli_export(argv[2], argv[3], argv[4], argv[5]);
    }
    bool shot = argv && argc >= 3 && !lstrcmpW(argv[1], L"--shot");
    const wchar_t* shot_png = shot ? argv[2] : nullptr;
    const wchar_t* target = nullptr;
    const wchar_t* shot_spec = nullptr;
    if (shot) {
        if (argc >= 4) target = argv[3];
        if (argc >= 5) shot_spec = argv[4];
    } else if (argv && argc >= 2 && argv[1][0] != L'-') {
        target = argv[1];
    }

    SetProcessDPIAware();
    CoInitialize(nullptr);

    WNDCLASSEXW wc = { sizeof wc, CS_CLASSDC, WndProc, 0, 0, hInst, nullptr,
                       LoadCursorW(nullptr, IDC_ARROW), nullptr, nullptr,
                       L"OSSResExplorer", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"OpenSummoners Resource Explorer",
                              WS_OVERLAPPEDWINDOW, 60, 40, 1480, 900,
                              nullptr, nullptr, hInst, nullptr);
    if (!CreateDeviceD3D(hwnd)) {
        MessageBoxW(hwnd, L"DX11 init failed.", L"res_explorer", MB_ICONERROR);
        return 1;
    }
    ShowWindow(hwnd, shot ? SW_SHOWNOACTIVATE : nShow);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;               // fixed layout, nothing to persist
    apply_theme();
    load_fonts();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);

    if (target) {
        open_target(target);
    } else if (shot) {   // screenshot needs the world before the first frame
        int n = rx_load_game_install(&g_world, nullptr, nullptr);
        set_status("Loaded %d module(s) from the detected game install.", n);
    } else {
        set_status("Loading the game install in the background...");
        start_install_load();
    }
    g_view_dirty = true;
    rebuild_view();
    if (shot) shot_select(shot_spec);

    int shot_frames = 0;
    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        draw_ui(hwnd);
        ImGui::Render();
        const float clear[4] = { 0.06f, 0.06f, 0.07f, 1.0f };
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        if (shot && ++shot_frames == 8) {       // a few frames to settle layout
            int ok = capture_backbuffer(shot_png);
            g_swap->Present(0, 0);
            CleanupDeviceD3D();
            return ok ? 0 : 1;
        }
        g_swap->Present(1, 0);
    }

    if (g_load_thread.joinable()) g_load_thread.join();
    if (g_load_result) { rx_close_all(g_load_result); delete g_load_result; }
    g_pcm.stop();
    g_mci.close();
    mi_clear();
    g_img_tex.release();
    g_map_tex.release();
    if (g_map_ok) map_data_free(&g_map);
    rx_close_all(&g_world);
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, hInst);
    CoUninitialize();
    return 0;
}
