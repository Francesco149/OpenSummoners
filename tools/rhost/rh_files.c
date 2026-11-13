/* rh_files.c — hermetic file overlay for retail sotes (copy-on-write into the run dir).
 *
 * The game dir stays pristine and is never written. Every retail file API that names a
 * path inside the game dir (or a relative one — cwd is the game dir) resolves through:
 *   read  : <rundir>\rel if it exists, else <gamedir>\rel (unless whited-out)
 *   write : <rundir>\rel (copied from the game dir first when the open keeps content)
 * Whited-out unless the run dir provides them: the user's saves/config under user\
 * (PathName = <game>\user, savedataNN.{dat,bak,sdt}) and temp\. So a run never sees the
 * user's real saves. GetTempPathA → "<gamedir>\tmp\" (run-independent string; files land in
 * <rundir>\tmp). APIs that open paths outside kernel32's CreateFileA (winmm mmioOpenA,
 * DirectShow RenderFile in rh_media.c) resolve through rh_files_resolve(). */
#include "rhost.h"
#include <mmsystem.h>

static char G[MAX_PATH], R[MAX_PATH];
static size_t GL;
static char g_white[64][96]; static int g_nwhite;         /* exact game-relative paths deleted */
static const char *g_white_prefix[] = { "user\\", "temp\\", "user", "temp" };
static long g_nopen;
static uint32_t g_fgen;                                  /* bumped by every write into the run dir (savestates) */

static int prefix_white(const char *rel)
{
    for (size_t i = 0; i < sizeof g_white_prefix / sizeof *g_white_prefix; i++) {
        size_t n = strlen(g_white_prefix[i]);
        if (!_strnicmp(rel, g_white_prefix[i], n) && (rel[n] == 0 || g_white_prefix[i][n - 1] == '\\')) return 1;
    }
    return 0;
}
static int is_white(const char *rel)
{
    for (int i = 0; i < g_nwhite; i++) if (!_stricmp(g_white[i], rel)) return 1;
    return prefix_white(rel);
}
static void unwhite(const char *rel)
{
    for (int i = 0; i < g_nwhite; i++) if (!_stricmp(g_white[i], rel)) { strcpy(g_white[i], g_white[--g_nwhite]); return; }
}

/* path → game-relative part. Returns 1 if inside the game dir. */
static int game_rel(const char *path, char *rel, size_t n)
{
    char full[MAX_PATH];
    if (!path || !*path) return 0;
    if (!GetFullPathNameA(path, sizeof full, full, NULL)) return 0;   /* relative → cwd (= game dir) */
    if (_strnicmp(full, G, GL) || (full[GL] != '\\' && full[GL] != 0)) return 0;
    const char *r = full + GL; while (*r == '\\') r++;
    snprintf(rel, n, "%s", r);
    return *rel != 0;
}
static int exists(const char *p) { DWORD a = GetFileAttributesA(p); return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY); }
static void mkparents(const char *p)
{
    char t[MAX_PATH]; snprintf(t, sizeof t, "%s", p);
    for (char *s = t + strlen(R) + 1; *s; s++) if (*s == '\\') { *s = 0; CreateDirectoryA(t, NULL); *s = '\\'; }
}

/* resolve for read (want_write=0) or write (1; keep=1 → copy-on-write) */
static const char *resolve(const char *path, int want_write, int keep, char *out, size_t n)
{
    char rel[MAX_PATH];
    if (!G[0] || !game_rel(path, rel, sizeof rel)) return path;
    char rp[MAX_PATH], gp[MAX_PATH];
    snprintf(rp, sizeof rp, "%s\\%s", R, rel);
    snprintf(gp, sizeof gp, "%s\\%s", G, rel);
    if (want_write) {
        mkparents(rp);
        if (keep && !exists(rp) && !is_white(rel) && exists(gp)) CopyFileA(gp, rp, TRUE);
        unwhite(rel);
        snprintf(out, n, "%s", rp);
    } else if (exists(rp)) snprintf(out, n, "%s", rp);
    else if (is_white(rel)) snprintf(out, n, "%s", rp);    /* → not found */
    else snprintf(out, n, "%s", gp);
    return out;
}
const char *rh_files_resolve(const char *path, int want_write, char *out, size_t n)
{ return resolve(path, want_write, 0, out, n); }

