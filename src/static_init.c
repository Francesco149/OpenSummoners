/* static_init.c — the CRT's static initializers (_initterm's entries, run before WinMain).  See static_init.h.
 *
 * Each table entry is a 5-byte `jmp` thunk to its body (0x4010f0 a `call` + the atexit tail).  The doubles are x87
 * `fld m64; fmul m64; fstp m64` (53-bit precision under MSVC's control word 0x27f): the factors are .rdata doubles
 * read where retail keeps them (0x850d60.. — IDA prints them rounded), not literals.  The thunks to 0x401140 /
 * 0x4011b0 / 0x4013a0 call their ports (src/leftovers.c). */
#include "static_init.h"

#include <stddef.h>
#include <stdint.h>

#include "engine_calls.h"
#include "leftovers.h"
#include "win_app.h"

#define DBL(va) (*(double *)g_eng.data(va))
#define CRT_atexit ((intptr_t (*)(intptr_t))g_eng.retail(0x5beecdu))   /* retail's CRT atexit (cdecl) */

/* the degree <-> radian constants (0x8a9a38 / 0x8a9b90: pi / 180).  0x8a9a38 is the table's FIRST entry, 0x401000
 * (a jmp to 0x401010): IDA defines no function there, so rhost's coverage never trapped it (the hybrid ran it as retail
 * code until 2026-10-23) */
void si_401010(void) { DBL(0x8a9a38) = DBL(0x5cc288) * DBL(0x850d60); }
void si_401040(void) { DBL(0x8a6b28) = DBL(0x8a9a38) * DBL(0x850d68); }   /*  90 */
void si_401070(void) { DBL(0x8a6618) = DBL(0x8a9a38) * DBL(0x850d70); }   /* -90 */
void si_4010a0(void) { DBL(0x8a6b48) = DBL(0x8a9a38) * DBL(0x850d78); }   /*  180 */
void si_4010d0(void) { DBL(0x8a6b40) = DBL(0x8a9a38) * DBL(0x850d80); }   /* -180 */
void si_5b5fb0(void) { DBL(0x8a9b90) = DBL(0x850dd8) * DBL(0x850d60); }   /* pi * (1 / 180) */
void si_5b5fe0(void) { DBL(0x8a9b78) = DBL(0x8a9b90) * DBL(0x850d68); }
void si_5b6010(void) { DBL(0x8a9b70) = DBL(0x8a9b90) * DBL(0x850d70); }
void si_5b6040(void) { DBL(0x8a9b88) = DBL(0x8a9b90) * DBL(0x850d78); }
void si_5b60b0(void) { DBL(0x8a9b80) = DBL(0x8a9b90) * DBL(0x850d80); }

/* the object at 0x8a6b50 (its +0 cleared) and its destructor 0x401120 (ecx = it, jmp 0x5b5a90) at exit */
void si_401100(void) { *(int32_t *)g_eng.data(0x8a6b50) = 0; }
void si_401120(void) { wm_5b5a90((wl_char *)g_eng.data(0x8a6b50)); }
int32_t si_4010f0(void)
{
    si_401100();
    return (int32_t)CRT_atexit((intptr_t)g_eng.data(0x401120u));   /* si_401120 by address: retail's onexit table holds the VA */
}

/* the system singleton's pump fields: byte +0x324, dwords +0x34c / +0x350 */
int32_t si_401370(void)
{
    *(uint8_t *)((uint8_t *)g_eng.sys + 0x324) = 0;
    *(int32_t *)((uint8_t *)g_eng.sys + 0x34c) = 0;
    *(int32_t *)((uint8_t *)g_eng.sys + 0x350) = 0;
    return 0;
}

/* the thunks */
void    si_401000(void) { si_401010(); }
void    si_401030(void) { si_401040(); }
void    si_401060(void) { si_401070(); }
void    si_401090(void) { si_4010a0(); }
void    si_4010c0(void) { si_4010d0(); }
int32_t si_401130(void) { return lo_401140(); }
int32_t si_4011a0(void) { return lo_4011b0(); }
int32_t si_401360(void) { return si_401370(); }
int32_t si_401390(void) { return lo_4013a0(); }
void    si_5b5fa0(void) { si_5b5fb0(); }
void    si_5b5fd0(void) { si_5b5fe0(); }
void    si_5b6000(void) { si_5b6010(); }
void    si_5b6030(void) { si_5b6040(); }
void    si_5b60a0(void) { si_5b60b0(); }
