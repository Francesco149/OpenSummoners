/*
 * scene_run.{c,h} — the SCENE COMMAND RUNNER on the retail layout (REPLACE-verified inside retail): the
 * frame loop every cutscene beat runs in.  A chapter script fills the event runner (its message, cast
 * commands, camera, fades, choices) and waits here (script_wait 0x439680) until the beat's wait mode ends.
 *
 *   FUN_00439690  scene_run
 *
 * Set-up, from the event runner R (raw offsets; IDA's runner type names its fields from +0x2e0 up 4 bytes
 * low, quirk #144): the letterbox mode R+0x6c (1 / 2 / 3: bars at 0 / -64 / +64, 4 none; R+0x70 keeps the
 * current height); R+0x2e4 retires the message box; every timed sound fades out; a new message R+0x78 (box
 * kind R+0x7c: 0 / 1 / 2 / 4 a balloon over the speaker R+0x2e0 — R+0x2f0 the bottom-strip variant — else a
 * narration box; the name R+0x28a, the text R+0x8a with its key tokens, the voice R+0x2e8 (its typing speed
 * 6 / 18 / 24 / 36), the portrait R+0x84 / R+0x88 cross-faded in); the screen effect R+0x24 (R+0x28 / R+0x2c)
 * and the fades R+0x30 (R+0x34..R+0x3c); the camera snap R+0x40 (R+0x44 / R+0x48) / pan R+0x4c (R+0x50 / R+0x54,
 * speed R+0x58); the listener R+0x64 / R+0x68; the choices R+0x584 (R+0x588 the pick, R+0x58c on cancel,
 * R+0x590 no cancel, the window R+0x594 w / R+0x598 x / R+0x59c y, the records from R+0x5a0); the picture
 * R+0xb28; the views' mark R+0x74; the auto-advance (40 / 50 / 60 frames).
 *
 * Each 16 ms update (the menu pacer, 0x55f550's machine): the timed sounds, the input (ring 19 → the room
 * loop's +0x34), the BGM ducked while a voice plays, a skip key (34; any fresh key while a waiting event
 * R+0x27ac is up: 6), every cast character's step (0x43e5b0; a temporary one released when done), the sound
 * R+0x580, the picture, the choice list, the old box's fade, the box (its voice, placement, fade, update), the
 * countdown R+0x57c, the listener / camera following R+0x60 / R+0x5c, the world tick unless R+0x1c, the camera
 * easer and the screen effects; then the wait mode R+0x20: 1 the message, 2 the effect, 3 the camera, 4 the
 * cast, 5 the choice, 6 the countdown, 7 the letterbox, 9 the room loop's +0x11c+0x20.  Renders + Flips per
 * the pacer (the letterbox fill on even ticks with R+0xb24, the demo captions).  Returns 6 when skipped.
 */
#ifndef OSS_SCENE_RUN_H
#define OSS_SCENE_RUN_H

#include <stdint.h>

#include "world_layout.h"

int32_t scene_run(wl_roomloop *rl, wl_event *ev);                                       /* 0x439690 */

#endif /* OSS_SCENE_RUN_H */
