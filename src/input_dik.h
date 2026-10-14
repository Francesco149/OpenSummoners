/*
 * input_dik.{c,h} — the keyboard INPUT PRODUCER FUN_0046a880 (retail layout; rhost-verified
 * inside retail): DirectInput keyboard data -> an input manager's event ring + held axes.
 *
 * For every buffered keyboard event (the device's +0x14 records: DIK offset, data & 0x80 =
 * down, timestamp), a fixed list of tests runs IN ORDER and each match pushes one record
 * {ring id, timestamp, down} at ring[0] (ring[1..63] = the old ring[0..62]; the old
 * ring[63] record is reused): the arrows (1/3/2/4 — or 12..15 while the skill modifier
 * (keycfg +0x580, key +0x590) is held with skill keys on), Enter 20 + 36, F1..F6
 * 16/21/22/23/10/11, the digits 24..33, Esc / Backspace 39, Up 35, Pause 18, F11 19, F8 34,
 * then the 25 bound keys of the keybind config (+0x548 + 28 n: enabled, key at +0x10;
 * some push two or three ids).  Then the held axes from the device state: the arrows
 * (not in the skill-modifier mode), the +0x548 / +0x564 / +0x5b8 keys, and Tab unless an
 * Alt is down.
 *
 *   FUN_0046a880  input_dik_produce
 */
#ifndef OSS_INPUT_DIK_H
#define OSS_INPUT_DIK_H

#include "input.h"

/* 0x46a880 (thiscall manager): `kb` = the keyboard device wrapper (sys+0x2878) */
void input_dik_produce(input_mgr *m, const void *kb);

#endif /* OSS_INPUT_DIK_H */
