/*
 * tools/ghidra-cpp-shim/stddef.h — see stdint.h for rationale.
 */
#ifndef _STDDEF_H
#define _STDDEF_H

typedef unsigned int  size_t;
typedef signed int    ptrdiff_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

/* offsetof — appears in our _Static_assert lines, which are stripped
 * by `-D_Static_assert(c,m)=` at parse time, so the value here is
 * never actually used.  Define to 0 so any stray reference parses. */
#define offsetof(type, member) 0

#endif
