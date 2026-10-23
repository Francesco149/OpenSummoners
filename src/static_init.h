/*
 * static_init.{c,h} — the CRT's STATIC INITIALIZERS: the _initterm table's entries (jmp thunks) and their bodies —
 * the degree <-> radian doubles, two zeroed globals + an atexit'd destructor.  Retail layout (g_eng.data).
 *
 *   FUN_00401000  si_401000   FUN_00401010  si_401010
 *   FUN_00401030  si_401030   FUN_00401040  si_401040   FUN_00401060  si_401060   FUN_00401070  si_401070
 *   FUN_00401090  si_401090   FUN_004010a0  si_4010a0   FUN_004010c0  si_4010c0   FUN_004010d0  si_4010d0
 *   FUN_004010f0  si_4010f0   FUN_00401100  si_401100   FUN_00401120  si_401120   FUN_00401130  si_401130   FUN_004011a0  si_4011a0
 *   FUN_00401360  si_401360   FUN_00401370  si_401370   FUN_00401390  si_401390
 *   FUN_005b5fa0  si_5b5fa0   FUN_005b5fb0  si_5b5fb0   FUN_005b5fd0  si_5b5fd0   FUN_005b5fe0  si_5b5fe0
 *   FUN_005b6000  si_5b6000   FUN_005b6010  si_5b6010   FUN_005b6030  si_5b6030   FUN_005b6040  si_5b6040
 *   FUN_005b60a0  si_5b60a0   FUN_005b60b0  si_5b60b0
 */
#ifndef OSS_STATIC_INIT_H
#define OSS_STATIC_INIT_H

#include <stdint.h>

void    si_401000(void);
void    si_401010(void);
void    si_401030(void);
void    si_401040(void);
void    si_401060(void);
void    si_401070(void);
void    si_401090(void);
void    si_4010a0(void);
void    si_4010c0(void);
void    si_4010d0(void);
int32_t si_4010f0(void);
void    si_401100(void);
void    si_401120(void);
int32_t si_401130(void);
int32_t si_4011a0(void);
int32_t si_401360(void);
int32_t si_401370(void);
int32_t si_401390(void);
void    si_5b5fa0(void);
void    si_5b5fb0(void);
void    si_5b5fd0(void);
void    si_5b5fe0(void);
void    si_5b6000(void);
void    si_5b6010(void);
void    si_5b6030(void);
void    si_5b6040(void);
void    si_5b60a0(void);
void    si_5b60b0(void);

#endif
