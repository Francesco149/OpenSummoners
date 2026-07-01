// tools/voice_view/voice_view.c — SotES resource browser + audio preview.
//
// Proves what lives in the game's resource DLLs — sotesx_s.dll (voice),
// sotesp.dll (SE), sotesw.dll (BGM): pick a DLL, list its resources by type
// (the engine's custom `WAVE` / `DATA` types plus the standard ones), see each
// clip's audio format + duration, and play it.  Built 32-bit to MATCH the game
// DLLs, so LoadLibraryEx maps them and FindResource/LoadResource/LockResource
// exercise the SAME resource path the engine itself uses.
//
// GUI by default.  Headless helpers for scripted verification (write to a file,
// no window — usable from WSL via WSLInterop):
//   voice_view.exe --list <dll> <out.txt>          enumerate every type/name
//   voice_view.exe --dump <dll> <type> <id> <out>  extract one resource to file
//
// NOTE on format strings: mingw's wide printf family wants %ls for wchar_t* and
// %hs for char* — a bare %s reads a wchar_t* as a narrow string and stops at the
// first UTF-16 null.  So every wide-string arg below uses %ls, narrow uses %hs.
//
//   nix develop --command make -C tools/voice_view   -> build/voice_view.exe
//   (run on Windows)  build\voice_view.exe

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

// ─── resource identity (a type or a name: either an int id or a string) ──────
typedef struct { int isStr; wchar_t str[96]; WORD id; } ResId;

#define MAX_TYPES  128
#define MAX_NAMES  8192

static HMODULE g_mod;
static wchar_t g_dllpath[MAX_PATH];
static ResId   g_types[MAX_TYPES]; static int g_ntypes;
static ResId   g_names[MAX_NAMES]; static int g_nnames;
static ResId   g_curType;

static LPCWSTR res_arg(const ResId *r) { return r->isStr ? r->str : MAKEINTRESOURCEW(r->id); }

// FindResource that tolerates single-language (1041) datafiles.
static HRSRC res_find(const ResId *type, const ResId *name) {
    HRSRC hr = FindResourceW(g_mod, res_arg(name), res_arg(type));
    if (!hr) hr = FindResourceExW(g_mod, res_arg(type), res_arg(name), MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT));
    if (!hr) hr = FindResourceExW(g_mod, res_arg(type), res_arg(name), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
    return hr;
}

// Lock a resource -> pointer + size (valid until FreeLibrary).
static const BYTE *res_lock(const ResId *type, const ResId *name, DWORD *outsz) {
    HRSRC hr = res_find(type, name);
    if (!hr) return NULL;
    DWORD sz = SizeofResource(g_mod, hr);
    HGLOBAL hg = LoadResource(g_mod, hr);
    if (!hg) return NULL;
    const BYTE *p = (const BYTE *)LockResource(hg);
    if (p && outsz) *outsz = sz;
    return p;
}

// ─── RIFF/WAVE header parse (for the format + duration display) ──────────────
typedef struct { int ok; WORD tag, ch, bits; DWORD sr, byterate, datasz; double dur; } WaveInfo;

static const char *wave_tag_name(WORD t) {
    switch (t) {
        case 0x0001: return "PCM";
        case 0x0002: return "ADPCM";
        case 0x0050: return "MPEG";
        case 0x0055: return "MP3";
        case 0x0161: return "WMAv2";
        case 0xFFFE: return "EXTENSIBLE";
        default:     return "?";
    }
}

static WaveInfo wave_parse(const BYTE *p, DWORD n) {
    WaveInfo w; memset(&w, 0, sizeof w);
    if (n < 20 || memcmp(p, "RIFF", 4) || memcmp(p + 8, "WAVE", 4)) return w;
    DWORD o = 12; int havefmt = 0;
    while (o + 8 <= n) {
        DWORD csz; memcpy(&csz, p + o + 4, 4);
        if (!memcmp(p + o, "fmt ", 4) && o + 8 + 16 <= n) {
            memcpy(&w.tag, p + o + 8, 2);  memcpy(&w.ch, p + o + 10, 2);
            memcpy(&w.sr, p + o + 12, 4);  memcpy(&w.byterate, p + o + 16, 4);
            memcpy(&w.bits, p + o + 22, 2); havefmt = 1;
        } else if (!memcmp(p + o, "data", 4)) {
            w.datasz = csz;
        }
        if (csz == 0) break;               // guard: a zero-size chunk => stop
        o += 8 + csz + (csz & 1);
    }
    if (havefmt) { w.ok = 1; w.dur = w.byterate ? (double)w.datasz / (double)w.byterate : 0.0; }
    return w;
}

