/* msvc_crt.c — the MSVC 6 C runtime as the game uses it (see msvc_crt.h).  Ported from retail's statically linked
 * LIBC.LIB functions (all_ida.c; VAs per function); every OS call through retail's import slots (g_eng.api). */
#include "msvc_crt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define D(va)   ((uint8_t *)g_eng.data(va))
#define G32(va) (*(int32_t *)D(va))
#define U16(p, o) (*(uint16_t *)((uint8_t *)(p) + (o)))

/* ---- retail's CRT globals the port keeps in place ---- */
#define V_TIMEZONE  0x8a5400u   /* _timezone (s west of UTC) */
#define V_DAYLIGHT  0x8a5404u   /* _daylight */
#define V_DSTBIAS   0x8a5408u   /* _dstbias */
#define V_TZNAME0   0x8a548cu   /* _tzname[0] -> 0x8a540c (64 B) */
#define V_TZNAME1   0x8a5490u   /* _tzname[1] -> 0x8a544c (64 B) */
#define V_DSTS_YR   0x8a5498u   /* dststart: yr / yd / ms */
#define V_DSTS_YD   0x8a549cu
#define V_DSTS_MS   0x8a54a0u
#define V_DSTE_YR   0x8a54a8u   /* dstend: yr / yd / ms */
#define V_DSTE_YD   0x8a54acu
#define V_DSTE_MS   0x8a54b0u
#define V_TZAPI     0x8a9c48u   /* tzapiused */
#define V_TZI       0x8a9c50u   /* the TIME_ZONE_INFORMATION _tzset reads (172 B) */
#define V_TZFIRST   0x8a9d00u   /* __tzset's once flag */
#define V_TM        0x8a9d08u   /* gmtime's static struct tm */
#define V_CODEPAGE  0x8a9d60u   /* __lc_codepage */
#define V_TCACHE    0x8a9bf8u   /* time(): the last GetSystemTime (minute resolution) */
#define V_TDST      0x8a9bf0u   /* time(): its dst flag */
#define V_DAYS      0x8a61d0u   /* _days - 1: [mo] = days before month mo, minus 1 */
#define V_LPDAYS    0x8a619cu   /* _lpdays - 1 (leap years) */
#define V_LCTIME    0x8a4fa0u   /* __lc_time_curr -> the "C" names / pictures (0x8a4fa8) */
#define V_PCTYPE    0x8a51e8u   /* _pctype (16-bit ctype entries; 0x8000 = lead byte) */
#define V_STRF_ALT  0x8ab2c8u   /* strftime: '#' seen */
#define V_STRF_NOZ  0x8ab2ccu   /* strftime: no leading zeros (the numeric field's '#') */
#define V_ADJFDIV   0x8a9b9cu   /* _adjust_fdiv */

/* TIME_ZONE_INFORMATION / SYSTEMTIME offsets */
#define TZ_BIAS 0
#define TZ_SNAME 4
#define TZ_SDATE 68
#define TZ_SBIAS 84
#define TZ_DNAME 88
#define TZ_DDATE 152
#define TZ_DBIAS 168
#define ST_YEAR 0
#define ST_MONTH 2
#define ST_DOW 4
#define ST_DAY 6
#define ST_HOUR 8
#define ST_MIN 10
#define ST_SEC 12
#define ST_MS 14

/* ---- the imports (retail's IAT) ---- */
typedef uintptr_t (ENG_STDCALL *api_heapcreate_t)(uint32_t, uint32_t, uint32_t);
typedef void *(ENG_STDCALL *api_heapalloc_t)(uintptr_t, uint32_t, uint32_t);
typedef int32_t (ENG_STDCALL *api_heapfree_t)(uintptr_t, uint32_t, void *);
typedef uintptr_t (ENG_STDCALL *api_createfile_t)(const char *, uint32_t, uint32_t, void *, uint32_t, uint32_t, uintptr_t);
typedef int32_t (ENG_STDCALL *api_rw_t)(uintptr_t, void *, uint32_t, uint32_t *, void *);
typedef uint32_t (ENG_STDCALL *api_setfp_t)(uintptr_t, int32_t, int32_t *, uint32_t);
typedef int32_t (ENG_STDCALL *api_close_t)(uintptr_t);
typedef void (ENG_STDCALL *api_systime_t)(void *);
typedef uint32_t (ENG_STDCALL *api_gtzi_t)(void *);
typedef int32_t (ENG_STDCALL *api_wc2mb_t)(uint32_t, uint32_t, const uint16_t *, int32_t, char *, int32_t, const char *, int32_t *);
typedef char *(ENG_STDCALL *api_str_t)(void);
typedef int32_t (ENG_STDCALL *api_freeenv_t)(char *);
typedef void (ENG_STDCALL *api_si_t)(void *);
typedef uintptr_t (ENG_STDCALL *api_gmh_t)(const char *);
typedef void (ENG_STDCALL *api_exit_t)(uint32_t);
#define API(t, n) ((t)g_eng.api(n))

