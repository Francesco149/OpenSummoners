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

// ── the NOTE / mark system (M7 — the human→agent hand-off contract) ─────────
// The USER drags a crop region on a panel + types a note → it persists to
// osr_notes.jsonl beside the .osr, keyed by the joined sim_tick + a crop rect in
// frame coords.  The agent reads that JSONL (tools/trace_studio2/notes.py renders
// the cropped port|retail|diff at each noted tick) — so a mark says exactly "look
// HERE at THIS tick".
struct Note {
    uint32_t tick;
    int port_flip, retail_flip;
    int crop[4];          // x, y, w, h in frame pixels (all 0 = whole frame)
    int differ;
    char text[256];
};

static void json_escape(const char* in, char* out, size_t cap)
{
    size_t j = 0;
    for (size_t i = 0; in[i] && j + 2 < cap; i++) {
        char c = in[i];
        if (c == '"' || c == '\\') { out[j++] = '\\'; out[j++] = c; }
        else if (c == '\n')        { out[j++] = '\\'; out[j++] = 'n'; }
        else if ((unsigned char)c >= 0x20) out[j++] = c;
    }
    out[j] = 0;
}

static void note_write_line(FILE* f, const Note& nt)
{
    char esc[512]; json_escape(nt.text, esc, sizeof esc);
    fprintf(f, "{\"tick\":%u,\"port_flip\":%d,\"retail_flip\":%d,"
               "\"crop\":[%d,%d,%d,%d],\"differ\":%d,\"text\":\"%s\"}\n",
            nt.tick, nt.port_flip, nt.retail_flip,
            nt.crop[0], nt.crop[1], nt.crop[2], nt.crop[3], nt.differ, esc);
}

static void notes_append(const char* path, const Note& nt)
{
    FILE* f = fopen(path, "a");
    if (!f) return;
    note_write_line(f, nt);
    fclose(f);
}

static void notes_rewrite(const char* path, const std::vector<Note>& v)
{
    FILE* f = fopen(path, "w");
    if (!f) return;
    for (const Note& nt : v) note_write_line(f, nt);
    fclose(f);
}

static bool note_scan_int(const char* s, const char* key, int* out)
{
    const char* p = strstr(s, key);
    if (!p) return false;
    return sscanf(p + strlen(key), "%d", out) == 1;
}

static void notes_load(const char* path, std::vector<Note>& v)
{
    FILE* f = fopen(path, "r");
    if (!f) return;
    char line[2048];
    while (fgets(line, sizeof line, f)) {
        Note nt; memset(&nt, 0, sizeof nt);
        int tk;
        if (!note_scan_int(line, "\"tick\":", &tk)) continue;
        nt.tick = (uint32_t)tk;
        note_scan_int(line, "\"port_flip\":", &nt.port_flip);
        note_scan_int(line, "\"retail_flip\":", &nt.retail_flip);
        note_scan_int(line, "\"differ\":", &nt.differ);
        const char* cp = strstr(line, "\"crop\":[");
        if (cp) sscanf(cp + 8, "%d,%d,%d,%d", &nt.crop[0], &nt.crop[1], &nt.crop[2], &nt.crop[3]);
        const char* tp = strstr(line, "\"text\":\"");
        if (tp) {
            tp += 8;
            size_t k = 0;
            while (*tp && k + 1 < sizeof nt.text) {
                if (*tp == '\\' && tp[1]) { tp++; nt.text[k++] = (*tp == 'n') ? '\n' : *tp; tp++; }
                else if (*tp == '"') break;
                else nt.text[k++] = *tp++;
            }
            nt.text[k] = 0;
        }
        v.push_back(nt);
    }
    fclose(f);
}

