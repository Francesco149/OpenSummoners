// tools/osr_view/osr_view_imgui.cpp — the Trace Studio v2 native frame viewer
// (Dear ImGui + DX11).  A native Windows PE (mingw-cross) that reconstructs each
// .osr frame with the real DDraw blits + GDI text (the C osr_scrub engine), uploads
// it to a DX11 texture, and presents it in an ImGui UI.
//
//   osr_view.exe <file.osr>                 single-panel scrub (one side)
//   osr_view.exe <port.osr> <retail.osr>    M6 tick-joined port|retail|diff studio
//
// DUAL MODE (M6): open two scrub sessions, JOIN their frames by the deterministic
// sim_tick (group each side by tick, take the last flip per tick = the presented
// state; the union timeline keeps honest port-only/retail-only gaps), and show
// port | retail | diff side-by-side with a precomputed per-pair diff heat ribbon
// (click → seek; "worst" jumps to the max-divergence pair).  The tick axis is the
// parity axis (quirk #99 / ckpt 105) — never the flip axis.
//
// The DX11 device/swapchain boilerplate is the stock Dear ImGui win32+dx11 example;
// the app content (frame textures + join + ribbon) is ours.

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#include <d3d11.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <vector>

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

// ── a frame texture (one per panel) ─────────────────────────────────────────
struct Panel {
    ID3D11Texture2D*          tex = nullptr;
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

// ── the tick-join (M6) ──────────────────────────────────────────────────────
struct JoinEntry {
    uint32_t tick;
    int  pidx = -1, ridx = -1;   // per-side frame index, or -1 = honest gap
    int  differ = -1, maxd = 0;  // diff metric (differ<0 = not computed yet)
    bool kind_paired() const { return pidx >= 0 && ridx >= 0; }
};

// group a side's frames by sim_tick → last frame index per tick (the presented
// state; retail coalesces/re-presents, quirk #99).  frames come in flip order so
// last-write-wins is the last flip at that tick.
static std::map<uint32_t,int> tick_index(osr_scrub* s)
{
    std::map<uint32_t,int> m;
    int n = osr_scrub_frame_count(s);
    for (int i = 0; i < n; i++) {
        uint32_t flip = 0, tick = 0;
        osr_scrub_frame_info(s, i, &flip, &tick);
        m[tick] = i;
    }
    return m;
}

static std::vector<JoinEntry> build_join(osr_scrub* port, osr_scrub* retail)
{
    std::map<uint32_t,int> p = tick_index(port), r = tick_index(retail);
    std::set<uint32_t> ticks;
    for (auto& kv : p) ticks.insert(kv.first);
    for (auto& kv : r) ticks.insert(kv.first);
    std::vector<JoinEntry> tl;
    tl.reserve(ticks.size());
    for (uint32_t t : ticks) {
        JoinEntry e; e.tick = t;
        auto pi = p.find(t); if (pi != p.end()) e.pidx = pi->second;
        auto ri = r.find(t); if (ri != r.end()) e.ridx = ri->second;
        tl.push_back(e);
    }
    return tl;
}

// per-pixel diff of two RGBA8 buffers → an amplified diff image + (differ_px,maxd).
// equal pixels render as a faint port silhouette; divergences ramp yellow→red by
// magnitude so they pop.  Ignores alpha (0xff both sides).
static void diff_image(const uint32_t* a, const uint32_t* b, uint32_t* out,
                       int w, int h, int* differ_px, int* maxd_out)
{
    int differ = 0, maxd = 0;
    int n = w * h;
    for (int i = 0; i < n; i++) {
        uint32_t pa = a[i], pb = b[i];
        int ar = pa & 0xff, ag = (pa >> 8) & 0xff, ab = (pa >> 16) & 0xff;
        int br = pb & 0xff, bg = (pb >> 8) & 0xff, bb = (pb >> 16) & 0xff;
        int dr = ar - br, dg = ag - bg, db = ab - bb;
        if (dr < 0) dr = -dr;
        if (dg < 0) dg = -dg;
        if (db < 0) db = -db;
        int d = dr;
        if (dg > d) d = dg;
        if (db > d) d = db;
        if (d) {
            differ++;
            if (d > maxd) maxd = d;
            int g = 255 - d; if (g < 0) g = 0;            // small=yellow, large=red
            out[i] = 0xff000000u | (0u << 16) | ((uint32_t)g << 8) | 255u;
        } else {
            int lum = (ar * 30 + ag * 59 + ab * 11) / 100;
            uint32_t s = (uint32_t)(lum * 40 / 255);      // faint silhouette
            out[i] = 0xff000000u | (s << 16) | (s << 8) | s;
        }
    }
    if (differ_px) *differ_px = differ;
    if (maxd_out)  *maxd_out  = maxd;
}

// heat colour for a paired entry's differ_px (log-scaled: green→yellow→red).
static ImU32 heat_col(int differ, int total_px)
{
    if (differ <= 0) return IM_COL32(40, 170, 60, 255);   // green = pixel-exact
    double t = log(1.0 + differ) / log(1.0 + (double)total_px);
    if (t > 1.0) t = 1.0;
    int r, g;
    if (t < 0.5) { r = (int)(255 * (t / 0.5)); g = 200; }  // green→yellow
    else         { r = 255; g = (int)(200 * (1.0 - (t - 0.5) / 0.5)); } // yellow→red
    return IM_COL32(r, g, 30, 255);
}

static void render_panel_image(Panel& pn, int w, int h, float scale, const char* label)
{
    ImGui::BeginGroup();
    ImGui::TextUnformatted(label);
    if (pn.srv)
        ImGui::Image((ImTextureID)pn.srv, ImVec2(w * scale, h * scale));
    else
        ImGui::Dummy(ImVec2(w * scale, h * scale));
    ImGui::EndGroup();
}

// ── single-panel mode (the original, unchanged behaviour) ───────────────────
static int run_single(HWND hwnd, const char* path)
{
    osr_scrub* sc = osr_scrub_open((void*)hwnd, path);
    int nframes = sc ? osr_scrub_frame_count(sc) : 0;
    int w = sc ? osr_scrub_width(sc)  : 640;
    int h = sc ? osr_scrub_height(sc) : 480;
    uint32_t* framebuf = (uint32_t*)malloc((size_t)w * h * 4);
    Panel pn;
    if (sc) pn.ensure(w, h);

    int cur = 0, shown = -1;
    bool playing = false, done = false;
    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0u, 0u, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
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
                if (osr_scrub_render_rgba(sc, cur, framebuf)) pn.upload(framebuf);
                shown = cur;
            }
            if (pn.srv) ImGui::Image((ImTextureID)pn.srv, ImVec2((float)w, (float)h));
        } else {
            ImGui::TextColored(ImVec4(1,0.4f,0.4f,1), "failed to open %s", path);
        }
        ImGui::End();

        ImGui::Render();
        const float clear[4] = { 0.08f, 0.08f, 0.10f, 1.0f };
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swap->Present(1, 0);
    }
    if (sc) osr_scrub_close(sc);
    pn.release();
    free(framebuf);
    return 0;
}

