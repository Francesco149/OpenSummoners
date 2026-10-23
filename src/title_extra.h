/*
 * title_extra.{c,h} — the title's other screens, the attract demo,
 * the opening, the input re-poll.  Retail layout; rhost REPLACE-wired.  Names by VA.
 *
 *   FUN_0040fe00  ts_40fe00   FUN_00410250  ts_410250   FUN_0054c530  ts_54c530   FUN_0055ff80  ts_55ff80
 *   FUN_005612f0  ts_5612f0   FUN_005640b0  ts_5640b0   FUN_00564110  ts_564110   FUN_00565b00  ts_565b00
 *   FUN_00566300  ts_566300   FUN_00568780  ts_568780   FUN_00569220  ts_569220   FUN_0056a270  ts_56a270
 *   FUN_0056a450  ts_56a450   FUN_0056a670  ts_56a670   FUN_0056aa70  ts_56aa70   FUN_0056ad50  ts_56ad50
 *   FUN_0056d620  ts_56d620   FUN_0056dd80  ts_56dd80   FUN_00583fe0  ts_583fe0   FUN_00584400  ts_584400
 *   FUN_00584420  ts_584420   FUN_00584550  ts_584550   FUN_005846f0  ts_5846f0   FUN_00584710  ts_584710
 *   FUN_00585260  ts_585260   FUN_00585400  ts_585400   FUN_00585530  ts_585530   FUN_00585590  ts_585590
 *   FUN_00585810  ts_585810   FUN_00585ae0  ts_585ae0   FUN_00585e70  ts_585e70   FUN_0059d160  ts_59d160
 *   FUN_0059d690  ts_59d690   FUN_0059d900  ts_59d900   FUN_0059dc60  ts_59dc60   FUN_0059df50  ts_59df50
 *   FUN_0059e1d0  ts_59e1d0   FUN_0059e230  ts_59e230   FUN_0059e5d0  ts_59e5d0   FUN_0059e9e0  ts_59e9e0
 */
#ifndef OSS_TITLE_EXTRA_H
#define OSS_TITLE_EXTRA_H

#include <stdint.h>
#include <stdio.h>

#include "world_layout.h"

int32_t ts_40fe00(wl_char *self);
int32_t ts_410250(wl_char *self, int32_t * a2, int32_t a3, int32_t a4);
int32_t ts_54c530(wl_char *self, char a2);
int32_t ts_55ff80(wl_char *self, int32_t a2, int32_t a3);
int32_t ts_5612f0(wl_char *self);
int32_t ts_5640b0(wl_char *self);
int32_t ts_564110(wl_char *self, int32_t a2, uint16_t a3);
int32_t ts_565b00(wl_char *self, char * a2, int32_t a3, int32_t a4, int32_t a5, uint16_t a6, uint16_t a7);
int32_t ts_566300(wl_char *self, int32_t a2);
int32_t ts_568780(int32_t a1);
int32_t ts_569220(wl_char *self, int32_t a2);
int32_t ts_56a270(wl_char *self, char * a2, int32_t a3);
int32_t ts_56a450(int32_t a1);
int32_t ts_56a670(wl_char *self, int32_t ** a2, uint16_t * a3, int32_t * a4);
int32_t ts_56aa70(wl_char *self, int32_t * a2, int32_t * a3);
void ts_56ad50(wl_char *self);
int32_t ts_56d620(wl_char *self);
int32_t ts_56dd80(int32_t a1, int32_t a2);
int32_t ts_583fe0(wl_char *self);
int32_t ts_584400(wl_char *self);
int32_t ts_584420(wl_char *self, int32_t a2, int32_t a3);
int32_t ts_584550(wl_char *self, int32_t a2, int32_t a3);
int32_t ts_5846f0(wl_char *self);
int32_t ts_584710(wl_char *self);
int32_t ts_585260(wl_char *self, int32_t a2, int32_t a3, int16_t a4, int32_t a5);
int32_t ts_585400(int32_t a1, int32_t a2);
int32_t ts_585530(int32_t * a1);
int32_t ts_585590(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t ts_585810(int32_t a1, uint16_t a2, int32_t a3, uint16_t a4, uint16_t a5, uint16_t a6, int32_t a7);
int32_t ts_585ae0(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t * a6);
int32_t ts_585e70(wl_char *self);
void ts_59d160(void);
void ts_59d690(wl_char *self);
int32_t ts_59d900(wl_char *self, char a2);
int32_t ts_59dc60(wl_char *self);
int32_t ts_59df50(wl_char *self, int32_t * a2, int32_t * a3, int32_t * a4);
int32_t ts_59e1d0(wl_char *self);
int32_t ts_59e230(wl_char *self);
int32_t ts_59e5d0(wl_char *self, int16_t a2);
int32_t ts_59e9e0(wl_char *self, uint32_t a2);

#endif /* OSS_TITLE_EXTRA_H */
