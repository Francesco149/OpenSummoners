/*
 * input_dev.{c,h} — the DirectInput DEVICE layer (the DirectInput7 object, the keyboard, the joysticks: create,
 * acquire, the buffered / state polls, the getters).  Retail layout; rhost-verified inside retail (REPLACE: rhost's
 * fake DirectInput7 answers the same COM calls).  Names by VA.
 *
 *   FUN_005b9cf0  id_5b9cf0   FUN_005b9d90  id_5b9d90   FUN_005b9de0  id_5b9de0   FUN_005b9e30  id_5b9e30
 *   FUN_005b9f80  id_5b9f80   FUN_005b9fc0  id_5b9fc0   FUN_005ba070  id_5ba070   FUN_005ba120  id_5ba120
 *   FUN_005ba1c0  id_5ba1c0   FUN_005ba200  id_5ba200   FUN_005ba220  id_5ba220   FUN_005ba240  id_5ba240
 *   FUN_005ba290  id_5ba290   FUN_005ba2b0  id_5ba2b0   FUN_005ba2d0  id_5ba2d0   FUN_005ba330  id_5ba330
 *   FUN_005ba3a0  id_5ba3a0   FUN_005ba3b0  id_5ba3b0   FUN_005ba3d0  id_5ba3d0   FUN_005ba3f0  id_5ba3f0
 *   FUN_005ba410  id_5ba410   FUN_005ba4c0  id_5ba4c0   FUN_005ba520  id_5ba520   FUN_005ba540  id_5ba540
 *   FUN_005ba650  id_5ba650   FUN_005ba6a0  id_5ba6a0
 */
#ifndef OSS_INPUT_DEV_H
#define OSS_INPUT_DEV_H

#include <stdint.h>

#include "world_layout.h"

int32_t id_5b9cf0(intptr_t * a1, int32_t a2);
int32_t id_5b9d90(wl_char *self);
void id_5b9de0(wl_char *self);
void id_5b9e30(wl_char *self, char * a2, char * a3, int32_t Value);
int32_t id_5b9f80(wl_char *self, int32_t a2);
int32_t id_5b9fc0(wl_char *self, int32_t * a2, int32_t a3);
int32_t id_5ba070(wl_char *self, int32_t * a2, int32_t a3, int32_t a4);
int32_t id_5ba120(wl_char *self, int32_t * a2, int32_t a3, int32_t a4);
int32_t id_5ba1c0(wl_char *self, void * a2);
int32_t id_5ba200(void * a1, int32_t a2);
int32_t id_5ba220(wl_char *self, int32_t a2);
int32_t id_5ba240(wl_char *self);
int32_t id_5ba290(wl_char *self);
int32_t id_5ba2b0(wl_char *self);
int32_t id_5ba2d0(wl_char *self);
int32_t id_5ba330(wl_char *self, int32_t a2);
int32_t id_5ba3a0(wl_char *self, int32_t a2);
int32_t id_5ba3b0(wl_char *self, int32_t a2);
int32_t id_5ba3d0(wl_char *self, int32_t a2);
int32_t id_5ba3f0(wl_char *self, int32_t a2);
int32_t id_5ba410(wl_char *self, int32_t a2);
int32_t id_5ba4c0(wl_char *self);
int32_t id_5ba520(wl_char *self, uint8_t a2);
int32_t id_5ba540(wl_char *self, int32_t a2, int32_t a3);
int32_t id_5ba650(wl_char *self);
int32_t id_5ba6a0(wl_char *self, int32_t a2);

#endif /* OSS_INPUT_DEV_H */