static HANDLE WINAPI my_CreateFileA(LPCSTR p, DWORD acc, DWORD share, LPSECURITY_ATTRIBUTES sa, DWORD disp, DWORD fl, HANDLE tmpl)
{
    int w = (acc & (GENERIC_WRITE | FILE_WRITE_DATA | FILE_APPEND_DATA)) || disp == CREATE_ALWAYS ||
            disp == CREATE_NEW || disp == TRUNCATE_EXISTING;
    int keep = disp == OPEN_EXISTING || disp == OPEN_ALWAYS;
    char buf[MAX_PATH];
    const char *q = resolve(p, w, keep, buf, sizeof buf);
    HANDLE h = CreateFileA(q, acc, share, sa, disp, fl, tmpl);
    g_nopen++;
    if (w) g_fgen++;
    if (O.log_files) rh_log("file %s %s -> %s = %s (f%u)\n", w ? "W" : "R", p, q, h == INVALID_HANDLE_VALUE ? "FAIL" : "ok", g_frame);
    return h;
}
static BOOL WINAPI my_DeleteFileA(LPCSTR p)
{
    char rel[MAX_PATH];
    if (!game_rel(p, rel, sizeof rel)) return DeleteFileA(p);
    char rp[MAX_PATH]; snprintf(rp, sizeof rp, "%s\\%s", R, rel);
    BOOL had = exists(rp);
    DeleteFileA(rp);
    g_fgen++;
    if (g_nwhite < 64 && !is_white(rel)) snprintf(g_white[g_nwhite++], 96, "%s", rel);
    if (O.log_files) rh_log("delete %s (f%u)\n", p, g_frame);
    (void)had;
    return TRUE;
}
static BOOL WINAPI my_CopyFileA(LPCSTR src, LPCSTR dst, BOOL fail_if)
{
    char b1[MAX_PATH], b2[MAX_PATH];
    const char *s = resolve(src, 0, 0, b1, sizeof b1), *d = resolve(dst, 1, 0, b2, sizeof b2);
    BOOL r = CopyFileA(s, d, fail_if);
    g_fgen++;
    if (O.log_files) rh_log("copy %s -> %s = %d (f%u)\n", src, dst, r, g_frame);
    return r;
}
static BOOL WINAPI my_CreateDirectoryA(LPCSTR p, LPSECURITY_ATTRIBUTES sa)
{
    char rel[MAX_PATH];
    if (!game_rel(p, rel, sizeof rel)) return CreateDirectoryA(p, sa);
    char rp[MAX_PATH]; snprintf(rp, sizeof rp, "%s\\%s", R, rel);
    mkparents(rp);
    CreateDirectoryA(rp, sa);
    SetLastError(ERROR_ALREADY_EXISTS);
    return FALSE;   /* retail ignores the result; "already exists" is what a normal install returns */
}
static HMMIO WINAPI my_mmioOpenA(LPSTR p, LPMMIOINFO info, DWORD fl)
{
    char buf[MAX_PATH];
    if (!p || (fl & MMIO_CREATE) == 0) {
        const char *q = p ? resolve(p, 0, 0, buf, sizeof buf) : p;
        HMMIO h = mmioOpenA((LPSTR)q, info, fl);
        if (O.log_files) rh_log("mmioOpen %s -> %s = %p (f%u)\n", p ? p : "(null)", q ? q : "", (void *)h, g_frame);
        return h;
    }
    return mmioOpenA((LPSTR)resolve(p, 1, 0, buf, sizeof buf), info, fl);
}
/* temp files: retail sees a run-INDEPENDENT path (<gamedir>\tmp\, mapped into <rundir>\tmp\)
 * and deterministic names — the real API's names depend on the clock and on what exists */
static DWORD WINAPI my_GetTempPathA(DWORD n, LPSTR buf)
{
    char t[MAX_PATH]; snprintf(t, sizeof t, "%s\\tmp", R); CreateDirectoryA(t, NULL);
    snprintf(t, sizeof t, "%s\\tmp\\", G);
    snprintf(buf, n, "%s", t); return (DWORD)strlen(t);
}
static UINT g_tmpseq;
static UINT WINAPI my_GetTempFileNameA(LPCSTR path, LPCSTR prefix, UINT unique, LPSTR out)
{
    UINT u = unique ? unique : ++g_tmpseq;
    snprintf(out, MAX_PATH, "%s%s%.3s%04X.tmp", path, (*path && path[strlen(path) - 1] != '\\') ? "\\" : "", prefix ? prefix : "", u & 0xffff);
    if (!unique) {
        HANDLE h = my_CreateFileA(out, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) return 0;
        CloseHandle(h);
    }
    return u;
}