/* ---- 0x5bee10 __fpmath ---- */
void crt_fpmath(void)
{
    /* __cfltcvt_init (printf's float hooks: this module formats through the system CRT), _adjust_fdiv =
     * _ms_p5_mp_test_fdiv() (0: no Pentium FDIV flaw on any CPU this runs on), _setdefaultprecision =
     * _controlfp(_PC_53, _MCW_PC), fnclex */
    G32(V_ADJFDIV) = 0;
#if defined(__i386__)
    uint16_t cw;
    __asm__ volatile("fnstcw %0" : "=m"(cw));
    cw = (uint16_t)((cw & ~0x0300u) | 0x0200u);   /* PC = 10b: 53-bit mantissa */
    __asm__ volatile("fldcw %0" : : "m"(cw));
    __asm__ volatile("fnclex");
#endif
}

/* ---- the heap: PORT-DEBT(crt-heap) ---- */
static uintptr_t g_heap;
static api_heapalloc_t p_alloc;
static api_heapfree_t p_free;
static void heap_init(void)
{
    if (g_heap) return;
    g_heap = API(api_heapcreate_t, "HeapCreate")(1u /* HEAP_NO_SERIALIZE: -ML */, 0x1000u, 0);   /* _heap_init(0) */
    p_alloc = API(api_heapalloc_t, "HeapAlloc");
    p_free = API(api_heapfree_t, "HeapFree");
}
void *crt_new(uint32_t n)
{
    heap_init();
    /* _heap_alloc: 0 -> 1, rounded up to the 16-byte paragraph */
    n = n ? (n + 15u) & ~15u : 16u;
    return p_alloc(g_heap, 8u /* HEAP_ZERO_MEMORY */, n);
}
void crt_delete(void *p)
{
    if (p) { heap_init(); p_free(g_heap, 0, p); }
}

/* ---- atexit / exit (0x5beecd _onexit, 0x5bf5fd doexit) ---- */
#define ONEXIT_MAX 64
static uint32_t g_onexit[ONEXIT_MAX];
static int g_nonexit, g_exiting;
int32_t crt_atexit(uint32_t fn)
{
    if (g_nonexit == ONEXIT_MAX) return -1;
    g_onexit[g_nonexit++] = fn;
    return 0;
}
static void endstdio(void);
void crt_exit(int32_t code)
{
    if (!g_exiting) {   /* _C_Termination_Done: an exit from an atexit function skips straight to ExitProcess */
        g_exiting = 1;
        while (g_nonexit > 0) {
            uint32_t fn = g_onexit[--g_nonexit];
            if (fn) ((void (*)(void))g_eng.retail(fn))();
        }
        endstdio();   /* the pre-terminator ___endstdio 0x5c7f2f */
    }
    API(api_exit_t, "ExitProcess")((uint32_t)code);
}

/* ---- stdio: PORT-DEBT(crt-stdio) ---- */
typedef struct crt_file {
    uintptr_t h;
    int rd, wr, app, text;
    uint8_t *buf; uint32_t len, pos;   /* a read stream: the whole file (text mode: CRLF -> LF, ^Z ends it) */
    struct crt_file *next;
} crt_file;
static crt_file *g_files;

void *crt_fopen(const char *name, const char *mode)
{
    /* _openfile: r / w / a, then + b t (c n S R T D: no effect on these files) */
    uint32_t access, disp;
    int rd = 0, wr = 0, app = 0, text = 1;
    switch (*mode) {
    case 'r': rd = 1; access = 0x80000000u; disp = 3; break;              /* GENERIC_READ, OPEN_EXISTING */
    case 'w': wr = 1; access = 0x40000000u; disp = 2; break;              /* GENERIC_WRITE, CREATE_ALWAYS */
    case 'a': wr = app = 1; access = 0x40000000u; disp = 4; break;        /* GENERIC_WRITE, OPEN_ALWAYS */
    default: return NULL;
    }
    for (const char *m = mode + 1; *m; m++) {
        if (*m == '+') { rd = wr = 1; access = 0xc0000000u; }
        else if (*m == 'b') text = 0;
        else if (*m == 't') text = 1;
    }
    uintptr_t h = API(api_createfile_t, "CreateFileA")(name, access, 3u /* FILE_SHARE_READ | WRITE */, NULL, disp,
                                                       0x80u /* FILE_ATTRIBUTE_NORMAL */, 0);
    if (h == (uintptr_t)-1) return NULL;
    crt_file *f = crt_new(sizeof *f);
    f->h = h; f->rd = rd; f->wr = wr; f->app = app; f->text = text;
    if (rd) {
        api_setfp_t sfp = API(api_setfp_t, "SetFilePointer");
        uint32_t size = sfp(h, 0, NULL, 2u /* FILE_END */), got = 0;
        sfp(h, 0, NULL, 0);
        f->buf = crt_new(size + 1);
        if (size && !API(api_rw_t, "ReadFile")(h, f->buf, size, &got, NULL)) got = 0;
        f->len = got;
        if (text) {   /* _read's text mode: CR LF -> LF, a ^Z ends the data */
            uint32_t o = 0;
            for (uint32_t i = 0; i < got; i++) {
                uint8_t c = f->buf[i];
                if (c == 0x1a) break;
                if (c == '\r' && i + 1 < got && f->buf[i + 1] == '\n') continue;
                f->buf[o++] = c;
            }
            f->len = o;
        }
    }
    f->next = g_files; g_files = f;
    return f;
}