// ─── playback: PCM/WAV straight from memory; ASF/WMA via a temp file + MCI ───
static void play_stop(void) {
    PlaySoundW(NULL, NULL, 0);
    mciSendStringW(L"close vclip", NULL, 0, NULL);
}

static void play_mem(const BYTE *p, DWORD n) {
    play_stop();
    if (n >= 12 && !memcmp(p, "RIFF", 4)) {
        // Async play straight out of the mapped resource (stays valid while loaded).
        PlaySoundW((LPCWSTR)p, NULL, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
        return;
    }
    // Non-RIFF (e.g. sotesw.dll BGM = ASF/WMA): stage to a temp file, let MCI decode.
    wchar_t dir[MAX_PATH], path[MAX_PATH];
    GetTempPathW(MAX_PATH, dir);
    const wchar_t *ext = (n >= 4 && p[0] == 0x30 && p[1] == 0x26) ? L"wma" : L"dat";
    _snwprintf(path, MAX_PATH, L"%lssotes_voice_preview.%ls", dir, ext);
    HANDLE h = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        DWORD wr; WriteFile(h, p, n, &wr, NULL); CloseHandle(h);
        wchar_t cmd[MAX_PATH + 64];
        _snwprintf(cmd, MAX_PATH + 64, L"open \"%ls\" alias vclip", path);
        if (mciSendStringW(cmd, NULL, 0, NULL) == 0)
            mciSendStringW(L"play vclip", NULL, 0, NULL);
    }
}

// ─── enumeration (shared by GUI + CLI) ───────────────────────────────────────
static void resid_from(ResId *r, LPCWSTR s) {
    if (IS_INTRESOURCE(s)) { r->isStr = 0; r->id = (WORD)(ULONG_PTR)s; r->str[0] = 0; }
    else { r->isStr = 1; r->id = 0; lstrcpynW(r->str, s, 96); }
}

static BOOL CALLBACK cb_type(HMODULE m, LPWSTR type, LONG_PTR p) {
    (void)m; (void)p;
    if (g_ntypes < MAX_TYPES) resid_from(&g_types[g_ntypes++], type);
    return TRUE;
}
static BOOL CALLBACK cb_name(HMODULE m, LPCWSTR type, LPWSTR name, LONG_PTR p) {
    (void)m; (void)type; (void)p;
    if (g_nnames < MAX_NAMES) resid_from(&g_names[g_nnames++], name);
    return TRUE;
}

static void enum_types(void) { g_ntypes = 0; EnumResourceTypesW(g_mod, cb_type, 0); }
static void enum_names(const ResId *type) { g_nnames = 0; g_curType = *type; EnumResourceNamesW(g_mod, res_arg(type), cb_name, 0); }

static void type_label(const ResId *t, wchar_t *out, int cap) {
    if (t->isStr) lstrcpynW(out, t->str, cap);
    else {
        const wchar_t *rt = NULL;
        switch (t->id) { case 16: rt = L"VERSION"; break; case 6: rt = L"STRING"; break;
                         case 2: rt = L"BITMAP"; break; case 14: rt = L"GROUP_ICON"; break; }
        if (rt) lstrcpynW(out, rt, cap); else _snwprintf(out, cap, L"#%u", t->id);
    }
}

// One "id  size  fmt  dur" line for a name under the current type.
static void name_line(const ResId *name, wchar_t *out, int cap) {
    DWORD sz = 0; const BYTE *p = res_lock(&g_curType, name, &sz);
    wchar_t idpart[32];
    if (name->isStr) _snwprintf(idpart, 32, L"%.20ls", name->str);
    else             _snwprintf(idpart, 32, L"id %-6u", name->id);
    if (p && sz) {
        WaveInfo w = wave_parse(p, sz);
        if (w.ok)
            _snwprintf(out, cap, L"%ls  %8u B   %hs %luHz %ub %ls  %6.2fs",
                       idpart, sz, wave_tag_name(w.tag), (unsigned long)w.sr, w.bits,
                       w.ch == 1 ? L"mono" : L"stereo", w.dur);
        else if (sz >= 4 && p[0] == 0x30 && p[1] == 0x26)
            _snwprintf(out, cap, L"%ls  %8u B   ASF/WMA stream", idpart, sz);
        else
            _snwprintf(out, cap, L"%ls  %8u B   (data)", idpart, sz);
    } else {
        _snwprintf(out, cap, L"%ls  (unreadable)", idpart);
    }
}