void rh_files_install(void)
{
    GetFullPathNameA(O.gamedir, sizeof G, G, NULL);
    GetFullPathNameA(O.rundir, sizeof R, R, NULL);
    for (size_t l = strlen(G); l && G[l - 1] == '\\'; ) G[--l] = 0;
    for (size_t l = strlen(R); l && R[l - 1] == '\\'; ) R[--l] = 0;
    GL = strlen(G);
    CreateDirectoryA(R, NULL);
    iat_override("CreateFileA", my_CreateFileA);
    iat_override("DeleteFileA", my_DeleteFileA);
    iat_override("CopyFileA", my_CopyFileA);
    iat_override("CreateDirectoryA", my_CreateDirectoryA);
    iat_override("GetTempPathA", my_GetTempPathA);
    iat_override("GetTempFileNameA", my_GetTempFileNameA);
    iat_try("mmioOpenA", my_mmioOpenA);
    char up[MAX_PATH]; snprintf(up, sizeof up, "%s\\user", R);
    DWORD a = GetFileAttributesA(up);
    rh_log("overlay: game=%s run=%s user=%s\n", G, R, a != INVALID_FILE_ATTRIBUTES ? "run-dir" : "none (whited-out: fresh install)");
}

/* savestates: the run dir's user\ files (the saves / config retail writes) by content, captured once per write
 * generation; the whiteouts and the temp-name sequence as host blocks */
typedef struct fblob { uint32_t gen, ref, n, bytes; char (*name)[64]; uint32_t *len; uint8_t *data; } fblob_t;
static fblob_t *g_fcur;
static fblob_t *blob_capture(void)
{
    fblob_t *b = calloc(1, sizeof *b);
    b->gen = g_fgen;
    char pat[MAX_PATH]; snprintf(pat, sizeof pat, "%s\\user\\*", R);
    WIN32_FIND_DATAA fd; HANDLE fh = FindFirstFileA(pat, &fd);
    if (fh == INVALID_HANDLE_VALUE) return b;
    uint32_t cap = 0;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        char p[MAX_PATH]; snprintf(p, sizeof p, "%s\\user\\%s", R, fd.cFileName);
        HANDLE h = CreateFileA(p, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (h == INVALID_HANDLE_VALUE) continue;
        DWORD sz = GetFileSize(h, NULL), got = 0;
        if (b->n % 16 == 0) { b->name = realloc(b->name, (b->n + 16) * sizeof *b->name); b->len = realloc(b->len, (b->n + 16) * 4); }
        if (b->bytes + sz > cap) { cap = (b->bytes + sz) * 2 + 4096; b->data = realloc(b->data, cap); }
        ReadFile(h, b->data + b->bytes, sz, &got, NULL); CloseHandle(h);
        snprintf(b->name[b->n], 64, "%s", fd.cFileName); b->len[b->n] = got; b->bytes += got; b->n++;
    } while (FindNextFileA(fh, &fd));
    FindClose(fh);
    return b;
}
static void blob_unref(fblob_t *b) { if (b && --b->ref == 0) { free(b->name); free(b->len); free(b->data); free(b); } }
static void *fs_save(void)
{
    if (!g_fcur || g_fcur->gen != g_fgen) { blob_unref(g_fcur); g_fcur = blob_capture(); g_fcur->ref = 1; }
    g_fcur->ref++;
    return g_fcur;
}
static void fs_restore(const void *p)
{
    fblob_t *b = (fblob_t *)p;
    if (g_fcur == b && b->gen == g_fgen) return;
    char pat[MAX_PATH]; snprintf(pat, sizeof pat, "%s\\user\\*", R);
    WIN32_FIND_DATAA fd; HANDLE fh = FindFirstFileA(pat, &fd);
    if (fh != INVALID_HANDLE_VALUE) {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            char p2[MAX_PATH]; snprintf(p2, sizeof p2, "%s\\user\\%s", R, fd.cFileName);
            DeleteFileA(p2);
        } while (FindNextFileA(fh, &fd));
        FindClose(fh);
    }
    uint32_t o = 0;
    for (uint32_t i = 0; i < b->n; o += b->len[i], i++) {
        char p2[MAX_PATH]; snprintf(p2, sizeof p2, "%s\\user\\%s", R, b->name[i]);
        mkparents(p2);
        HANDLE h = CreateFileA(p2, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        DWORD put = 0;
        if (h == INVALID_HANDLE_VALUE || !WriteFile(h, b->data + o, b->len[i], &put, NULL) || put != b->len[i])
            rh_log("rh_files: could not put %s back for a savestate (err %lu)\n", b->name[i], GetLastError());
        if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    }
    blob_unref(g_fcur); g_fcur = b; b->ref++;
    b->gen = ++g_fgen;              /* the files ARE b now; g_fgen stays monotonic across branches */
}
static void fs_free(void *p) { blob_unref(p); }
void rh_files_snapreg(void)
{
    rh_snap_module("files", fs_save, fs_restore, fs_free);
    rh_snap_register("white", g_white, sizeof g_white); rh_snap_register("nwhite", &g_nwhite, sizeof g_nwhite);
    rh_snap_register("tmpseq", &g_tmpseq, sizeof g_tmpseq); rh_snap_register("nopen", &g_nopen, sizeof g_nopen);
}