int32_t crt_fclose(void *fp)
{
    crt_file *f = fp, **pp = &g_files;
    if (!f) return -1;
    while (*pp && *pp != f) pp = &(*pp)->next;
    if (*pp) *pp = f->next;
    API(api_close_t, "CloseHandle")(f->h);
    crt_delete(f->buf);
    crt_delete(f);
    return 0;
}
static void endstdio(void)
{
    while (g_files) crt_fclose(g_files);
}

static int vformat(char *buf, uint32_t n, const char *fmt, va_list ap)
{
#if defined(_WIN32)
    return _vsnprintf(buf, n, fmt, ap);   /* the system's Microsoft CRT (msvcrt.dll): MSVC's formatting */
#else
    return vsnprintf(buf, n, fmt, ap);
#endif
}
int32_t crt_vfprintf(void *fp, const char *fmt, va_list ap)
{
    crt_file *f = fp;
    char stack[1024], *b = stack;
    uint32_t cap = sizeof stack;
    int n;
    for (;;) {
        va_list aq; va_copy(aq, ap);
        n = vformat(b, cap, fmt, aq);
        va_end(aq);
        if (n >= 0 && (uint32_t)n < cap) break;
        if (b != stack) crt_delete(b);
        cap = n >= 0 ? (uint32_t)n + 1 : cap * 2;
        b = crt_new(cap);
    }
    if (f && f->wr) {
        /* _write: an append stream seeks to the end first; text mode writes every LF as CR LF */
        uint8_t out[512]; uint32_t o = 0, put;
        api_rw_t wf = API(api_rw_t, "WriteFile");
        if (f->app) API(api_setfp_t, "SetFilePointer")(f->h, 0, NULL, 2u);
        for (int i = 0; i < n; i++) {
            if (f->text && b[i] == '\n') out[o++] = '\r';
            out[o++] = (uint8_t)b[i];
            if (o >= sizeof out - 1) { wf(f->h, out, o, &put, NULL); o = 0; }
        }
        if (o) wf(f->h, out, o, &put, NULL);
    }
    if (b != stack) crt_delete(b);
    return f && f->wr ? n : -1;
}
int32_t crt_fprintf(void *f, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int32_t n = crt_vfprintf(f, fmt, ap);
    va_end(ap);
    return n;
}

static int is_ws(int c) { return c == ' ' || (c >= 9 && c <= 13); }   /* isspace, "C" locale */
/* _input over the read buffer: white space, %[width]s / d / c, %%, literal characters (the game's only format is
 * "%1023s") */
int32_t crt_vfscanf(void *fp, const char *fmt, va_list ap)
{
    crt_file *f = fp;
    if (!f || !f->rd) return -1;
    int32_t got = 0, eof_first = 1;
    for (const char *p = fmt; *p; p++) {
        if (is_ws((uint8_t)*p)) { while (f->pos < f->len && is_ws(f->buf[f->pos])) f->pos++; continue; }
        if (*p != '%' || p[1] == '%') {
            if (*p == '%') p++;
            if (f->pos >= f->len) break;
            if (f->buf[f->pos] != (uint8_t)*p) break;
            f->pos++; continue;
        }
        p++;
        uint32_t width = 0;
        while (*p >= '0' && *p <= '9') width = width * 10 + (uint32_t)(*p++ - '0');
        if (*p != 'c') while (f->pos < f->len && is_ws(f->buf[f->pos])) f->pos++;
        if (f->pos >= f->len) break;
        eof_first = 0;
        if (*p == 's') {
            char *d = va_arg(ap, char *);
            uint32_t k = 0;
            while (f->pos < f->len && !is_ws(f->buf[f->pos]) && (!width || k < width)) d[k++] = (char)f->buf[f->pos++];
            d[k] = 0; got++;
        } else if (*p == 'c') {
            char *d = va_arg(ap, char *);
            uint32_t k = 0, w = width ? width : 1;
            while (f->pos < f->len && k < w) d[k++] = (char)f->buf[f->pos++];
            got++;
        } else if (*p == 'd') {
            int32_t *d = va_arg(ap, int32_t *), neg = 0, v = 0;
            uint32_t k = 0, nd = 0;
            if (f->buf[f->pos] == '-' || f->buf[f->pos] == '+') { neg = f->buf[f->pos] == '-'; f->pos++; k++; }
            while (f->pos < f->len && f->buf[f->pos] >= '0' && f->buf[f->pos] <= '9' && (!width || k < width))
                { v = v * 10 + (f->buf[f->pos++] - '0'); k++; nd++; }
            if (!nd) break;
            *d = neg ? -v : v; got++;
        } else break;
    }
    return got == 0 && eof_first && f->pos >= f->len ? -1 : got;
}
int32_t crt_fscanf(void *f, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int32_t n = crt_vfscanf(f, fmt, ap);
    va_end(ap);
    return n;
}

