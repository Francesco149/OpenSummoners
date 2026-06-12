// tools/osr_view/osr_view_imgui.cpp — the Trace Studio v2 native frame viewer
// (Dear ImGui + DX11).  A native Windows PE (mingw-cross): reconstructs each .osr
// frame with the real DDraw blits + GDI text (the C osr_scrub engine), uploads it
// to a DX11 texture, and presents it in an ImGui UI — instant scrub now, with the
// draw-call drill-in + per-frame state panels to grow into the same window.
//
// The DX11 device/swapchain boilerplate is the stock Dear ImGui win32+dx11
// example; the app content (frame texture + scrubber) is ours.  The DDraw
// reconstruction (osr_scrub) and the DX11 UI coexist in one process — DDraw uses
// offscreen surfaces (windowed cooperative level), DX11 owns the swapchain.

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#include <d3d11.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "osr_scrub.h"

// ── DX11 globals ────────────────────────────────────────────────────────────
static ID3D11Device*           g_dev = nullptr;
static ID3D11DeviceContext*    g_ctx = nullptr;
static IDXGISwapChain*         g_swap = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;

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
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT flags = 0;
    D3D_FEATURE_LEVEL fl;
    const D3D_FEATURE_LEVEL lvls[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
            lvls, 2, D3D11_SDK_VERSION, &sd, &g_swap, &g_dev, &fl, &g_ctx) != S_OK) {
        // fall back to WARP (software) if no hardware device
        if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags,
                lvls, 2, D3D11_SDK_VERSION, &sd, &g_swap, &g_dev, &fl, &g_ctx) != S_OK)
            return false;
    }
    CreateRenderTarget();
    return true;
}
static void CleanupDeviceD3D()
{
    CleanupRenderTarget();
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

// ── the frame texture (updated from osr_scrub) ──────────────────────────────
static ID3D11Texture2D*          g_tex = nullptr;
static ID3D11ShaderResourceView* g_srv = nullptr;
static int g_tw = 0, g_th = 0;

static bool create_frame_texture(int w, int h)
{
    D3D11_TEXTURE2D_DESC d;
    ZeroMemory(&d, sizeof d);
    d.Width = w; d.Height = h; d.MipLevels = 1; d.ArraySize = 1;
    d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    d.SampleDesc.Count = 1;
    d.Usage = D3D11_USAGE_DYNAMIC;
    d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    d.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (g_dev->CreateTexture2D(&d, nullptr, &g_tex) != S_OK) return false;
    if (g_dev->CreateShaderResourceView(g_tex, nullptr, &g_srv) != S_OK) return false;
    g_tw = w; g_th = h;
    return true;
}

static void upload_frame(const uint32_t* rgba, int w, int h)
{
    D3D11_MAPPED_SUBRESOURCE m;
    if (g_ctx->Map(g_tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &m) != S_OK) return;
    for (int y = 0; y < h; y++)
        memcpy((uint8_t*)m.pData + (size_t)y * m.RowPitch,
               rgba + (size_t)y * w, (size_t)w * 4);
    g_ctx->Unmap(g_tex, 0);
}

int main(int argc, char** argv)
{
    const char* path = (argc > 1) ? argv[1] : "C:\\oss-osr\\retail.osr";

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr),
                       nullptr, nullptr, nullptr, nullptr, L"osr_view", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"osr_view — Trace Studio v2",
                              WS_OVERLAPPEDWINDOW, 80, 80, 1100, 720,
                              nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        MessageBoxW(nullptr, L"DX11 device creation failed", L"osr_view", MB_OK);
        return 1;
    }
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);

    // open the scrub session (DDraw reconstruction) on our window
    osr_scrub* sc = osr_scrub_open((void*)hwnd, path);
    int nframes = sc ? osr_scrub_frame_count(sc) : 0;
    int w = sc ? osr_scrub_width(sc)  : 640;
    int h = sc ? osr_scrub_height(sc) : 480;
    uint32_t* framebuf = (uint32_t*)malloc((size_t)w * h * 4);
    if (sc) create_frame_texture(w, h);

    int cur = 0, shown = -1;
    bool playing = false;

    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0u, 0u, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // keyboard scrub
        if (nframes > 0) {
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) cur++;
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))  cur--;
            if (ImGui::IsKeyPressed(ImGuiKey_PageDown))   cur += 30;
            if (ImGui::IsKeyPressed(ImGuiKey_PageUp))     cur -= 30;
            if (ImGui::IsKeyPressed(ImGuiKey_Home))       cur = 0;
            if (ImGui::IsKeyPressed(ImGuiKey_End))        cur = nframes - 1;
            if (ImGui::IsKeyPressed(ImGuiKey_Space))      playing = !playing;
            if (playing) { cur++; if (cur >= nframes) { cur = nframes - 1; playing = false; } }
        }

        // ── the frame view + scrubber ───────────────────────────────────────
        ImGui::SetNextWindowPos(ImVec2(8, 8), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(w + 32, h + 130), ImGuiCond_FirstUseEver);
        ImGui::Begin("frame");
        if (sc) {
            if (cur < 0) cur = 0;
            if (cur >= nframes) cur = nframes - 1;
            uint32_t flip = 0, tick = 0;
            osr_scrub_frame_info(sc, cur, &flip, &tick);
            ImGui::Text("frame %d / %d    flip=%u   tick=%u   %s",
                        cur, nframes - 1, flip, tick, playing ? "[PLAY]" : "");
            int slider = cur;
            ImGui::SetNextItemWidth((float)w);
            if (ImGui::SliderInt("##scrub", &slider, 0, nframes - 1)) cur = slider;

            if (cur != shown) {
                if (osr_scrub_render_rgba(sc, cur, framebuf))
                    upload_frame(framebuf, w, h);
                shown = cur;
            }
            if (g_srv)
                ImGui::Image((ImTextureID)g_srv, ImVec2((float)w, (float)h));
        } else {
            ImGui::TextColored(ImVec4(1,0.4f,0.4f,1), "failed to open %s", path);
        }
        ImGui::End();

        // (future) draw-call list + per-frame state panels go here.

        ImGui::Render();
        const float clear[4] = { 0.08f, 0.08f, 0.10f, 1.0f };
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swap->Present(1, 0);
    }

    if (sc) osr_scrub_close(sc);
    free(framebuf);
    if (g_srv) g_srv->Release();
    if (g_tex) g_tex->Release();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
