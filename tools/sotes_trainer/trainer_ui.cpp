// tools/sotes_trainer/trainer_ui.cpp — the trainer's Dear ImGui window.
//
// A separate top-level window (its own D3D11 device + thread) that opens when the trainer DLL
// loads.  It is a SECOND front-end onto the same in-process trainer core as the socket
// (trainer_core.h): reads fill typed structs each frame, buttons call the tc_* actions (which
// enqueue onto the engine thread — see trainer.c).  DX11/Win32 boilerplate = the stock Dear
// ImGui example (as tools/osr_view); the "cosmic2d" theme is slopstudio's.
//
// Hotkeys (global, handled in trainer.c): F7 = toggle mouse-fly.  Here: F8 = show/hide window.

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#include <d3d11.h>
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cctype>
#include <vector>

#include "trainer_core.h"

// ── DX11 device (stock ImGui win32+dx11 example) ─────────────────────────────
static ID3D11Device*           g_dev  = nullptr;
static ID3D11DeviceContext*    g_ctx  = nullptr;
static IDXGISwapChain*         g_swap = nullptr;
static ID3D11RenderTargetView* g_rtv  = nullptr;
static bool                    g_visible = true;

static void CreateRenderTarget() {
    ID3D11Texture2D* back = nullptr;
    g_swap->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) { g_dev->CreateRenderTargetView(back, nullptr, &g_rtv); back->Release(); }
}
static void CleanupRenderTarget() { if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; } }
static bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd; ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60; sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    const D3D_FEATURE_LEVEL lvls[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL fl;
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, lvls, 2,
            D3D11_SDK_VERSION, &sd, &g_swap, &g_dev, &fl, &g_ctx) != S_OK &&
        D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, lvls, 2,
            D3D11_SDK_VERSION, &sd, &g_swap, &g_dev, &fl, &g_ctx) != S_OK)
        return false;
    CreateRenderTarget();
    return true;
}
static void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_swap) { g_swap->Release(); g_swap = nullptr; }
    if (g_ctx)  { g_ctx->Release();  g_ctx = nullptr; }
    if (g_dev)  { g_dev->Release();  g_dev = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wp, lp)) return true;
    switch (msg) {
    case WM_SIZE:
        if (g_dev && wp != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_swap->ResizeBuffers(0, (UINT)LOWORD(lp), (UINT)HIWORD(lp), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_CLOSE:                          // hide, don't destroy (F8 re-shows; the trainer keeps running)
        g_visible = false; ShowWindow(hWnd, SW_HIDE); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hWnd, msg, wp, lp);
}

// ── theme: slopstudio's cosmic2d (deep-purple base, mint accent, periwinkle focus) ──
static void apply_trainer_theme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    auto C = [](unsigned rgba){ return ImVec4(((rgba>>24)&255)/255.f, ((rgba>>16)&255)/255.f, ((rgba>>8)&255)/255.f, (rgba&255)/255.f); };
    const ImVec4 panel=C(0x1e1b2effu), panel2=C(0x262238ffu), panel3=C(0x322d48ffu),
                 edge=C(0x3a3560ffu), edgeHot=C(0x6a60a0ffu), active=C(0x4a4370ffu), accent=C(0x7fd8a8ffu),
                 accentHot=C(0x9fe8c0ffu), focus=C(0x8878d0ffu), text=C(0xe8e4ffffu), dim=C(0x8a84b0ffu), bg=C(0x141220ffu);
    ImVec4* c = s.Colors;
    c[ImGuiCol_Text]=text;                    c[ImGuiCol_TextDisabled]=dim;
    c[ImGuiCol_WindowBg]=panel;               c[ImGuiCol_ChildBg]=C(0x191527ffu);       c[ImGuiCol_PopupBg]=C(0x1e1b2ef7u);
    c[ImGuiCol_Border]=edge;                  c[ImGuiCol_BorderShadow]=ImVec4(0,0,0,0);
    c[ImGuiCol_FrameBg]=panel2;               c[ImGuiCol_FrameBgHovered]=panel3;        c[ImGuiCol_FrameBgActive]=active;
    c[ImGuiCol_TitleBg]=bg;                   c[ImGuiCol_TitleBgActive]=panel2;         c[ImGuiCol_TitleBgCollapsed]=bg;
    c[ImGuiCol_MenuBarBg]=C(0x181528ffu);
    c[ImGuiCol_ScrollbarBg]=ImVec4(0,0,0,0);  c[ImGuiCol_ScrollbarGrab]=panel3;         c[ImGuiCol_ScrollbarGrabHovered]=edgeHot; c[ImGuiCol_ScrollbarGrabActive]=focus;
    c[ImGuiCol_CheckMark]=accent;             c[ImGuiCol_SliderGrab]=accent;            c[ImGuiCol_SliderGrabActive]=accentHot;
    c[ImGuiCol_Button]=panel2;                c[ImGuiCol_ButtonHovered]=panel3;         c[ImGuiCol_ButtonActive]=active;
    c[ImGuiCol_Header]=panel3;                c[ImGuiCol_HeaderHovered]=edgeHot;        c[ImGuiCol_HeaderActive]=focus;
    c[ImGuiCol_Separator]=edge;               c[ImGuiCol_SeparatorHovered]=edgeHot;     c[ImGuiCol_SeparatorActive]=focus;
    c[ImGuiCol_ResizeGrip]=panel3;            c[ImGuiCol_ResizeGripHovered]=edgeHot;    c[ImGuiCol_ResizeGripActive]=focus;
    c[ImGuiCol_Tab]=panel;                    c[ImGuiCol_TabHovered]=panel3;            c[ImGuiCol_TabSelected]=panel3;
    c[ImGuiCol_TabDimmed]=bg;                 c[ImGuiCol_TabDimmedSelected]=panel2;
    c[ImGuiCol_TabSelectedOverline]=accent;   c[ImGuiCol_TabDimmedSelectedOverline]=edge;
    c[ImGuiCol_PlotLines]=accent;             c[ImGuiCol_PlotLinesHovered]=accentHot;   c[ImGuiCol_PlotHistogram]=accent;
    c[ImGuiCol_TextSelectedBg]=C(0x7fd8a855u); c[ImGuiCol_DragDropTarget]=accent;       c[ImGuiCol_NavCursor]=focus;
    s.WindowRounding=8; s.ChildRounding=6; s.FrameRounding=6; s.PopupRounding=6; s.GrabRounding=5; s.TabRounding=6; s.ScrollbarRounding=8;
    s.WindowBorderSize=1; s.ChildBorderSize=1; s.PopupBorderSize=1; s.FrameBorderSize=0; s.SeparatorTextBorderSize=2;
    s.WindowPadding=ImVec2(12,12); s.FramePadding=ImVec2(9,5); s.CellPadding=ImVec2(6,4);
    s.ItemSpacing=ImVec2(8,7); s.ItemInnerSpacing=ImVec2(6,5); s.IndentSpacing=18;
    s.ScrollbarSize=13; s.GrabMinSize=11;
}
static const ImVec4 COL_ACCENT = ImVec4(0x7f/255.f, 0xd8/255.f, 0xa8/255.f, 1);
static const ImVec4 COL_DIM    = ImVec4(0x8a/255.f, 0x84/255.f, 0xb0/255.f, 1);
static const ImVec4 COL_WARN   = ImVec4(0xff/255.f, 0xb0/255.f, 0x5a/255.f, 1);

