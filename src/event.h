/*
 * event.{c,h} — running an actor's EVENT SCRIPT, and the resource unload that follows a
 * script's room change (retail layout; rhost-verified inside retail).
 *
 * 0x40b8f0 fills the runner *(world+0x1054) and runs the script engine 0x40c380 (still a
 * slot, PORT-DEBT(engine-call-slots)).  When the script asked for a room change (+0x0c)
 * every timed sound still playing (sys+0xaa8, run by 0x43e140) fades out at 400 per tick, and
 * every resource of load group >= 4 is released (0x43e3a0: surface sets, sound pools,
 * GDI objects); the result is then 24.
 *
 *   FUN_0040b8f0  event_run        FUN_0043e3a0  sys_unload
 */
#ifndef OSS_EVENT_H
#define OSS_EVENT_H

#include <stdint.h>

#include "world_layout.h"

/* 0x40b8f0 (thiscall runner): the script's result, 24 after a room change's unload */
int event_run(wl_event *ev, void *f00, wl_room *room, wl_actor *actor, int32_t a5, int32_t f10, int32_t mode);
/* 0x43e3a0 (thiscall sys): release every resource whose load group is >= `group` */
void sys_unload(void *sys, uint16_t group);

#endif /* OSS_EVENT_H */
