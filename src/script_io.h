/*
 * script_io.{c,h} — the script engine's callees with outside effects (the log file, the BGM, gifts, voice picks) on
 * the retail layout (verified inside retail by the hybrid gate + shadows).  Names by VA.
 *
 *   FUN_00408730  si_408730   FUN_00409520  si_409520   FUN_0040a340  si_40a340   FUN_0041a160  si_41a160
 *   FUN_0041d4a0  si_41d4a0   FUN_0040a2a0  si_40a2a0   FUN_00408b90  si_408b90   FUN_0040a5d0  si_40a5d0
 */
#ifndef OSS_SCRIPT_IO_H
#define OSS_SCRIPT_IO_H

#include <stdint.h>

#include "world_layout.h"

int32_t si_408730(wl_char *self, int32_t a2);
int32_t si_409520(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t * a9, int32_t a10);
int32_t si_40a340(wl_char *self);
int32_t si_41a160(wl_char *self);
int32_t si_41d4a0(wl_char *self, int32_t a2, int32_t a3);
int32_t si_40a2a0(wl_char *self, int32_t a2);
void si_408b90(wl_char *self, char * msg, int32_t errno_decorate, char * trailer);
void si_40a5d0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);

#endif