// ── async load/newgame worker (Win32 thread; no std::thread dependency) ──────
static volatile LONG g_busy = 0;                 // 1 while a menu-drive runs
static char g_busy_what[64] = "";
static char g_result[192]   = "";
static struct { int slot; int newgame; } g_req;
static DWORD WINAPI load_worker(void*) {
    char msg[160] = {0};
    if (g_req.newgame) {
        int ok = tc_newgame();
        snprintf(g_result, sizeof g_result, ok ? "new game started" : "new game failed (not at title?)");
    } else {
        int ok = tc_load(g_req.slot, msg, sizeof msg);
        snprintf(g_result, sizeof g_result, "%s%s%s", ok ? "loaded" : "load failed",
                 msg[0] ? " - " : "", msg);
    }
    InterlockedExchange(&g_busy, 0);
    return 0;
}
static void start_load(int slot, int newgame) {
    if (InterlockedCompareExchange(&g_busy, 1, 0) != 0) return;   // one drive at a time
    g_req.slot = slot; g_req.newgame = newgame;
    snprintf(g_busy_what, sizeof g_busy_what,
             newgame ? "starting new game..." : (slot < 0 ? "loading newest save..." : "loading slot %d..."), slot);
    g_result[0] = 0;
    CloseHandle(CreateThread(nullptr, 0, load_worker, nullptr, 0, nullptr));
}