int32_t crt_fseek(void *fp, int32_t off, int32_t origin)
{
    crt_file *f = fp;
    if (!f || origin < 0 || origin > 2) return -1;
    if (f->rd && f->buf) {
        int64_t at = (origin == 0 ? 0 : origin == 1 ? (int64_t)f->pos : (int64_t)f->len) + off;
        if (at < 0) return -1;
        f->pos = at > f->len ? f->len : (uint32_t)at;
        return 0;
    }
    return API(api_setfp_t, "SetFilePointer")(f->h, off, NULL, (uint32_t)origin) == 0xffffffffu ? -1 : 0;
}

/* ---- time ---- */
static uint8_t *tzi(void) { return D(V_TZI); }

/* getenv over the process environment block (the CRT reads its startup copy, _environ) */
static int env_tz(char *out, uint32_t n)
{
    char *env = API(api_str_t, "GetEnvironmentStrings")(), *e = env;
    int found = 0;
    for (; e && *e; e += strlen(e) + 1)
        if ((e[0] == 'T' || e[0] == 't') && (e[1] == 'Z' || e[1] == 'z') && e[2] == '=') {
            snprintf(out, n, "%s", e + 3); found = 1; break;
        }
    if (env) API(api_freeenv_t, "FreeEnvironmentStringsA")(env);
    return found;
}

static char g_lasttz[256];   /* the TZ value last parsed (retail: a heap copy) */
void crt_tzset(void)
{
    /* ___tzset 0x5c3005: once */
    if (G32(V_TZFIRST)) return;
    G32(V_TZFIRST)++;
    /* __tzset 0x5c301a */
    char tz[256];
    char *tz0 = (char *)D((uint32_t)G32(V_TZNAME0)), *tz1 = (char *)D((uint32_t)G32(V_TZNAME1));
    G32(V_TZAPI) = 0;
    G32(V_DSTE_YR) = -1;
    G32(V_DSTS_YR) = -1;
    if (!env_tz(tz, sizeof tz)) {
        uint8_t *z = tzi();
        if (API(api_gtzi_t, "GetTimeZoneInformation")(z) == 0xffffffffu) return;
        G32(V_TIMEZONE) = 60 * *(int32_t *)(z + TZ_BIAS);
        G32(V_TZAPI) = 1;
        if (U16(z, TZ_SDATE + ST_MONTH)) G32(V_TIMEZONE) = 60 * *(int32_t *)(z + TZ_SBIAS) + 60 * *(int32_t *)(z + TZ_BIAS);
        if (U16(z, TZ_DDATE + ST_MONTH) && *(int32_t *)(z + TZ_DBIAS)) {
            G32(V_DAYLIGHT) = 1;
            G32(V_DSTBIAS) = 60 * (*(int32_t *)(z + TZ_DBIAS) - *(int32_t *)(z + TZ_SBIAS));
        } else {
            G32(V_DAYLIGHT) = 0;
            G32(V_DSTBIAS) = 0;
        }
        api_wc2mb_t w2m = API(api_wc2mb_t, "WideCharToMultiByte");
        int32_t used = 0;
        if (!w2m((uint32_t)G32(V_CODEPAGE), 0x220u, (const uint16_t *)(z + TZ_SNAME), -1, tz0, 63, NULL, &used) || used)
            *tz0 = 0;
        else
            tz0[63] = 0;
        used = 0;
        if (w2m((uint32_t)G32(V_CODEPAGE), 0x220u, (const uint16_t *)(z + TZ_DNAME), -1, tz1, 63, NULL, &used) && !used)
            tz1[63] = 0;
        else
            *tz1 = 0;
        return;
    }
    if (!*tz || !strcmp(tz, g_lasttz)) return;
    snprintf(g_lasttz, sizeof g_lasttz, "%s", tz);
    /* "SSS[+|-]hh[:mm[:ss]][DDD]" */
    strncpy(tz0, tz, 3); tz0[3] = 0;
    const char *p = tz + 3;
    int neg = 0;
    if (*p == '-') { p++; neg = 1; }
    int32_t v = 3600 * (int32_t)strtol(p, NULL, 10);
    G32(V_TIMEZONE) = v;
    while (*p == '+' || (*p >= '0' && *p <= '9')) p++;
    if (*p == ':') {
        v = 60 * (int32_t)strtol(++p, NULL, 10) + G32(V_TIMEZONE); G32(V_TIMEZONE) = v;
        while (*p >= '0' && *p <= '9') p++;
        if (*p == ':') {
            v = (int32_t)strtol(++p, NULL, 10) + G32(V_TIMEZONE); G32(V_TIMEZONE) = v;
            while (*p >= '0' && *p <= '9') p++;
        }
    }
    if (neg) G32(V_TIMEZONE) = -v;
    G32(V_DAYLIGHT) = (uint8_t)*p;   /* the DST name's first character (not 1) */
    if (*p) { strncpy(tz1, p, 3); tz1[3] = 0; }
    else *tz1 = 0;
}