// ── dual port|retail|diff mode (M6) ─────────────────────────────────────────
static int run_dual(HWND hwnd, const char* port_path, const char* retail_path)
{
    osr_scrub* port   = osr_scrub_open((void*)hwnd, port_path);
    osr_scrub* retail = osr_scrub_open((void*)hwnd, retail_path);
    if (!port || !retail) {
        bool done = false;
        while (!done) {
            MSG msg;
            while (PeekMessage(&msg, nullptr, 0u, 0u, PM_REMOVE)) {
                TranslateMessage(&msg); DispatchMessage(&msg);
                if (msg.message == WM_QUIT) done = true;
            }
            if (done) break;
            ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
            ImGui::Begin("error");
            ImGui::TextColored(ImVec4(1,0.4f,0.4f,1), "failed to open %s",
                               port ? retail_path : port_path);
            ImGui::End();
            ImGui::Render();
            const float clear[4] = { 0.08f, 0.08f, 0.10f, 1.0f };
            g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
            g_ctx->ClearRenderTargetView(g_rtv, clear);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            g_swap->Present(1, 0);
        }
        if (port) osr_scrub_close(port);
        if (retail) osr_scrub_close(retail);
        return 1;
    }

    int w = osr_scrub_width(port), h = osr_scrub_height(port);
    int total_px = w * h;
    std::vector<JoinEntry> tl = build_join(port, retail);
    int n = (int)tl.size();

    int n_paired = 0, n_ponly = 0, n_ronly = 0;
    for (auto& e : tl) {
        if (e.kind_paired()) n_paired++;
        else if (e.pidx >= 0) n_ponly++;
        else n_ronly++;
    }

    Panel pPort, pRetail, pDiff;
    pPort.ensure(w, h); pRetail.ensure(w, h); pDiff.ensure(w, h);
    uint32_t* bport = (uint32_t*)malloc((size_t)total_px * 4);
    uint32_t* bret  = (uint32_t*)malloc((size_t)total_px * 4);
    uint32_t* bdiff = (uint32_t*)malloc((size_t)total_px * 4);
    uint32_t* ma    = (uint32_t*)malloc((size_t)total_px * 4);   // metric scratch
    uint32_t* mb    = (uint32_t*)malloc((size_t)total_px * 4);
    uint32_t* mtmp  = (uint32_t*)malloc((size_t)total_px * 4);

    // start the scrubber on the first paired tick (skip leading gaps)
    int cur = 0;
    for (int i = 0; i < n; i++) { if (tl[i].kind_paired()) { cur = i; break; } }
    int shown = -1;
    bool playing = false;
    int metric_cursor = 0;           // background precompute progress over tl[]

    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0u, 0u, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();

        // ── background diff precompute: a few paired entries per UI frame ────
        bool precomputing = (metric_cursor < n);
        for (int k = 0; k < 12 && metric_cursor < n; ) {
            JoinEntry& e = tl[metric_cursor];
            if (e.kind_paired() && e.differ < 0) {
                if (osr_scrub_render_rgba(port,   e.pidx, ma) &&
                    osr_scrub_render_rgba(retail, e.ridx, mb)) {
                    diff_image(ma, mb, mtmp, w, h, &e.differ, &e.maxd);
                } else { e.differ = 0; }
                k++;                 // only count real renders toward the budget
            } else if (!e.kind_paired()) {
                e.differ = 0;        // gaps have no metric
            }
            metric_cursor++;
        }

        // keyboard scrub (timeline index)
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) cur++;
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))  cur--;
        if (ImGui::IsKeyPressed(ImGuiKey_PageDown))   cur += 30;
        if (ImGui::IsKeyPressed(ImGuiKey_PageUp))     cur -= 30;
        if (ImGui::IsKeyPressed(ImGuiKey_Home))       cur = 0;
        if (ImGui::IsKeyPressed(ImGuiKey_End))        cur = n - 1;
        if (ImGui::IsKeyPressed(ImGuiKey_Space))      playing = !playing;
        if (playing) { cur++; if (cur >= n) { cur = n - 1; playing = false; } }
        if (cur < 0) cur = 0;
        if (cur >= n) cur = n - 1;

        ImGui::SetNextWindowPos(ImVec2(8, 8), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(3 * w + 60, h + 230), ImGuiCond_FirstUseEver);
        ImGui::Begin("Trace Studio v2 — port | retail | diff  (tick-joined)");

        JoinEntry& e = tl[cur];
        ImGui::Text("timeline %d / %d   sim_tick=%u   %s",
                    cur, n - 1, e.tick,
                    e.kind_paired() ? "PAIRED" : (e.pidx >= 0 ? "PORT-ONLY (retail coalesced)"
                                                              : "RETAIL-ONLY (port has no frame)"));
        ImGui::SameLine();
        if (e.kind_paired() && e.differ >= 0)
            ImGui::Text("   differ_px=%d (%.2f%%)  maxd=%d",
                        e.differ, 100.0 * e.differ / total_px, e.maxd);

        // metadata line
        {
            uint32_t pf = 0, pt = 0, rf = 0, rt = 0;
            if (e.pidx >= 0) osr_scrub_frame_info(port,   e.pidx, &pf, &pt);
            if (e.ridx >= 0) osr_scrub_frame_info(retail, e.ridx, &rf, &rt);
            ImGui::Text("port: %s flip=%u (frame #%d)     retail: %s flip=%u (frame #%d)",
                        e.pidx >= 0 ? "" : "[gap]", pf, e.pidx,
                        e.ridx >= 0 ? "" : "[gap]", rf, e.ridx);
        }

        // slider + nav buttons
        int slider = cur;
        ImGui::SetNextItemWidth((float)(3 * w));
        if (ImGui::SliderInt("##scrub", &slider, 0, n - 1)) cur = slider;
        if (ImGui::Button("|< first paired")) {
            for (int i = 0; i < n; i++) if (tl[i].kind_paired()) { cur = i; break; }
        }
        ImGui::SameLine();
        if (ImGui::Button("worst diff >|")) {
            int best = -1, bi = cur;
            for (int i = 0; i < n; i++)
                if (tl[i].kind_paired() && tl[i].differ > best) { best = tl[i].differ; bi = i; }
            cur = bi;
        }
        ImGui::SameLine();
        if (ImGui::Button("next diff >")) {
            for (int i = cur + 1; i < n; i++)
                if (tl[i].kind_paired() && tl[i].differ > 0) { cur = i; break; }
        }
        ImGui::SameLine();
        ImGui::Text("   joined: %d paired, %d port-only, %d retail-only%s",
                    n_paired, n_ponly, n_ronly,
                    precomputing ? "   [computing diffs…]" : "");

        // ── the diff heat ribbon (aggregate worst-per-column; click to seek) ─
        {
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            float W = ImGui::GetContentRegionAvail().x;
            if (W < 16) W = 16;
            float H = 26;
            ImGui::InvisibleButton("##ribbon", ImVec2(W, H));
            bool hov = ImGui::IsItemHovered();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            int Wi = (int)W;
            for (int px = 0; px < Wi; px++) {
                int i0 = (int)((double)px      / W * n);
                int i1 = (int)((double)(px + 1) / W * n);
                if (i1 <= i0) i1 = i0 + 1;
                if (i1 > n) i1 = n;
                int worst = -1; bool anyGap = false, anyUndone = false, anyPaired = false;
                for (int i = i0; i < i1; i++) {
                    if (tl[i].kind_paired()) {
                        anyPaired = true;
                        if (tl[i].differ < 0) anyUndone = true;
                        else if (tl[i].differ > worst) worst = tl[i].differ;
                    } else anyGap = true;
                }
                ImU32 col;
                if (anyPaired && worst >= 0)      col = heat_col(worst, total_px);
                else if (anyPaired && anyUndone)  col = IM_COL32(70, 70, 80, 255);   // computing
                else if (anyGap)                  col = IM_COL32(45, 60, 95, 255);   // honest gap
                else                              col = IM_COL32(70, 70, 80, 255);
                dl->AddRectFilled(ImVec2(p0.x + px, p0.y), ImVec2(p0.x + px + 1, p0.y + H), col);
            }
            // playhead marker
            float mx = p0.x + (float)cur / (n > 1 ? n - 1 : 1) * (W - 1);
            dl->AddLine(ImVec2(mx, p0.y - 1), ImVec2(mx, p0.y + H + 1), IM_COL32(255,255,255,255), 1.5f);
            if (hov && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                float rel = (ImGui::GetMousePos().x - p0.x) / W;
                if (rel < 0) rel = 0;
                if (rel > 1) rel = 1;
                cur = (int)(rel * n);
                if (cur >= n) cur = n - 1;
            }
        }

        // ── render the three panels at the joined tick ──────────────────────
        if (cur != shown || precomputing) {
            if (e.pidx >= 0 && osr_scrub_render_rgba(port, e.pidx, bport)) pPort.upload(bport);
            else { memset(bport, 0, (size_t)total_px * 4); pPort.upload(bport); }
            if (e.ridx >= 0 && osr_scrub_render_rgba(retail, e.ridx, bret)) pRetail.upload(bret);
            else { memset(bret, 0, (size_t)total_px * 4); pRetail.upload(bret); }
            if (e.kind_paired()) {
                int dpx = 0, md = 0;
                diff_image(bport, bret, bdiff, w, h, &dpx, &md);
                if (e.differ < 0) { e.differ = dpx; e.maxd = md; }
            } else {
                memset(bdiff, 0, (size_t)total_px * 4);
            }
            pDiff.upload(bdiff);
            shown = cur;
        }

        float avail = ImGui::GetContentRegionAvail().x;
        float scale = (avail - 24) / (3.0f * w);
        if (scale > 1.0f) scale = 1.0f;
        if (scale < 0.1f) scale = 0.1f;
        render_panel_image(pPort,   w, h, scale, "PORT");
        ImGui::SameLine();
        render_panel_image(pRetail, w, h, scale, "RETAIL");
        ImGui::SameLine();
        render_panel_image(pDiff,   w, h, scale, "DIFF (yellow→red = divergence)");

        ImGui::End();

        ImGui::Render();
        const float clear[4] = { 0.08f, 0.08f, 0.10f, 1.0f };
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swap->Present(1, 0);
    }

    osr_scrub_close(port); osr_scrub_close(retail);
    pPort.release(); pRetail.release(); pDiff.release();
    free(bport); free(bret); free(bdiff); free(ma); free(mb); free(mtmp);
    return 0;
}

int main(int argc, char** argv)
{
    bool dual = (argc > 2);
    const char* path   = (argc > 1) ? argv[1] : "C:\\oss-osr\\retail.osr";
    const char* path2  = (argc > 2) ? argv[2] : nullptr;

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr),
                       nullptr, nullptr, nullptr, nullptr, L"osr_view", nullptr };
    RegisterClassExW(&wc);
    int win_w = dual ? 1960 : 1100;
    int win_h = dual ? 820  : 720;
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"osr_view — Trace Studio v2",
                              WS_OVERLAPPEDWINDOW, 60, 60, win_w, win_h,
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

    int rc = dual ? run_dual(hwnd, path, path2) : run_single(hwnd, path);

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return rc;
}
