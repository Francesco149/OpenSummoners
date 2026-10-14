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