int32_t crt_cvtdate(int32_t trantype, int32_t datetype, int32_t year, int32_t month, int32_t week, int32_t dow,
                    int32_t date, int32_t hour, int32_t min, int32_t sec, int32_t msec);
int32_t crt_cvtdate(int32_t trantype, int32_t datetype, int32_t year, int32_t month, int32_t week, int32_t dow,
                    int32_t date, int32_t hour, int32_t min, int32_t sec, int32_t msec)
{
    const int32_t *days = (const int32_t *)D(V_DAYS), *lpdays = (const int32_t *)D(V_LPDAYS);
    int32_t yd;
    if (datetype == 1) {   /* day-in-month: the week-th dow of month (5 = last) */
        int32_t leapless = year & 3;
        int32_t base = leapless ? days[month] : lpdays[month];
        int32_t first = base + 1;
        int32_t wd = (365 * year + ((year - 1) >> 2) + base + 1 - 25563) % 7;
        yd = wd > dow ? dow + 7 * week - wd + first : first + dow + 7 * week - wd - 7;
        if (week == 5 && yd > (leapless ? days[month + 1] : lpdays[month + 1])) yd -= 7;
    } else {               /* an absolute date */
        yd = date + ((year & 3) ? days[month] : lpdays[month]);
    }
    if (trantype == 1) {
        G32(V_DSTS_YD) = yd;
        G32(V_DSTS_YR) = year;
        return G32(V_DSTS_MS) = msec + 1000 * (sec + 60 * (min + 60 * hour));
    }
    int32_t ms = msec + 1000 * (sec + G32(V_DSTBIAS) + 60 * (min + 60 * hour));
    G32(V_DSTE_YD) = yd;
    G32(V_DSTE_MS) = ms;
    if (ms < 0) { G32(V_DSTE_MS) = ms + 86400000; G32(V_DSTE_YD) = yd - 1; }
    else if (ms >= 86400000) { G32(V_DSTE_MS) = ms - 86400000; G32(V_DSTE_YD) = yd + 1; }
    G32(V_DSTE_YR) = year;
    return G32(V_DSTE_MS);
}

int32_t crt_isindst(const crt_tm *tb)
{
    if (!G32(V_DAYLIGHT)) return 0;
    int32_t yr = tb->year;
    if (yr != G32(V_DSTS_YR) || yr != G32(V_DSTE_YR)) {
        if (G32(V_TZAPI)) {
            const uint8_t *z = tzi(), *dd = z + TZ_DDATE, *sd = z + TZ_SDATE;
            if (U16(dd, ST_YEAR))
                crt_cvtdate(1, 0, yr, U16(dd, ST_MONTH), 0, 0, U16(dd, ST_DAY), U16(dd, ST_HOUR), U16(dd, ST_MIN),
                            U16(dd, ST_SEC), U16(dd, ST_MS));
            else
                crt_cvtdate(1, 1, yr, U16(dd, ST_MONTH), U16(dd, ST_DAY), U16(dd, ST_DOW), 0, U16(dd, ST_HOUR),
                            U16(dd, ST_MIN), U16(dd, ST_SEC), U16(dd, ST_MS));
            if (U16(sd, ST_YEAR))
                crt_cvtdate(0, 0, tb->year, U16(sd, ST_MONTH), 0, 0, U16(sd, ST_DAY), U16(sd, ST_HOUR), U16(sd, ST_MIN),
                            U16(sd, ST_SEC), U16(sd, ST_MS));
            else
                crt_cvtdate(0, 1, tb->year, U16(sd, ST_MONTH), U16(sd, ST_DAY), U16(sd, ST_DOW), 0, U16(sd, ST_HOUR),
                            U16(sd, ST_MIN), U16(sd, ST_SEC), U16(sd, ST_MS));
        } else {   /* the USA rules: first Sunday of April .. last Sunday of October, 2 AM */
            crt_cvtdate(1, 1, yr, 4, 1, 0, 0, 2, 0, 0, 0);
            crt_cvtdate(0, 1, tb->year, 10, 5, 0, 0, 2, 0, 0, 0);
        }
    }
    int32_t yd = tb->yday, s = G32(V_DSTS_YD), e = G32(V_DSTE_YD);
    if (s < e) {
        if (yd < s || yd > e) return 0;
        if (yd > s && yd < e) return 1;
    } else {
        if (yd < e || yd > s) return 1;
        if (yd > e && yd < s) return 0;
    }
    int32_t ms = 1000 * (tb->sec + 60 * (tb->min + 60 * tb->hour));
    return yd == s ? ms >= G32(V_DSTS_MS) : ms < G32(V_DSTE_MS);
}

