/* rh_hostalloc.c — the host heap of rhost.exe on a fixed reserved region.  See rh_hostalloc.h.
 *
 * Power-of-two size classes (a 16-byte header holds the class), free lists per class, a bump pointer committing
 * 1 MB at a time.  Nothing here calls VirtualAlloc outside [HH_LO, HH_HI), so the host's allocation history no
 * longer moves anything else in the address space. */
#define RH_HOSTALLOC_IMPL
#include "rh_hostalloc.h"

#include <stdint.h>
#include <windows.h>

#define HH_LO 0xb0000000u
#define HH_HI 0xe8000000u
#define HDR 16u

static volatile LONG g_lock;
static uint32_t g_top, g_committed;
static void *g_free[32];

static void lock(void) { while (InterlockedCompareExchange(&g_lock, 1, 0)) YieldProcessor(); }
static void unlock(void) { InterlockedExchange(&g_lock, 0); }

static void die(const char *m)
{
    OutputDebugStringA(m);
    MessageBoxA(NULL, m, "rhost host heap", MB_OK);
    ExitProcess(70);
}

static void init(void)
{
    if (VirtualAlloc((void *)(uintptr_t)HH_LO, HH_HI - HH_LO, MEM_RESERVE, PAGE_READWRITE) != (void *)(uintptr_t)HH_LO)
        die("rhost: cannot reserve the host heap at 0xb0000000");
    g_top = g_committed = HH_LO;
}

static int cls_of(size_t n)
{
    int c = 5;
    while (c < 31 && ((size_t)1 << c) < n + HDR) c++;
    return c;
}

void *rh_hmalloc(size_t n)
{
    if (!n) n = 1;
    int c = cls_of(n);
    lock();
    if (!g_top) init();
    uint8_t *b = g_free[c];
    if (b) g_free[c] = *(void **)(b + HDR);
    else {
        uint32_t sz = 1u << c;
        if (sz > HH_HI - g_top) { unlock(); die("rhost: host heap exhausted"); }
        b = (uint8_t *)(uintptr_t)g_top;
        g_top += sz;
        if (g_top > g_committed) {
            uint32_t to = (g_top + 0xfffffu) & ~0xfffffu;
            if (to > HH_HI) to = HH_HI;
            if (!VirtualAlloc((void *)(uintptr_t)g_committed, to - g_committed, MEM_COMMIT, PAGE_READWRITE)) {
                unlock(); die("rhost: host heap commit failed");
            }
            g_committed = to;
        }
    }
    *(uint32_t *)b = (uint32_t)c;
    unlock();
    return b + HDR;
}

void rh_hfree(void *p)
{
    if (!p) return;
    uint8_t *b = (uint8_t *)p - HDR;
    if ((uint32_t)(uintptr_t)b < HH_LO || (uint32_t)(uintptr_t)b >= HH_HI) { free(p); return; }   /* the CRT's own */
    int c = (int)*(uint32_t *)b;
    lock();
    *(void **)p = g_free[c];
    g_free[c] = b;
    unlock();
}

void *rh_hcalloc(size_t a, size_t b)
{
    if (b && a > (size_t)-1 / b) return NULL;
    void *p = rh_hmalloc(a * b);
    memset(p, 0, a * b);
    return p;
}

void *rh_hrealloc(void *p, size_t n)
{
    if (!p) return rh_hmalloc(n);
    if (!n) n = 1;
    if ((uint32_t)(uintptr_t)p < HH_LO + HDR || (uint32_t)(uintptr_t)p >= HH_HI) return realloc(p, n);   /* the CRT's own */
    int c = (int)*(uint32_t *)((uint8_t *)p - HDR);
    size_t have = ((size_t)1 << c) - HDR;
    if (n <= have && cls_of(n) == c) return p;
    void *q = rh_hmalloc(n);
    memcpy(q, p, n < have ? n : have);
    rh_hfree(p);
    return q;
}

char *rh_hstrdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = rh_hmalloc(n);
    memcpy(p, s, n);
    return p;
}

int rh_host_in(unsigned v) { return v >= HH_LO && v < HH_HI; }
