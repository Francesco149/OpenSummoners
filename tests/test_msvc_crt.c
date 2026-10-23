/*
 * tests/test_msvc_crt.c — the port CRT's pure parts (src/msvc_crt.c): _itoa, gmtime / localtime (zone bias, the USA
 * DST rules _isindst falls back to), __loctotime_t, strftime, sprintf.  The CRT keeps its tables and globals where
 * retail does (g_eng.data): the test maps .rdata / .data of vendor/unpacked/sotes.unpacked.exe at their VAs — skipped
 * without it.  No Win32: every test pre-sets the zone globals and the __tzset once-flag, so no import is called.
 */
#include "t.h"
#include "engine_calls.h"
#include "msvc_crt.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LO 0x5cc000u
#define HI 0x8ac000u
static uint8_t *g_img;
static const void *img_data(uint32_t va) { return va >= LO && va < HI ? g_img + (va - LO) : NULL; }
#define G32(va) (*(int32_t *)(g_img + ((va) - LO)))

static int img_load(void)
{
    if (g_img) return 0;
    FILE *f = fopen("../vendor/unpacked/sotes.unpacked.exe", "rb");
    if (!f) return -1;
    g_img = calloc(1, HI - LO);
    /* .rdata (va 0x5cc000, raw 0x1cc000, 0x287536 B), .data (va 0x854000, raw 0x454000, 0x53000 raw B) */
    int ok = !fseek(f, 0x1cc000, SEEK_SET) && fread(g_img, 1, 0x287536, f) == 0x287536 &&
             !fseek(f, 0x454000, SEEK_SET) && fread(g_img + (0x854000 - LO), 1, 0x53000, f) == 0x53000;
    fclose(f);
    if (!ok) { free(g_img); g_img = NULL; return -1; }
    return 0;
}
/* the zone: _timezone / _daylight / _dstbias, tzapiused 0 (the USA rules), the dst-year caches cleared, tzset done */
static void zone(int32_t tz, int32_t daylight, int32_t dstbias)
{
    G32(0x8a5400) = tz; G32(0x8a5404) = daylight; G32(0x8a5408) = dstbias;
    G32(0x8a9c48) = 0; G32(0x8a5498) = -1; G32(0x8a54a8) = -1; G32(0x8a9d00) = 1;
}
#define SETUP() do { if (img_load()) T_SKIP("vendor/unpacked/sotes.unpacked.exe not present"); g_eng.data = img_data; } while (0)

int test_crt_itoa_radix_and_sign(void)
{
    char b[40];
    T_ASSERT(!strcmp(crt_itoa(255, b, 16), "ff"));
    T_ASSERT(!strcmp(crt_itoa(-1, b, 16), "ffffffff"));    /* only radix 10 signs */
    T_ASSERT(!strcmp(crt_itoa(-42, b, 10), "-42"));
    T_ASSERT(!strcmp(crt_itoa(0, b, 10), "0"));
    T_ASSERT(!strcmp(crt_itoa(0x5b10d0, b, 16), "5b10d0"));
    return 0;
}

int test_crt_gmtime_epoch_leap_and_year_end(void)
{
    SETUP();
    int32_t t = 0;
    crt_tm *tm = crt_gmtime(&t);
    T_ASSERT((uint8_t *)tm == g_img + (0x8a9d08 - LO));   /* retail's static struct tm */
    T_ASSERT(tm->year == 70 && tm->mon == 0 && tm->mday == 1 && tm->hour == 0 && tm->wday == 4 && tm->yday == 0);
    t = 951782400;                                         /* 2000-02-29 00:00:00 UTC */
    tm = crt_gmtime(&t);
    T_ASSERT(tm->year == 100 && tm->mon == 1 && tm->mday == 29 && tm->yday == 59 && tm->wday == 2);
    t = 1767225599;                                        /* 2025-12-31 23:59:59 */
    tm = crt_gmtime(&t);
    T_ASSERT(tm->year == 125 && tm->mon == 11 && tm->mday == 31 && tm->yday == 364 && tm->wday == 3);
    T_ASSERT(tm->hour == 23 && tm->min == 59 && tm->sec == 59 && tm->isdst == 0);
    t = -1;
    T_ASSERT(crt_gmtime(&t) == NULL);
    return 0;
}