int32_t crt_loctotime(int32_t yr, int32_t mo, int32_t dy, int32_t hr, int32_t mn, int32_t sc, int32_t dst)
{
    int32_t y = yr - 1900;
    if (y < 70 || y > 138) return -1;
    int32_t yday = dy + ((const int32_t *)D(V_DAYS))[mo];
    if (!(y & 3) && mo > 2) yday++;
    crt_tzset();
    /* the struct tm _isindst sees: sec / min left as retail's stack garbage — here the call's own */
    crt_tm tb = { sc, mn, hr, 0, mo - 1, y, 0, yday, 0 };
    uint32_t t = (uint32_t)G32(V_TIMEZONE) +
                 60u * ((uint32_t)mn + 60u * ((uint32_t)hr + 24u * (uint32_t)(((yr - 1901) >> 2) + yday + 365 * y))) +
                 (uint32_t)sc + 2085978496u;   /* -(1900 -> 1970 seconds) mod 2^32 */
    if (dst == 1 || (dst == -1 && G32(V_DAYLIGHT) && crt_isindst(&tb))) t += (uint32_t)G32(V_DSTBIAS);
    return (int32_t)t;
}

int32_t crt_time(int32_t *out)
{
    uint8_t loc[16], gmt[16], z[172];
    API(api_systime_t, "GetLocalTime")(loc);
    API(api_systime_t, "GetSystemTime")(gmt);
    uint8_t *c = D(V_TCACHE);
    int32_t dst;
    if (U16(gmt, ST_MIN) == U16(c, ST_MIN) && U16(gmt, ST_HOUR) == U16(c, ST_HOUR) && U16(gmt, ST_DAY) == U16(c, ST_DAY) &&
        U16(gmt, ST_MONTH) == U16(c, ST_MONTH) && U16(gmt, ST_YEAR) == U16(c, ST_YEAR)) {
        dst = G32(V_TDST);
    } else {
        uint32_t st = API(api_gtzi_t, "GetTimeZoneInformation")(z);
        if (st == 0xffffffffu) dst = -1;
        else dst = st == 2 && U16(z, TZ_DDATE + ST_MONTH) && *(int32_t *)(z + TZ_DBIAS);   /* TIME_ZONE_ID_DAYLIGHT */
        memcpy(c, gmt, 16);
        G32(V_TDST) = dst;
    }
    int32_t t = crt_loctotime(U16(loc, ST_YEAR), U16(loc, ST_MONTH), U16(loc, ST_DAY), U16(loc, ST_HOUR),
                              U16(loc, ST_MIN), U16(loc, ST_SEC), dst);
    if (out) *out = t;
    return t;
}

crt_tm *crt_gmtime(const int32_t *tp)
{
    int32_t t = *tp, leap = 0;
    if (t < 0) return NULL;
    int32_t yr = 4 * (t / 126230400) + 70, r = t % 126230400;   /* 4-year blocks from 1970 (1972 the leap year) */
    if (r >= 31536000) {
        r -= 31536000; yr++;
        if (r >= 31536000) {
            r -= 31536000; yr++;
            if (r < 31622400) leap = 1;
            else { yr++; r -= 31622400; }
        }
    }
    crt_tm *tm = (crt_tm *)D(V_TM);
    tm->year = yr;
    int32_t yd = r / 86400, s = r % 86400;
    tm->yday = yd;
    const int32_t *mdays = (const int32_t *)D(leap ? V_LPDAYS + 4 : V_DAYS + 4);
    int32_t m = 1;
    while (mdays[m] < yd) m++;
    tm->mon = m - 1;
    tm->mday = yd - mdays[m - 1];
    tm->wday = (t / 86400 + 4) % 7;   /* 1970-01-01 was a Thursday */
    tm->hour = s / 3600;
    s %= 3600;
    tm->min = s / 60;
    tm->isdst = 0;
    tm->sec = s % 60;
    return tm;
}

crt_tm *crt_localtime(const int32_t *tp)
{
    int32_t t = *tp;
    if (t < 0) return NULL;
    crt_tzset();
    crt_tm *tm;
    if (t > 259200 && t < 2147224447) {   /* 3 days in from either end: the zone bias cannot wrap */
        int32_t lt = t - G32(V_TIMEZONE);
        tm = crt_gmtime(&lt);
        if (G32(V_DAYLIGHT) && crt_isindst(tm)) {
            lt -= G32(V_DSTBIAS);
            tm = crt_gmtime(&lt);
            tm->isdst = 1;
        }
        return tm;
    }
    /* near the ends: convert, then carry the bias through the fields by hand */
    tm = crt_gmtime(tp);
    int32_t s = tm->sec;
    if (crt_isindst(tm)) s -= G32(V_DSTBIAS);
    int32_t lt = s - G32(V_TIMEZONE);
    tm->sec = lt % 60;
    if (tm->sec < 0) { tm->sec += 60; lt -= 60; }
    lt = tm->min + lt / 60;
    tm->min = lt % 60;
    if (lt % 60 < 0) { tm->min += 60; lt -= 60; }
    lt = tm->hour + lt / 60;
    tm->hour = lt % 24;
    if (lt % 24 < 0) { tm->hour += 24; lt -= 24; }
    int32_t dd = lt / 24;
    if (dd <= 0) {
        if (dd >= 0) return tm;
        tm->wday = (tm->wday + dd + 7) % 7;
        tm->mday += dd;
        if (tm->mday <= 0) {
            tm->year--;
            tm->mday += 31;
            tm->yday = 364;
            tm->mon = 11;
            return tm;
        }
    } else {
        tm->wday = (dd + tm->wday) % 7;
        tm->mday += dd;
    }
    tm->yday += dd;
    return tm;
}