// ── data caches (refreshed on demand — the room table + save files are heavier reads) ──
static std::vector<tc_room> g_rooms;
static std::vector<tc_save> g_saves;
static int  g_save_sel   = -1;    // selected save slot
static int  g_edit_slot  = -1;    // exit slot whose destination is being changed
static char g_room_query[64] = "";

static void refresh_rooms() {
    g_rooms.assign(1024, tc_room{});
    int n = tc_get_rooms(g_rooms.data(), (int)g_rooms.size());
    g_rooms.resize(n < 0 ? 0 : n);
}
static void refresh_saves() {
    g_saves.assign(16, tc_save{});
    int n = tc_get_saves(g_saves.data(), (int)g_saves.size());
    g_saves.resize(n < 0 ? 0 : n);
}
static bool ci_contains(const char* hay, const char* needle) {
    if (!needle || !needle[0]) return true;
    for (const char* h = hay; *h; ++h) {
        const char *a = h, *b = needle;
        while (*a && *b && tolower((unsigned char)*a) == tolower((unsigned char)*b)) { ++a; ++b; }
        if (!*b) return true;
    }
    return false;
}

// ── small widget helpers ─────────────────────────────────────────────────────
static void toggle_row(const char* key, const char* label, const char* help) {
    bool v = tc_get_toggle(key) != 0;
    if (ImGui::Checkbox(label, &v)) tc_set_toggle(key, v ? 1 : 0);
    if (help && *help && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", help);
}
static void kv(const char* k, const char* fmt, ...) {
    ImGui::TextColored(COL_DIM, "%s", k); ImGui::SameLine(150);
    va_list ap; va_start(ap, fmt); char buf[128]; vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    ImGui::TextUnformatted(buf);
}

// ── panels ───────────────────────────────────────────────────────────────────
static void panel_cheats() {
    if (!ImGui::CollapsingHeader("Cheats", ImGuiTreeNodeFlags_DefaultOpen)) return;
    toggle_row("god",        "God (freeze HP + MP at 9999)",   "Pin HP and MP to 9999 every frame so nothing kills you and casting is free. On by default.");
    toggle_row("autoskip",   "Auto-skip dialogue (TAB)",       "Auto-advance ALL story/cutscene/NPC dialogue, hands-free. On by default; turn off to read or pick a choice.");
    toggle_row("mousefly",   "Mouse-fly (F7)",                 "Continuously teleport the player to the cursor over the game window (view frozen while flying). Also toggled by F7.");
    ImGui::Spacing();
    toggle_row("fastskip",   "Instant text",                   "Snap the current dialogue line's typewriter reveal to the end (door-safe UI-state write).");
    toggle_row("dlgskip",    "Auto-advance open dialogue",     "Injects the advance buttons while a box is up. WARNING: those ids double as world action input, so it AUTO-CONFIRMS world prompts (bed/door) you land on with mouse-fly. Off by default; auto-skip is the world-safe one.");
    ImGui::Spacing();
    toggle_row("keepactive", "Keep running unfocused",         "Re-post WM_ACTIVATEAPP so the game keeps updating while its window is in the background. On by default.");
    toggle_row("attract",    "Freeze title (no attract demo)", "Hold the title screen so it never cycles to the attract demo. On by default.");
}

static void panel_teleport(const tc_player& pl, bool hasp) {
    if (!ImGui::CollapsingHeader("Teleport", ImGuiTreeNodeFlags_DefaultOpen)) return;
    static int tx = 0, ty = 0;
    if (hasp) {
        ImGui::TextColored(COL_DIM, "current:"); ImGui::SameLine();
        ImGui::Text("X %d  Y %d", pl.world_x, pl.world_y);
        ImGui::SameLine();
        ImGui::TextDisabled("(%d, %d px)", pl.world_x / 100, pl.world_y / 100);
        if (ImGui::SmallButton("copy current")) { tx = pl.world_x; ty = pl.world_y; }
    } else {
        ImGui::TextColored(COL_WARN, "no player (load a game first)");
    }
    ImGui::SetNextItemWidth(140); ImGui::InputInt("world X##tp", &tx, 100, 1000);
    ImGui::SetNextItemWidth(140); ImGui::InputInt("world Y##tp", &ty, 100, 1000);
    ImGui::TextDisabled("world units = centi-px (px x 100)");
    ImGui::BeginDisabled(!hasp);
    if (ImGui::Button("Teleport")) tc_teleport(tx, ty, 1, 1, 0);
    ImGui::EndDisabled();
}

static void panel_player(const tc_player& pl, bool hasp) {
    if (!ImGui::CollapsingHeader("Player", ImGuiTreeNodeFlags_DefaultOpen)) return;
    if (!hasp) { ImGui::TextColored(COL_WARN, "no player (load a game first)"); return; }
    kv("HP",         "%d / %d", pl.hp, pl.hp_max);
    kv("MP",         "%d / %d", pl.mp, pl.mp_max);
    kv("Combat lv max", "%d", pl.combat_level_max);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("The max combat level (stat +0xe0): the 'N' in the "
                                                  "stat window's 'combat level M/N', shown as the HUD "
                                                  "stars. NOT the character's EXP-derived display level.");
    kv("EXP",        "%d / %d", pl.exp_cur, pl.exp_max);
    kv("actor",      "0x%08x", pl.actor);
    kv("stat block", "0x%08x", pl.stat_block);
    ImGui::Spacing();
    ImGui::SeparatorText("set");
    static int sv_level = 5, sv_hpmax = 100, sv_mpmax = 20;
    ImGui::SetNextItemWidth(110); ImGui::InputInt("##lvl", &sv_level); ImGui::SameLine();
    if (ImGui::Button("set combat lv max")) tc_setstat("combat_level_max", sv_level, 0);
    ImGui::SetNextItemWidth(110); ImGui::InputInt("##hpm", &sv_hpmax); ImGui::SameLine();
    if (ImGui::Button("set HP max")) tc_setstat("hp_max", sv_hpmax, 0);
    ImGui::SetNextItemWidth(110); ImGui::InputInt("##mpm", &sv_mpmax); ImGui::SameLine();
    if (ImGui::Button("set MP max")) tc_setstat("mp_max", sv_mpmax, 0);
}

static void panel_map(const tc_map& mp, bool hasm) {
    if (!ImGui::CollapsingHeader("Map", ImGuiTreeNodeFlags_DefaultOpen)) return;
    if (!hasm) { ImGui::TextColored(COL_WARN, "not in a scene"); return; }
    kv("room key", "%u  (0x%x)", mp.room_key, mp.room_key);
    kv("area",     "%u", mp.area);
    kv("scene (DATA)", "%u", mp.scene);
    kv("tileset",  "%u", mp.tileset);
    kv("parallax", "%u", mp.parallax);
    kv("room record", "0x%08x", mp.room_record);
}

static void panel_portals(const tc_map& mp, bool hasm) {
    if (!ImGui::CollapsingHeader("Portals", ImGuiTreeNodeFlags_DefaultOpen)) return;
    if (!hasm) { ImGui::TextColored(COL_WARN, "not in a scene"); return; }
    if (mp.n_exits == 0) { ImGui::TextDisabled("no exits in this room"); return; }
    ImGui::TextDisabled("change a door's destination to warp somewhere else (within-area).");
    if (ImGui::BeginTable("portals", 4, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("exit");
        ImGui::TableSetupColumn("-> target");
        ImGui::TableSetupColumn("change");
        ImGui::TableSetupColumn("revert");
        ImGui::TableHeadersRow();
        for (int i = 0; i < mp.n_exits; ++i) {
            const tc_exit& e = mp.exits[i];
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("#%d  key %u", e.slot, e.exit_key);
            ImGui::TableSetColumnIndex(1);
            if (e.hijacked) { ImGui::TextColored(COL_ACCENT, "%u", e.target_room); ImGui::SameLine(); ImGui::TextDisabled("(was %u)", e.orig_target); }
            else            ImGui::Text("%u", e.target_room);
            ImGui::TableSetColumnIndex(2);
            ImGui::PushID(e.slot);
            if (ImGui::SmallButton("change")) { g_edit_slot = e.slot; g_room_query[0] = 0; if (g_rooms.empty()) refresh_rooms(); ImGui::OpenPopup("change_portal"); }
            // per-row popup (opened above; content rendered once, keyed by g_edit_slot)
            if (ImGui::BeginPopup("change_portal")) {
                ImGui::TextColored(COL_ACCENT, "warp door #%d to room:", g_edit_slot);
                ImGui::SetNextItemWidth(300);
                ImGui::InputTextWithHint("##q", "search room key / area / scene", g_room_query, sizeof g_room_query);
                ImGui::SameLine(); if (ImGui::SmallButton("refresh")) refresh_rooms();
                ImGui::TextDisabled("%d rooms resident", (int)g_rooms.size());
                ImGui::BeginChild("roomlist", ImVec2(320, 260), true);
                for (const tc_room& r : g_rooms) {
                    char label[96];
                    snprintf(label, sizeof label, "%u    area %u   scene %u", r.key, r.area, r.scene);
                    if (!ci_contains(label, g_room_query)) continue;
                    if (ImGui::Selectable(label)) { tc_hijack_exit(g_edit_slot, r.key); g_edit_slot = -1; ImGui::CloseCurrentPopup(); }
                }
                ImGui::EndChild();
                if (ImGui::Button("cancel")) { g_edit_slot = -1; ImGui::CloseCurrentPopup(); }
                ImGui::EndPopup();
            }
            ImGui::TableSetColumnIndex(3);
            ImGui::BeginDisabled(!e.hijacked);
            if (ImGui::SmallButton("revert")) tc_revert_exit(e.slot);
            ImGui::EndDisabled();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

static void panel_saves(const tc_status& st) {
    if (!ImGui::CollapsingHeader("Saves", ImGuiTreeNodeFlags_DefaultOpen)) return;
    if (g_saves.empty()) refresh_saves();
    if (ImGui::SmallButton("refresh saves")) refresh_saves();
    ImGui::SameLine(); ImGui::TextDisabled("%d on disk", (int)g_saves.size());
    if (!st.at_title)
        ImGui::TextColored(COL_WARN, "loading is only available from the TITLE screen");
    ImGui::TextDisabled("cLv = max combat level (stat +0xe0 / HUD stars), not the display level.");

    ImGui::BeginChild("savelist", ImVec2(0, 190), true);
    for (const tc_save& sv : g_saves) {
        char label[160];
        if (sv.valid)
            snprintf(label, sizeof label, "slot %-2d   %-12s   cLv %-2d   %u KB", sv.slot,
                     sv.party0[0] ? sv.party0 : "(party)", sv.level0, (unsigned)(sv.file_size / 1024));
        else
            snprintf(label, sizeof label, "slot %-2d   <invalid>", sv.slot);
        if (ImGui::Selectable(label, g_save_sel == sv.slot)) g_save_sel = sv.slot;
    }
    ImGui::EndChild();

    bool can = st.at_title && !g_busy;
    ImGui::BeginDisabled(!can || g_save_sel < 0);
    if (ImGui::Button("Load selected slot")) start_load(g_save_sel, 0);
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!can);
    if (ImGui::Button("Load newest")) start_load(-1, 0);
    ImGui::SameLine();
    if (ImGui::Button("New Game")) start_load(0, 1);
    ImGui::EndDisabled();

    if (g_busy) { ImGui::SameLine(); ImGui::TextColored(COL_WARN, "%s", g_busy_what); }
    else if (g_result[0]) ImGui::TextColored(COL_ACCENT, "%s", g_result);
}

static void panel_camera(const tc_status& st, const tc_player& pl, bool hasp) {
    if (!ImGui::CollapsingHeader("Mouse-fly / camera (advanced)")) return;
    ImGui::TextWrapped("Mouse-fly maps the cursor over the game window to world coords via the view "
                       "object *(render_root + offset): view top-left = the eased scroll origin "
                       "(cur_x/cur_y), span 640x480. The view is frozen while flying so she stays under "
                       "the cursor. The pointer offset is exposed only for a future game update.");
    if (st.cam_ok) {
        kv("view left/top", "%d , %d", st.cam_x, st.cam_y);
        if (hasp) kv("world_x - left", "%d", pl.world_x - st.cam_x);
    } else {
        ImGui::TextColored(COL_WARN, "camera unreadable at offset 0x%x", (unsigned)tc_get_cam_off());
    }
    static int off = 0; if (off == 0) off = (int)tc_get_cam_off();
    ImGui::SetNextItemWidth(140);
    ImGui::InputInt("cam ptr offset (hex)##camoff", &off, 4, 16, ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine(); if (ImGui::Button("set##off")) tc_set_cam_off((uint32_t)off);
    kv("game window", "0x%08x", st.game_hwnd);
}

// ── main window ──────────────────────────────────────────────────────────────
static void draw_ui() {
    tc_status st; tc_get_status(&st);
    tc_player pl; bool hasp = tc_get_player(&pl) != 0;
    tc_map    mp; bool hasm = tc_get_map(&mp) != 0;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("Fortune Summoners Trainer", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // header + status dot
    const char* state = !st.hooks ? "attaching..." : st.player_present ? "in game" : st.at_title ? "at title" : "menu / loading";
    ImVec4 dot = !st.hooks ? COL_WARN : st.player_present ? COL_ACCENT : COL_DIM;
    ImGui::TextColored(COL_ACCENT, "Fortune Summoners"); ImGui::SameLine();
    ImGui::TextDisabled("EN-SE trainer"); ImGui::SameLine(ImGui::GetWindowWidth() - 160);
    ImGui::TextColored(dot, "%s", "\xe2\x97\x8f"); ImGui::SameLine(); ImGui::TextUnformatted(state);
    ImGui::Separator();

    panel_cheats();
    panel_teleport(pl, hasp);
    panel_player(pl, hasp);
    panel_map(mp, hasm);
    panel_portals(mp, hasm);
    panel_saves(st);
    panel_camera(st, pl, hasp);

    ImGui::Spacing(); ImGui::Separator();
    ImGui::TextDisabled("F7 mouse-fly  |  F8 hide/show this window  |  socket :7777 (MCP)");
    ImGui::End();
}

// ── UI thread ────────────────────────────────────────────────────────────────
static DWORD WINAPI ui_thread(void*) {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandleW(nullptr),
                       nullptr, nullptr, nullptr, nullptr, L"OSSTrainerUI", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"Fortune Summoners Trainer",
                              WS_OVERLAPPEDWINDOW, 80, 80, 500, 900,
                              nullptr, nullptr, wc.hInstance, nullptr);
    if (!CreateDeviceD3D(hwnd)) { CleanupDeviceD3D(); DestroyWindow(hwnd); UnregisterClassW(wc.lpszClassName, wc.hInstance); return 1; }
    ShowWindow(hwnd, SW_SHOWNORMAL); UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;                          // don't drop imgui.ini in the game dir
    // a nicer UI font if the system has one (Segoe UI is always present on Windows); else default.
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    apply_trainer_theme();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);

    int prev_f8 = 0;
    bool running = true;
    while (running) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0u, 0u, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessage(&msg);
            if (msg.message == WM_QUIT) running = false;
        }
        if (!running) break;
        // F8 toggles the window (global; works while the game is focused)
        int f8 = (GetAsyncKeyState(VK_F8) & 0x8000) ? 1 : 0;
        if (f8 && !prev_f8) { g_visible = !g_visible; ShowWindow(hwnd, g_visible ? SW_SHOW : SW_HIDE); }
        prev_f8 = f8;
        if (!g_visible) { Sleep(80); continue; }

        ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
        draw_ui();
        ImGui::Render();
        const float clear[4] = { 0.055f, 0.043f, 0.086f, 1.0f };
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swap->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    CleanupDeviceD3D(); DestroyWindow(hwnd); UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

extern "C" void trainer_ui_start(void) { CloseHandle(CreateThread(nullptr, 0, ui_thread, nullptr, 0, nullptr)); }