int test_crt_localtime_zone_and_usa_dst(void)
{
    SETUP();
    zone(-9 * 3600, 0, 0);                                 /* JST, no DST */
    int32_t t = 951782400 + 13 * 3600;                     /* 2000-02-29 13:00 UTC = 22:00 JST */
    crt_tm *tm = crt_localtime(&t);
    T_ASSERT(tm->mday == 29 && tm->hour == 22 && tm->isdst == 0);
    zone(5 * 3600, 1, -3600);                              /* EST/EDT, the pre-2007 USA rules (tzapiused 0) */
    t = 962452800;                                         /* 2000-07-01 12:00 UTC = 08:00 EDT */
    tm = crt_localtime(&t);
    T_ASSERT(tm->hour == 8 && tm->isdst == 1);
    t = 947937600;                                         /* 2000-01-15 12:00 UTC = 07:00 EST */
    tm = crt_localtime(&t);
    T_ASSERT(tm->hour == 7 && tm->isdst == 0);
    t = 954658800;                                         /* 2000-04-02 07:00 UTC: 03:00 EDT, just past the switch */
    tm = crt_localtime(&t);
    T_ASSERT(tm->mon == 3 && tm->mday == 2 && tm->hour == 3 && tm->isdst == 1);
    t = 954655200;                                         /* 06:00 UTC: 01:00 EST, before it */
    tm = crt_localtime(&t);
    T_ASSERT(tm->hour == 1 && tm->isdst == 0);
    return 0;
}

int test_crt_loctotime_inverts_localtime(void)
{
    SETUP();
    zone(5 * 3600, 1, -3600);
    T_ASSERT(crt_loctotime(2000, 7, 1, 8, 0, 0, 1) == 962452800);   /* dst flag 1: the bias applies */
    T_ASSERT(crt_loctotime(2000, 1, 15, 7, 0, 0, 0) == 947937600);
    zone(0, 0, 0);
    T_ASSERT(crt_loctotime(1970, 1, 1, 0, 0, 0, 0) == 0);
    T_ASSERT(crt_loctotime(2000, 2, 29, 0, 0, 0, 0) == 951782400);
    T_ASSERT(crt_loctotime(1969, 12, 31, 0, 0, 0, 0) == -1);       /* before 1970: -1 */
    return 0;
}

int test_crt_strftime_save_stamp_and_names(void)
{
    SETUP();
    crt_tm tm = { 5, 7, 13, 29, 1, 100, 2, 59, 0 };       /* Tue 2000-02-29 13:07:05 */
    char b[64];
    T_ASSERT(crt_strftime(b, sizeof b, "%Y/%m/%d %H:%M", &tm) == 16 && !strcmp(b, "2000/02/29 13:07"));   /* 0x8a0b70 */
    T_ASSERT(crt_strftime(b, sizeof b, "%a %A %b %B %j %p %y %I %%", &tm) > 0);
    T_ASSERT(!strcmp(b, "Tue Tuesday Feb February 060 PM 00 01 %"));
    T_ASSERT(crt_strftime(b, sizeof b, "%#d/%#m", &tm) > 0 && !strcmp(b, "29/2"));   /* '#': no leading zeros */
    T_ASSERT(crt_strftime(b, 5, "%Y/%m", &tm) == 0);       /* too small: 0 */
    return 0;
}

int test_crt_sprintf_formats(void)
{
    char b[64];
    T_ASSERT(crt_sprintf(b, "%d", -7) == 2 && !strcmp(b, "-7"));
    T_ASSERT(crt_sprintf(b, "%s learned %s.%%n", "Arche", "Fire") > 0 && !strcmp(b, "Arche learned Fire.%n"));
    T_ASSERT(crt_sprintf(b, "%05x|%-3d|", 0xbeef, 4) > 0 && !strcmp(b, "0beef|4  |"));
    return 0;
}