/* ---- strftime 0x5bf865 / _Strftime 0x5bf880 / _expandtime 0x5bf92f ---- */
static void store_str(uint32_t sva, char **out, uint32_t *left)
{
    const char *s = (const char *)D(sva);
    if (!*left) return;
    do {
        if (!*s) break;
        *(*out)++ = *s++;
    } while ((*left)-- != 1);
}
static void store_number(int32_t n, char **out, uint32_t *left)   /* 0x5bfdb4: no leading zeros */
{
    char *p = *out, *a = *out;
    do {
        if (*left <= 1) break;
        *p++ = (char)('0' + n % 10);
        --*left;
        n /= 10;
    } while (n > 0);
    *out = p;
    for (char *b = p - 1; a < b; a++, b--) { char c = *b; *b = *a; *a = c; }
}
static void store_num(int32_t n, uint32_t digits, char **out, uint32_t *left)   /* 0x5bfd45 */
{
    if (G32(V_STRF_NOZ)) { store_number(n, out, left); return; }
    if (digits >= *left) { *left = 0; return; }
    for (uint32_t i = digits; i-- > 0; n /= 10) (*out)[i] = (char)('0' + n % 10);
    *out += digits;
    *left -= digits;
}
static void expandtime(char spec, const crt_tm *tm, char **out, uint32_t *left, uint32_t lct)
{
#define LCT(o) (*(uint32_t *)D(lct + (o)))
#define NUM(v, d) do { G32(V_STRF_NOZ) = G32(V_STRF_ALT); store_num((v), (d), out, left); } while (0)
    int32_t first, wk;
    switch (spec) {
    case 'a': store_str(LCT(4 * tm->wday), out, left); break;
    case 'A': store_str(LCT(4 * tm->wday + 28), out, left); break;
    case 'b': store_str(LCT(4 * tm->mon + 56), out, left); break;
    case 'B': store_str(LCT(4 * tm->mon + 104), out, left); break;
    case 'd': NUM(tm->mday, 2); break;
    case 'H': NUM(tm->hour, 2); break;
    case 'I': NUM(tm->hour % 12 ? tm->hour % 12 : 12, 2); break;
    case 'j': NUM(tm->yday + 1, 3); break;
    case 'm': NUM(tm->mon + 1, 2); break;
    case 'M': NUM(tm->min, 2); break;
    case 'p': store_str(LCT(tm->hour > 11 ? 156 : 152), out, left); break;
    case 'S': NUM(tm->sec, 2); break;
    case 'U': case 'W':
        G32(V_STRF_NOZ) = G32(V_STRF_ALT);
        first = spec == 'U' ? tm->wday : tm->wday ? tm->wday - 1 : 6;
        wk = tm->yday >= first ? tm->yday / 7 + (tm->yday % 7 >= first) : 0;
        store_num(wk, 2, out, left);
        break;
    case 'w': NUM(tm->wday, 1); break;
    case 'y': NUM(tm->year % 100, 2); break;
    case 'Y': NUM(tm->year % 100 + 100 * (tm->year / 100 + 19), 4); break;
    case 'z': case 'Z':
        crt_tzset();
        store_str((uint32_t)G32(tm->isdst ? V_TZNAME1 : V_TZNAME0), out, left);
        break;
    case '%': *(*out)++ = '%'; --*left; break;
    /* %c %x %X (_store_winword over the locale's date / time pictures at lc_time +160..+168): the game's one
     * format ("%Y/%m/%d %H:%M", 0x8a0b70) never reaches them — not ported, they store nothing */
    default: break;
    }
#undef NUM
#undef LCT
}
uint32_t crt_strftime(char *buf, uint32_t max, const char *fmt, const crt_tm *tm)
{
    uint32_t lct = (uint32_t)G32(V_LCTIME), left = max;
    const uint8_t *ctype = D((uint32_t)G32(V_PCTYPE));
    if (!max) return 0;
    const char *p = fmt;
    do {
        uint8_t c = (uint8_t)*p;
        if (!c) break;
        if (c == '%') {
            G32(V_STRF_ALT) = 0;
            if (*++p == '#') { G32(V_STRF_ALT) = 1; p++; }
            expandtime(*p, tm, &buf, &left, lct);
            p++;
        } else {
            if ((int8_t)ctype[2 * c + 1] < 0 && left > 1) { *buf++ = (char)c; p++; left--; }   /* a DBCS lead byte */
            *buf++ = *p++;
            left--;
        }
    } while (left);
    if (!left) return 0;
    *buf = 0;
    return max - left;
}