static void derive_notes_path(const char* port_path, char* out, size_t cap)
{
    const char* bs = strrchr(port_path, '\\');
    const char* fs = strrchr(port_path, '/');
    const char* slash = (fs > bs) ? fs : bs;
    if (slash) {
        size_t n = (size_t)(slash - port_path) + 1;
        if (n >= cap) n = cap - 1;
        memcpy(out, port_path, n);
        out[n] = 0;
        strncat(out, "osr_notes.jsonl", cap - strlen(out) - 1);
    } else {
        snprintf(out, cap, "osr_notes.jsonl");
    }
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
    uint32_t* binsp = (uint32_t*)malloc((size_t)total_px * 4);   // draw-inspector

    // start the scrubber on the first paired tick (skip leading gaps)
    int cur = 0;
    for (int i = 0; i < n; i++) { if (tl[i].kind_paired()) { cur = i; break; } }
    int shown = -1;
    bool playing = false;
    int metric_cursor = 0;           // background precompute progress over tl[]

    // note/mark state (the human→agent hand-off)
    std::vector<Note> notes;
    char notes_path[512];
    derive_notes_path(port_path, notes_path, sizeof notes_path);
    notes_load(notes_path, notes);
    char note_buf[256] = {0};
    bool   crop_valid = false, crop_dragging = false;
    ImVec2 crop_a(0,0), crop_b(0,0);

    // draw-inspector state (M7 N3 — render-up-to-K / draw list / pixel→draw pick)
    Panel  pInsp;
    int    insp_side = 0;            // 0 = port, 1 = retail
    int    insp_k = -1;             // render up to K draws (-1 = all)
    int    insp_sel = -1;           // selected draw index (highlighted)
    std::vector<osr_draw_info> insp_draws;
    int    insp_list_idx = -2, insp_list_side = -2;            // what insp_draws holds
    int    insp_shown_k = -99, insp_shown_idx = -2, insp_shown_side = -2;  // texture state

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

        // keyboard scrub (timeline index) — suppressed while typing a note so
        // arrows/space edit the text instead of scrubbing the frame
        if (!ImGui::GetIO().WantCaptureKeyboard) {
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) cur++;
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))  cur--;
            if (ImGui::IsKeyPressed(ImGuiKey_PageDown))   cur += 30;
            if (ImGui::IsKeyPressed(ImGuiKey_PageUp))     cur -= 30;
            if (ImGui::IsKeyPressed(ImGuiKey_Home))       cur = 0;
            if (ImGui::IsKeyPressed(ImGuiKey_End))        cur = n - 1;
            if (ImGui::IsKeyPressed(ImGuiKey_Space))      playing = !playing;
        }
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

        auto draw_panel = [&](Panel& pn, const char* label, bool has_frame) {
            ImGui::BeginGroup();
            ImGui::TextUnformatted(label);
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            ImVec2 sz(w * scale, h * scale);
            // an InvisibleButton OWNS the drag → dragging a crop no longer moves the
            // ImGui window (the bug).  The image is drawn into the same rect.
            ImGui::InvisibleButton(label, sz);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 p1(p0.x + sz.x, p0.y + sz.y);
            if (pn.srv) dl->AddImage((ImTextureID)pn.srv, p0, p1);
            else        dl->AddRectFilled(p0, p1, IM_COL32(15, 15, 18, 255));
            if (!has_frame) {                          // honest gap → label, not black
                const char* msg = "- no frame at this tick (gap) -";
                ImVec2 ts = ImGui::CalcTextSize(msg);
                dl->AddText(ImVec2((p0.x + p1.x - ts.x) * 0.5f, (p0.y + p1.y - ts.y) * 0.5f),
                            IM_COL32(220, 190, 90, 255), msg);
            }
            // crop drag, mapped from THIS panel's rect; the activated button stays the
            // active item for the whole drag, even if the mouse moves over a sibling.
            ImVec2 m = ImGui::GetMousePos();
            float fx = (m.x - p0.x) / scale, fy = (m.y - p0.y) / scale;
            fx = fx < 0 ? 0 : (fx > w ? w : fx);
            fy = fy < 0 ? 0 : (fy > h ? h : fy);
            if (ImGui::IsItemActivated())   { crop_a = crop_b = ImVec2(fx, fy); crop_dragging = true; }
            if (ImGui::IsItemActive())      { crop_b = ImVec2(fx, fy); }
            if (ImGui::IsItemDeactivated()) {
                crop_dragging = false;
                crop_valid = (fabsf(crop_a.x - crop_b.x) >= 3 && fabsf(crop_a.y - crop_b.y) >= 3);
            }
            if (crop_valid || crop_dragging) {         // overlay the crop on every panel
                float x0 = crop_a.x < crop_b.x ? crop_a.x : crop_b.x;
                float y0 = crop_a.y < crop_b.y ? crop_a.y : crop_b.y;
                float x1 = crop_a.x > crop_b.x ? crop_a.x : crop_b.x;
                float y1 = crop_a.y > crop_b.y ? crop_a.y : crop_b.y;
                dl->AddRect(ImVec2(p0.x + x0 * scale, p0.y + y0 * scale),
                            ImVec2(p0.x + x1 * scale, p0.y + y1 * scale),
                            IM_COL32(0, 230, 230, 255), 0.0f, 0, 1.5f);
            }
            ImGui::EndGroup();
        };
        draw_panel(pPort,   "PORT",   e.pidx >= 0);
        ImGui::SameLine();
        draw_panel(pRetail, "RETAIL", e.ridx >= 0);
        ImGui::SameLine();
        draw_panel(pDiff,   "DIFF (yellow->red = divergence)", e.kind_paired());

        // ── notes (the human→agent hand-off: drag a region, type, Add) ──────
        ImGui::SeparatorText("NOTES — drag a region on a panel, type, Add  →  osr_notes.jsonl (the agent reads it)");
        int cx = 0, cy = 0, cw = 0, ch = 0;
        if (crop_valid) {
            cx = (int)(crop_a.x < crop_b.x ? crop_a.x : crop_b.x);
            cy = (int)(crop_a.y < crop_b.y ? crop_a.y : crop_b.y);
            cw = (int)fabsf(crop_a.x - crop_b.x);
            ch = (int)fabsf(crop_a.y - crop_b.y);
            ImGui::Text("crop: (%d,%d) %dx%d", cx, cy, cw, ch);
        } else {
            ImGui::TextDisabled("crop: (none — drag on a panel; no crop = whole frame)");
        }
        ImGui::SameLine();
        if (ImGui::Button("clear crop")) crop_valid = false;
        ImGui::SetNextItemWidth(560);
        bool enter = ImGui::InputText("##notetext", note_buf, sizeof note_buf,
                                      ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        if ((ImGui::Button("Add note") || enter) && note_buf[0]) {
            Note nt; memset(&nt, 0, sizeof nt);
            nt.tick = e.tick;
            uint32_t pf = 0, pt = 0, rf = 0, rt = 0;
            if (e.pidx >= 0) osr_scrub_frame_info(port,   e.pidx, &pf, &pt);
            if (e.ridx >= 0) osr_scrub_frame_info(retail, e.ridx, &rf, &rt);
            nt.port_flip = (int)pf; nt.retail_flip = (int)rf;
            nt.crop[0] = cx; nt.crop[1] = cy; nt.crop[2] = cw; nt.crop[3] = ch;
            nt.differ = e.kind_paired() ? e.differ : -1;
            snprintf(nt.text, sizeof nt.text, "%s", note_buf);
            notes.push_back(nt);
            notes_append(notes_path, nt);
            note_buf[0] = 0; crop_valid = false;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(%d notes)", (int)notes.size());

        ImGui::BeginChild("##noteslist", ImVec2(0, 92), ImGuiChildFlags_Borders);
        for (int i = 0; i < (int)notes.size(); i++) {
            Note& nt = notes[i];
            ImGui::PushID(i);
            if (ImGui::SmallButton("go")) {
                for (int j = 0; j < n; j++) if (tl[j].tick == nt.tick) { cur = j; break; }
            }
            ImGui::SameLine();
            bool del = ImGui::SmallButton("x");
            ImGui::SameLine();
            if (nt.crop[2] || nt.crop[3])
                ImGui::Text("[t%u] %s  {crop %d,%d %dx%d}", nt.tick, nt.text,
                            nt.crop[0], nt.crop[1], nt.crop[2], nt.crop[3]);
            else
                ImGui::Text("[t%u] %s", nt.tick, nt.text);
            ImGui::PopID();
            if (del) { notes.erase(notes.begin() + i); notes_rewrite(notes_path, notes); break; }
        }
        ImGui::EndChild();

        ImGui::End();

        // ── the draw inspector (M7 N3 — separate window) ────────────────────
        ImGui::SetNextWindowPos(ImVec2(60, 560), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(720, 660), ImGuiCond_FirstUseEver);
        ImGui::Begin("draw inspector");
        {
            osr_scrub* isc = (insp_side == 0) ? port : retail;
            int iframe = (insp_side == 0) ? e.pidx : e.ridx;
            ImGui::RadioButton("PORT", &insp_side, 0); ImGui::SameLine();
            ImGui::RadioButton("RETAIL", &insp_side, 1);
            ImGui::SameLine();
            ImGui::TextDisabled("(click the image to pick the draw at a pixel)");

            if (iframe < 0) {
                ImGui::Text("no %s frame at tick %u (honest gap)",
                            insp_side == 0 ? "port" : "retail", e.tick);
            } else {
                if (insp_list_idx != cur || insp_list_side != insp_side) {
                    int nd = osr_scrub_frame_ndraws(isc, iframe);
                    insp_draws.resize(nd > 0 ? (size_t)nd : 0);
                    if (nd > 0) osr_scrub_frame_draws(isc, iframe, insp_draws.data(), nd);
                    insp_list_idx = cur; insp_list_side = insp_side;
                    insp_k = -1; insp_sel = -1;
                }
                int nd = (int)insp_draws.size();
                ImGui::Text("tick %u   %s frame #%d   %d draws", e.tick,
                            insp_side == 0 ? "port" : "retail", iframe, nd);
                int kshow = (insp_k < 0) ? nd : insp_k;
                ImGui::SetNextItemWidth(340);
                if (ImGui::SliderInt("up to draw K", &kshow, 0, nd))
                    insp_k = (kshow >= nd) ? -1 : kshow;
                ImGui::SameLine(); if (ImGui::Button("all")) { insp_k = -1; insp_sel = -1; }

                pInsp.ensure(w, h);
                if (insp_shown_k != insp_k || insp_shown_idx != cur || insp_shown_side != insp_side) {
                    if (osr_scrub_render_rgba_upto(isc, iframe, insp_k, binsp)) pInsp.upload(binsp);
                    insp_shown_k = insp_k; insp_shown_idx = cur; insp_shown_side = insp_side;
                }
                float iavail = ImGui::GetContentRegionAvail().x;
                float iscale = iavail / (float)w;
                if (iscale > 1.0f) iscale = 1.0f;
                if (iscale < 0.3f) iscale = 0.3f;
                ImVec2 imn = ImGui::GetCursorScreenPos();
                ImVec2 isz(w * iscale, h * iscale);
                ImGui::InvisibleButton("##inspimg", isz);   // owns the click → no window drag
                ImDrawList* idl = ImGui::GetWindowDrawList();
                if (pInsp.srv) idl->AddImage((ImTextureID)pInsp.srv, imn, ImVec2(imn.x + isz.x, imn.y + isz.y));
                else           idl->AddRectFilled(imn, ImVec2(imn.x + isz.x, imn.y + isz.y), IM_COL32(15, 15, 18, 255));
                if (ImGui::IsItemActivated()) {
                    ImVec2 mp = ImGui::GetMousePos();
                    int px = (int)((mp.x - imn.x) / iscale), py = (int)((mp.y - imn.y) / iscale);
                    int d = osr_scrub_pick_draw(isc, iframe, px, py);
                    if (d >= 0) { insp_sel = d; insp_k = d + 1; }
                }
                if (insp_sel >= 0 && insp_sel < nd) {          // highlight the selected draw
                    osr_draw_info& di = insp_draws[insp_sel];
                    idl->AddRect(
                        ImVec2(imn.x + di.dx * iscale, imn.y + di.dy * iscale),
                        ImVec2(imn.x + (di.dx + di.w) * iscale, imn.y + (di.dy + di.h) * iscale),
                        IM_COL32(0, 255, 0, 255), 0.0f, 0, 2.0f);
                    ImGui::Text("selected #%d: %s", insp_sel, di.label);
                } else {
                    ImGui::TextDisabled("no draw selected — click the image or a list row");
                }
                ImGui::BeginChild("##drawlist", ImVec2(0, 0), ImGuiChildFlags_Borders);
                ImGuiListClipper clip;
                clip.Begin(nd);
                while (clip.Step())
                    for (int i = clip.DisplayStart; i < clip.DisplayEnd; i++) {
                        char lbl[112];
                        snprintf(lbl, sizeof lbl, "#%-4d %s", i, insp_draws[i].label);
                        if (ImGui::Selectable(lbl, i == insp_sel)) { insp_sel = i; insp_k = i + 1; }
                    }
                ImGui::EndChild();
            }
        }
        ImGui::End();

        ImGui::Render();
        const float clear[4] = { 0.08f, 0.08f, 0.10f, 1.0f };
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swap->Present(1, 0);
    }

    osr_scrub_close(port); osr_scrub_close(retail);
    pPort.release(); pRetail.release(); pDiff.release(); pInsp.release();
    free(bport); free(bret); free(bdiff); free(ma); free(mb); free(mtmp); free(binsp);
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