// ─── headless CLI (verification) ─────────────────────────────────────────────
static int open_dll(const wchar_t *path) {
    if (g_mod) { FreeLibrary(g_mod); g_mod = NULL; }
    g_mod = LoadLibraryExW(path, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (!g_mod) return 0;
    lstrcpynW(g_dllpath, path, MAX_PATH);
    enum_types();
    return 1;
}

static int cli_list(const wchar_t *dll, const wchar_t *out) {
    if (!open_dll(dll)) return 2;
    FILE *f = _wfopen(out, L"w, ccs=UTF-8");
    if (!f) return 3;
    fwprintf(f, L"# %ls\n", dll);
    fwprintf(f, L"# %d resource type(s)\n", g_ntypes);
    for (int i = 0; i < g_ntypes; i++) {
        wchar_t tl[96]; type_label(&g_types[i], tl, 96);
        enum_names(&g_types[i]);
        DWORD total = 0;
        for (int j = 0; j < g_nnames; j++) { DWORD sz = 0; if (res_lock(&g_types[i], &g_names[j], &sz)) total += sz; }
        fwprintf(f, L"\n== type %-10ls : %d name(s), %lu bytes ==\n", tl, g_nnames, (unsigned long)total);
        int show = g_nnames; if (show > 24) show = 24;
        for (int j = 0; j < show; j++) { wchar_t ln[256]; name_line(&g_names[j], ln, 256); fwprintf(f, L"  %ls\n", ln); }
        if (g_nnames > show) fwprintf(f, L"  ... (%d more)\n", g_nnames - show);
    }
    fclose(f);
    return 0;
}

static int cli_dump(const wchar_t *dll, const wchar_t *typeStr, const wchar_t *idStr, const wchar_t *out) {
    if (!open_dll(dll)) return 2;
    ResId type; type.isStr = 1; lstrcpynW(type.str, typeStr, 96); type.id = 0;
    ResId name;
    if (idStr[0] >= L'0' && idStr[0] <= L'9') { name.isStr = 0; name.id = (WORD)_wtoi(idStr); name.str[0] = 0; }
    else { name.isStr = 1; lstrcpynW(name.str, idStr, 96); name.id = 0; }
    g_curType = type;
    DWORD sz = 0; const BYTE *p = res_lock(&type, &name, &sz);
    if (!p || !sz) return 4;
    FILE *f = _wfopen(out, L"wb"); if (!f) return 3;
    fwrite(p, 1, sz, f); fclose(f);
    return 0;
}

// ─── GUI ─────────────────────────────────────────────────────────────────────
enum { IDC_OPEN = 1001, IDC_TYPE, IDC_LIST, IDC_PLAY, IDC_STOP, IDC_SAVE, IDC_PATH, IDC_INFO, IDC_COUNT };
static HWND g_hPath, g_hType, g_hList, g_hInfo, g_hCount, g_hPlay, g_hStop, g_hSave;
static HFONT g_mono;

static void gui_fill_list(void) {
    SendMessageW(g_hList, LB_RESETCONTENT, 0, 0);
    for (int j = 0; j < g_nnames; j++) {
        wchar_t ln[256]; name_line(&g_names[j], ln, 256);
        int idx = (int)SendMessageW(g_hList, LB_ADDSTRING, 0, (LPARAM)ln);
        SendMessageW(g_hList, LB_SETITEMDATA, idx, (LPARAM)j);
    }
    wchar_t c[64]; _snwprintf(c, 64, L"%d resources", g_nnames);
    SetWindowTextW(g_hCount, c);
}

static void gui_fill_types(void) {
    SendMessageW(g_hType, CB_RESETCONTENT, 0, 0);
    int want = 0;
    for (int i = 0; i < g_ntypes; i++) {
        wchar_t tl[96]; type_label(&g_types[i], tl, 96);
        SendMessageW(g_hType, CB_ADDSTRING, 0, (LPARAM)tl);
        if (g_types[i].isStr && !lstrcmpW(g_types[i].str, L"WAVE")) want = i; // default to voice/SE
    }
    SendMessageW(g_hType, CB_SETCURSEL, want, 0);
    if (g_ntypes) { enum_names(&g_types[want]); gui_fill_list(); }
}

static void gui_open(HWND hwnd, const wchar_t *forced) {
    wchar_t path[MAX_PATH] = {0};
    if (forced) { lstrcpynW(path, forced, MAX_PATH); }
    else {
        OPENFILENAMEW ofn; memset(&ofn, 0, sizeof ofn);
        ofn.lStructSize = sizeof ofn; ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = L"Game DLLs (*.dll)\0*.dll\0All files (*.*)\0*.*\0";
        ofn.lpstrFile = path; ofn.nMaxFile = MAX_PATH;
        ofn.lpstrInitialDir = L"C:\\Program Files (x86)\\lizsoft\\FortuneSummoners";
        ofn.lpstrTitle = L"Pick a SotES resource DLL (sotesx_s.dll = voice)";
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        if (!GetOpenFileNameW(&ofn)) return;
    }
    play_stop();
    if (!open_dll(path)) { MessageBoxW(hwnd, L"LoadLibraryEx failed (not a 32-bit resource DLL?)", L"voice_view", MB_ICONERROR); return; }
    SetWindowTextW(g_hPath, g_dllpath);
    gui_fill_types();
    BOOL on = g_nnames > 0;
    EnableWindow(g_hPlay, on); EnableWindow(g_hStop, on); EnableWindow(g_hSave, on);
}

static int gui_selected_name(ResId *out) {
    int sel = (int)SendMessageW(g_hList, LB_GETCURSEL, 0, 0);
    if (sel < 0) return 0;
    int j = (int)SendMessageW(g_hList, LB_GETITEMDATA, sel, 0);
    if (j < 0 || j >= g_nnames) return 0;
    *out = g_names[j];
    return 1;
}

static void gui_show_info(const ResId *name) {
    DWORD sz = 0; const BYTE *p = res_lock(&g_curType, name, &sz);
    wchar_t s[512];
    if (p && sz) {
        WaveInfo w = wave_parse(p, sz);
        if (w.ok)
            _snwprintf(s, 512, L"%ls %u — RIFF/WAVE — %hs, %lu Hz, %u-bit, %ls — %u data bytes — %.2f s",
                       g_curType.isStr ? g_curType.str : L"#", name->id, wave_tag_name(w.tag),
                       (unsigned long)w.sr, w.bits, w.ch == 1 ? L"mono" : L"stereo", w.datasz, w.dur);
        else
            _snwprintf(s, 512, L"id %u — %u bytes — first bytes: %02X %02X %02X %02X %02X %02X %02X %02X",
                       name->id, sz, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
    } else {
        _snwprintf(s, 512, L"(unreadable)");
    }
    SetWindowTextW(g_hInfo, s);
}

static void gui_play_selected(void) {
    ResId n; if (!gui_selected_name(&n)) return;
    DWORD sz = 0; const BYTE *p = res_lock(&g_curType, &n, &sz);
    if (p && sz) play_mem(p, sz);
}

static void gui_save_selected(HWND hwnd) {
    ResId n; if (!gui_selected_name(&n)) return;
    DWORD sz = 0; const BYTE *p = res_lock(&g_curType, &n, &sz);
    if (!p || !sz) return;
    wchar_t path[MAX_PATH];
    _snwprintf(path, MAX_PATH, L"%ls_%u.wav", g_curType.isStr ? g_curType.str : L"res", n.id);
    OPENFILENAMEW ofn; memset(&ofn, 0, sizeof ofn);
    ofn.lStructSize = sizeof ofn; ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"WAV (*.wav)\0*.wav\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = path; ofn.nMaxFile = MAX_PATH; ofn.Flags = OFN_OVERWRITEPROMPT;
    if (GetSaveFileNameW(&ofn)) {
        HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
        if (h != INVALID_HANDLE_VALUE) { DWORD wr; WriteFile(h, p, sz, &wr, NULL); CloseHandle(h); }
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        g_mono = CreateFontW(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                             FIXED_PITCH | FF_MODERN, L"Consolas");
        HINSTANCE hi = ((LPCREATESTRUCTW)lp)->hInstance;
        CreateWindowW(L"BUTTON", L"Open DLL…", WS_CHILD | WS_VISIBLE, 10, 10, 110, 26, hwnd, (HMENU)IDC_OPEN, hi, NULL);
        g_hPath = CreateWindowW(L"STATIC", L"(no DLL loaded)", WS_CHILD | WS_VISIBLE | SS_PATHELLIPSIS, 130, 14, 500, 20, hwnd, (HMENU)IDC_PATH, hi, NULL);
        CreateWindowW(L"STATIC", L"Type:", WS_CHILD | WS_VISIBLE, 10, 48, 36, 20, hwnd, NULL, hi, NULL);
        g_hType = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 48, 44, 130, 300, hwnd, (HMENU)IDC_TYPE, hi, NULL);
        g_hPlay = CreateWindowW(L"BUTTON", L"▶ Play", WS_CHILD | WS_VISIBLE | WS_DISABLED, 190, 44, 70, 26, hwnd, (HMENU)IDC_PLAY, hi, NULL);
        g_hStop = CreateWindowW(L"BUTTON", L"■ Stop", WS_CHILD | WS_VISIBLE | WS_DISABLED, 266, 44, 70, 26, hwnd, (HMENU)IDC_STOP, hi, NULL);
        g_hSave = CreateWindowW(L"BUTTON", L"Save…", WS_CHILD | WS_VISIBLE | WS_DISABLED, 342, 44, 80, 26, hwnd, (HMENU)IDC_SAVE, hi, NULL);
        g_hCount = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 440, 48, 190, 20, hwnd, (HMENU)IDC_COUNT, hi, NULL);
        g_hList = CreateWindowW(L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | LBS_NOTIFY, 10, 80, 620, 360, hwnd, (HMENU)IDC_LIST, hi, NULL);
        g_hInfo = CreateWindowW(L"STATIC", L"Pick sotesx_s.dll to prove the JP voice bank (1,448 WAVE clips). Double-click a row to play.", WS_CHILD | WS_VISIBLE, 10, 448, 620, 40, hwnd, (HMENU)IDC_INFO, hi, NULL);
        if (g_mono) SendMessageW(g_hList, WM_SETFONT, (WPARAM)g_mono, TRUE);
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wp), code = HIWORD(wp);
        if (id == IDC_OPEN)  gui_open(hwnd, NULL);
        else if (id == IDC_PLAY)  gui_play_selected();
        else if (id == IDC_STOP)  play_stop();
        else if (id == IDC_SAVE)  gui_save_selected(hwnd);
        else if (id == IDC_TYPE && code == CBN_SELCHANGE) {
            int t = (int)SendMessageW(g_hType, CB_GETCURSEL, 0, 0);
            if (t >= 0 && t < g_ntypes) { enum_names(&g_types[t]); gui_fill_list(); SetWindowTextW(g_hInfo, L""); }
        } else if (id == IDC_LIST) {
            if (code == LBN_DBLCLK) gui_play_selected();
            else if (code == LBN_SELCHANGE) { ResId n; if (gui_selected_name(&n)) gui_show_info(&n); }
        }
        return 0;
    }
    case WM_DESTROY: play_stop(); if (g_mod) FreeLibrary(g_mod); if (g_mono) DeleteObject(g_mono); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    (void)hPrev; (void)lpCmd;
    int argc = 0; LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv && argc >= 2) {
        if (!lstrcmpW(argv[1], L"--list") && argc >= 4) return cli_list(argv[2], argv[3]);
        if (!lstrcmpW(argv[1], L"--dump") && argc >= 6) return cli_dump(argv[2], argv[3], argv[4], argv[5]);
    }

    WNDCLASSW wc; memset(&wc, 0, sizeof wc);
    wc.lpfnWndProc = WndProc; wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"SotesVoiceView";
    RegisterClassW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"SotES Resource / Voice Viewer",
                              WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
                              CW_USEDEFAULT, CW_USEDEFAULT, 656, 540, NULL, NULL, hInst, NULL);
    ShowWindow(hwnd, nShow); UpdateWindow(hwnd);

    // Auto-prompt (or auto-open a DLL passed as argv[1]) so it's one click to voice.
    if (argv && argc >= 2 && argv[1][0] != L'-') gui_open(hwnd, argv[1]);
    else gui_open(hwnd, NULL);

    MSG m;
    while (GetMessageW(&m, NULL, 0, 0) > 0) { TranslateMessage(&m); DispatchMessageW(&m); }
    return 0;
}