/* setlocale 0x5c022c: the game asks once, setlocale(LC_ALL, "ja") (0x8a0b98).  VC6 resolves a short language name as a
 * case-insensitive PREFIX of a locale's English name (getqloc.c: GetPrimaryLen + _strnicmp), so "ja" is Japanese_Japan.932:
 * every category's __lc_handle (0x8a9d48 [1..5]) 0x411, __lc_codepage 0x8a9d60 / __lc_collate_cp 0x8a9d64 932 — read off
 * retail (rhost --watch, 2026-10-23).  The category tables it loads (ctype: the SJIS lead bytes; lc_time: Japanese names)
 * are not: the game reads none of them (its one strftime format is ASCII digits) and ignores the result */
char *crt_setlocale(int32_t cat, const char *loc)
{
    static char c_locale[] = "C", ja_locale[] = "Japanese_Japan.932";
    if (!loc) return cat == 0 && G32(V_CODEPAGE) == 932 ? ja_locale : c_locale;
    if (!strcmp(loc, "C")) return c_locale;
    if (cat == 0 && (loc[0] | 0x20) == 'j' && (loc[1] | 0x20) == 'a' && !loc[2]) {
        for (int c = 1; c <= 5; c++) G32(0x8a9d48u + 4u * (uint32_t)c) = 0x411;
        G32(V_CODEPAGE) = 932;
        G32(0x8a9d64u) = 932;
        return ja_locale;
    }
    return NULL;   /* (no other name reaches it) */
}

void crt_srand(uint32_t seed) { rng_srand(seed); }

int32_t crt_vsprintf(char *buf, const char *fmt, va_list ap)
{
    return vformat(buf, 0x7fffffffu, fmt, ap);   /* sprintf: an unbounded string stream (_cnt 0x7fffffff) */
}
int32_t crt_sprintf(char *buf, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int32_t n = crt_vsprintf(buf, fmt, ap);
    va_end(ap);
    return n;
}

static void crt_xtoa(uint32_t v, char *buf, uint32_t radix, int neg)   /* 0x5c0934 */
{
    char *p = buf;
    if (neg) { *p++ = '-'; v = 0u - v; }
    char *first = p;
    do {
        uint32_t d = v % radix;
        v /= radix;
        *p++ = (char)(d > 9 ? d - 10 + 'a' : d + '0');
    } while (v);
    *p-- = 0;
    for (; first < p; first++, p--) { char c = *p; *p = *first; *first = c; }
}
char *crt_itoa(int32_t v, char *buf, int32_t radix)
{
    crt_xtoa((uint32_t)v, buf, (uint32_t)radix, radix == 10 && v < 0);
    return buf;
}

/* ---- the process entry 0x5c0a8f ---- */
typedef int32_t (ENG_STDCALL *winmain_t)(uintptr_t hinst, uintptr_t prev, char *cmd, int32_t show);
void crt_start(void)
{
    /* GetVersion -> _osver / _winver / _winmajor / _winminor: the game never reads them.  _heap_init, _ioinit,
     * _acmdln / _aenvptr, _setargv / _setenvp: this module's own state.  _cinit: __fpmath, the C initializers
     * 0x854044..0x854058 (the onexit table, stdio, the MBCS table, the unhandled-exception filter — none needed
     * here), then the C++ ones 0x854000..0x854040 through their VAs (src/static_init.c) */
    crt_fpmath();
    heap_init();
    for (uint32_t va = 0x854000u; va < 0x854040u; va += 4) {
        uint32_t fn = *(uint32_t *)D(va);
        if (fn) ((void (*)(void))g_eng.retail(fn))();
    }
    uint8_t si[68] = { 0 };
    API(api_si_t, "GetStartupInfoA")(si);
    /* _wincmdln: past the program name (quoted or not) and the blanks after it */
    char *cmd = API(api_str_t, "GetCommandLineA")();
    if (!cmd) cmd = "";
    if (*cmd == '"') {
        while (*++cmd != '"' && *cmd) {}
        if (*cmd == '"') cmd++;
    } else {
        while ((uint8_t)*cmd > ' ') cmd++;
    }
    while (*cmd && (uint8_t)*cmd <= ' ') cmd++;
    int32_t show = (*(uint32_t *)(si + 44) & 1u) ? U16(si, 48) : 10;   /* STARTF_USESHOWWINDOW, else SW_SHOWDEFAULT */
    uintptr_t hinst = API(api_gmh_t, "GetModuleHandleA")(NULL);
    crt_exit(((winmain_t)g_eng.retail(0x562210u))(hinst, 0, cmd, show));
}
