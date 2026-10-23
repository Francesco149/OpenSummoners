/*
 * engine_rest.{c,h} — the engine's remaining unreached callees.  Retail layout; rhost
 * REPLACE-wired (UNVERIFIED).  Names by VA.
 *
 *   FUN_00401df0  el_401df0   FUN_00402890  el_402890   FUN_00407f30  el_407f30   FUN_0040a2e0  el_40a2e0
 *   FUN_0040ac90  el_40ac90   FUN_00415ae0  el_415ae0   FUN_004192f0  el_4192f0   FUN_00419980  el_419980
 *   FUN_0041b470  el_41b470   FUN_0041c230  el_41c230   FUN_0041c390  el_41c390   FUN_0041cb10  el_41cb10
 *   FUN_0041db30  el_41db30   FUN_0041dba0  el_41dba0   FUN_0041e070  el_41e070   FUN_0041e4c0  el_41e4c0
 *   FUN_0041f0c0  el_41f0c0   FUN_00426110  el_426110   FUN_00438970  el_438970   FUN_0043c1b0  el_43c1b0
 *   FUN_0043d4b0  el_43d4b0   FUN_0043d540  el_43d540   FUN_0043d830  el_43d830   FUN_0043dd30  el_43dd30
 *   FUN_0043e010  el_43e010   FUN_00441de0  el_441de0   FUN_004428f0  el_4428f0   FUN_0044bf60  el_44bf60
 *   FUN_004675c0  el_4675c0   FUN_00467750  el_467750   FUN_00468970  el_468970   FUN_00479ca0  el_479ca0
 *   FUN_0047a300  el_47a300   FUN_0047a420  el_47a420   FUN_0047a7f0  el_47a7f0   FUN_0049d330  el_49d330
 *   FUN_0049eea0  el_49eea0   FUN_004c5af0  el_4c5af0   FUN_004cc0e0  el_4cc0e0   FUN_0054c5e0  el_54c5e0
 *   FUN_0054ed80  el_54ed80   FUN_005534d0  el_5534d0   FUN_00556e50  el_556e50   FUN_0055fdb0  el_55fdb0
 *   FUN_005851a0  el_5851a0
 */
#ifndef OSS_ENGINE_REST_H
#define OSS_ENGINE_REST_H

#include <stdint.h>
#include <stdio.h>

#include "world_layout.h"

int32_t el_401df0(wl_char *self, int32_t a2, int32_t a3);
int32_t el_402890(wl_char *self, int32_t a2);
int32_t el_407f30(wl_char *self, char * lpOutputString, int32_t a3, int32_t a4);
int32_t el_40a2e0(int32_t a1);
int32_t el_40ac90(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t el_415ae0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t el_4192f0(wl_char *self, int32_t a2);
int32_t el_419980(wl_char *self, uint16_t a2);
int32_t el_41b470(int32_t a1, uint16_t a2);
void el_41c230(void);
int32_t el_41c390(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, char * Format, int32_t a7, int32_t a8, int32_t a9, int32_t a10);
int32_t el_41cb10(wl_char *self, int32_t a2);
int32_t el_41db30(int32_t a1, int32_t a2);
int32_t el_41dba0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4);
int32_t el_41e070(wl_char *self, int32_t a2);
int32_t el_41e4c0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t el_41f0c0(int32_t a1, int32_t a2, int32_t a3);
int32_t el_426110(wl_char *self, char * lpOutputString, int32_t a3, char * a4);
int32_t el_438970(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t el_43c1b0(wl_char *self, int32_t hModule, int32_t a3, int32_t a4);
int32_t el_43d4b0(uint16_t * a1, int32_t a2);
int32_t el_43d540(wl_char *self);
int32_t el_43d830(wl_char *self);
void el_43dd30(wl_char *self, int32_t * a2, uint16_t a3);
int32_t el_43e010(int32_t a1, char * a2);
int32_t el_441de0(wl_char *self, int32_t * a2, int32_t a3);
int32_t el_4428f0(wl_char *self);
void el_44bf60(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t el_4675c0(wl_char *self, int32_t * a2, uint16_t a3);
int32_t el_467750(wl_char *self, int32_t a2, uint16_t a3);
int32_t el_468970(wl_char *self, char a2);
int32_t el_479ca0(wl_char *self, int32_t a2, uint32_t a3, uint32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t el_47a300(wl_char *self, int32_t a2, int32_t a3);
int32_t el_47a420(wl_char *self, int32_t a2, int32_t a3);
int32_t el_47a7f0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
void el_49d330(int32_t a1, int32_t a2);
int32_t el_49eea0(wl_char *self, int32_t a2, int32_t a3);
int32_t el_4c5af0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
void el_4cc0e0(wl_char *self, int32_t a2);
void el_54c5e0(wl_char *self);
int32_t el_54ed80(void);
int32_t el_5534d0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
void el_556e50(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
void el_55fdb0(wl_char *self);
void el_5851a0(wl_char *self);

#endif /* OSS_ENGINE_REST_H */
