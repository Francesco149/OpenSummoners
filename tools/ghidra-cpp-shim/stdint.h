/*
 * tools/ghidra-cpp-shim/stdint.h
 *
 * Minimal <stdint.h> for Ghidra's bundled CPP preprocessor when it
 * parses src/*.h via ParseCSource.java.  Ghidra's CPP doesn't ship a
 * libc, so the real <stdint.h> isn't resolvable.  This shim provides
 * just the typedefs our headers use, sized for the 32-bit target the
 * decomp is built against.
 *
 * Do NOT include this from production code — the C build uses the
 * real <stdint.h> from the mingw toolchain.
 */
#ifndef _STDINT_H
#define _STDINT_H

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef signed long long    int64_t;

typedef unsigned int        uintptr_t;
typedef signed int          intptr_t;

#define UINTPTR_MAX 0xFFFFFFFFu

#endif
