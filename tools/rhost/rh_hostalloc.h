/* rh_hostalloc.h — force-included (-include) into every rhost and port object of rhost.exe: the HOST's own
 * malloc / calloc / realloc / free / strdup come from a fixed reserved region (rh_hostalloc.c), not from the
 * CRT heap.  The CRT heap's growth (a hook spec, the port's map buffers, GX snapshots: different per hook set)
 * moved where wine / Windows then placed the system DLLs' heaps (ddraw objects retail keeps pointers to), so
 * pure and hybrid runs hashed differently mid-run. */
#ifndef RH_HOSTALLOC_H
#define RH_HOSTALLOC_H
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
void *rh_hmalloc(size_t n);
void *rh_hcalloc(size_t a, size_t b);
void *rh_hrealloc(void *p, size_t n);
void  rh_hfree(void *p);
char *rh_hstrdup(const char *s);
int   rh_host_in(unsigned v);   /* v inside the host heap region */
#ifndef RH_HOSTALLOC_IMPL
#define malloc rh_hmalloc
#define calloc rh_hcalloc
#define realloc rh_hrealloc
#define free rh_hfree
#define strdup rh_hstrdup
#define _strdup rh_hstrdup
#endif
#endif
