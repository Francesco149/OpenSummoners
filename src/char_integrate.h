/*
 * char_integrate.{c,h} — the per-tick character INTEGRATOR FUN_00442a70 (12 KB): the
 * dispatcher every AI calls with its command block.  Faithful on the retail layout;
 * rhost-verified inside retail (`--hook shadow:char_integrate`, full state).
 *
 * One call: copy the body `in` -> `out` (in == out for the live tick; the AIs also
 * simulate ahead on scratch bodies, probe != 0), the ground probe (0x54e5c0 +1),
 * cancel / START the commanded action (skill records char+0x754, the world action
 * serial), the action poses 0x20 / 0x68..0x6a, the clip step 0x445f70, then the
 * per-TYPE behaviour (char+0x1d4: Arche's forms 0xc35b/0xc35c, townsfolk 58010.., the
 * monsters 0xc74e.. / 0x1874a..) -> the movement step 0x467bb0 + the clip choice
 * 0x466930, and the tail (carry release, the +0x66 timer, the region-C flags 0x426be0).
 *
 * PORT-DEBT(integrate-special-poses): the inlined special poses 0x75..0x7a / 0x81
 * (flying / butterflies / riding — ~1,500 decompiled lines of physics variants) are
 * not ported: a call whose INPUT pose is one of them runs retail's 0x442a70 through
 * the engine_calls slot; one that only ENTERS such a pose mid-call is counted in
 * g_integrate_unported (rhost reports it).
 *
 *   FUN_00442a70  char_integrate
 *   FUN_0040a540  body_action_start    FUN_00426f20  body_action_reset
 *   FUN_00426f50  body_set_state       FUN_00445f70  body_clip_step
 *   FUN_00407b80  body_set_clip        FUN_00426be0  body_region_flags
 *   FUN_00445d30  char_copy_rects      FUN_00446640  skill_pose_ok
 *   FUN_00407320  char_skill_ok        FUN_00445f50  body_facing_sign
 *   FUN_00451250  body_special_clip    FUN_00442890  body_can_act
 *   FUN_00442920  char_carry           FUN_004671c0  char_float_cap
 */
#ifndef OSS_CHAR_INTEGRATE_H
#define OSS_CHAR_INTEGRATE_H

#include <stdint.h>

#include "world_layout.h"

int  char_integrate(wl_char *self, wl_cmd *cmd, wl_body *in, wl_body *out, int32_t probe, int32_t part);
extern long g_integrate_unported;

int     body_action_start(wl_body *b, int16_t pose, int32_t len);
void    body_action_reset(wl_body *b);
void    body_set_state(wl_body *b, int16_t st);
void    body_clip_step(wl_body *b);
void    body_set_clip(wl_body *b, const void *clip);
int     body_region_flags(wl_body *b);
void    char_copy_rects(const wl_char *self, wl_body *b);
int     skill_pose_ok(const wl_skill *k, const wl_body *b);
int     char_skill_ok(wl_char *self, const wl_skill *k, int32_t probe);
int32_t body_facing_sign(const wl_body *b, int32_t v);
int     body_special_clip(wl_body *b, int32_t probe);
int     body_can_act(const wl_body *b);
int     char_carry(wl_char *self, wl_char *other);
int     char_float_cap(wl_char *self, wl_body *b, int32_t part, int32_t keep_speed);

#endif /* OSS_CHAR_INTEGRATE_H */
