/*
 * msvc_crt.{c,h} — the MSVC 6 C runtime AS THE GAME USES IT, ported: the 16 CRT entry points retail's game code
 * calls (operator new / delete, atexit / exit, the stdio of its log files and its text table, time / localtime /
 * strftime for the save stamp, setlocale, srand, sprintf, _itoa) and the process entry `start` 0x5c0a8f.
 *
 * Retail links LIBC.LIB (VC6, -ML) statically; under rhost's pure / hybrid runs that CRT still runs as retail code.
 * The standalone (rhost --standalone, opensummoners-sa.exe) maps no retail code, so these stand in for it:
 *   - every OS effect goes through retail's own import slots (g_eng.api): rhost's deterministic stand-ins (virtual
 *     clock, file overlay, heap arena) stay under it, the live platform gets the real calls;
 *   - CRT state the game could observe sits where retail's CRT keeps it (g_eng.data): the time-zone globals, the
 *     static `struct tm` localtime returns (0x8a9d08), the day tables, the "C" lc_time names;
 *   - the heap and the FILE objects are this module's own: PORT-DEBT(crt-heap) — Win32 heap blocks (zeroed, as
 *     rhost's site-D zeroing hands out retail's), not MSVC 6's small-block heap, so block ADDRESSES differ from
 *     retail (state hashes are not comparable; behaviour is); PORT-DEBT(crt-stdio) — unbuffered streams over
 *     Win32 handles (the file bytes are retail's: text-mode CRLF, append seeks), not the _iob / 4 KB buffers.
 * Game code reads no CRT-owned global (data_xrefs: game and CRT share only 3 pooled strings), so the CRT's other
 * internals (locale tables, _iob, the SBH headers, argv / environ copies) are not reproduced.
 *
 *   FUN_005c0a8f  crt_start      FUN_005bee10  crt_fpmath     FUN_005bf00d  crt_new        FUN_005bef0e  crt_delete
 *   FUN_005beecd  crt_atexit     FUN_005bf5db  crt_exit       FUN_005bf4e8  crt_fopen      FUN_005bf440  crt_fclose
 *   FUN_005bf496  crt_fprintf    FUN_005c08f1  crt_fscanf     FUN_005c0865  crt_fseek      FUN_005bf6df  crt_time
 *   FUN_005c2a94  crt_loctotime  FUN_005c301a  crt_tzset      FUN_005c00cc  crt_localtime  FUN_005c35fc  crt_gmtime
 *   FUN_005c3278  crt_isindst    FUN_005c3424  crt_cvtdate    FUN_005bf865  crt_strftime   FUN_005c022c  crt_setlocale
 *   FUN_005bf4fb  crt_srand      FUN_005bf3ee  crt_sprintf    FUN_005c0907  crt_itoa       FUN_005c0934  crt_xtoa
 */
#ifndef OSS_MSVC_CRT_H
#define OSS_MSVC_CRT_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/* the process entry 0x5c0a8f: precision, heap, the C++ static initializers (retail's table 0x854000..0x854040, each
 * through its VA), the command line, WinMain 0x562210, exit — never returns */
void crt_start(void);
void crt_fpmath(void);                    /* 0x5bee10: _setdefaultprecision (53-bit x87) + fnclex */

void *crt_new(uint32_t n);                /* 0x5bf00d operator new (zeroed) */
void  crt_delete(void *p);                /* 0x5bef0e operator delete */
int32_t crt_atexit(uint32_t fn);          /* 0x5beecd: fn is a retail VA (or a port wrapper), called through g_eng.retail */
void  crt_exit(int32_t code);             /* 0x5bf5db: the atexit list (last first), the streams, ExitProcess */

/* stdio: an opaque stream (the game only passes it back) */
void   *crt_fopen(const char *name, const char *mode);                  /* 0x5bf4e8 */
int32_t crt_fclose(void *f);                                            /* 0x5bf440 */
int32_t crt_fprintf(void *f, const char *fmt, ...);                     /* 0x5bf496 */
int32_t crt_vfprintf(void *f, const char *fmt, va_list ap);
int32_t crt_fscanf(void *f, const char *fmt, ...);                      /* 0x5c08f1 */
int32_t crt_vfscanf(void *f, const char *fmt, va_list ap);
int32_t crt_fseek(void *f, int32_t off, int32_t origin);                /* 0x5c0865 */

/* time: __time32_t, `struct tm` = 9 int32 (sec min hour mday mon year wday yday isdst) */
typedef struct { int32_t sec, min, hour, mday, mon, year, wday, yday, isdst; } crt_tm;
int32_t crt_time(int32_t *out);                                         /* 0x5bf6df */
int32_t crt_loctotime(int32_t yr, int32_t mo, int32_t dy, int32_t hr, int32_t mn, int32_t sc, int32_t dst);
void    crt_tzset(void);                                                /* 0x5c3005 / 0x5c301a */
crt_tm *crt_localtime(const int32_t *t);                                /* 0x5c00cc */
crt_tm *crt_gmtime(const int32_t *t);                                   /* 0x5c35fc */
int32_t crt_isindst(const crt_tm *tb);                                  /* 0x5c3278 */
uint32_t crt_strftime(char *buf, uint32_t max, const char *fmt, const crt_tm *tm);   /* 0x5bf865 */
char   *crt_setlocale(int32_t cat, const char *loc);                    /* 0x5c022c */

void    crt_srand(uint32_t seed);                                       /* 0x5bf4fb */
int32_t crt_sprintf(char *buf, const char *fmt, ...);                   /* 0x5bf3ee */
int32_t crt_vsprintf(char *buf, const char *fmt, va_list ap);
char   *crt_itoa(int32_t v, char *buf, int32_t radix);                  /* 0x5c0907 */

#endif
