/*
 * engine_calls.{h,c} — slots for the engine functions PORTED code calls that the port
 * has not ported yet, plus the engine-global roots that code reads.
 *
 * The rhost methodology ports a function as soon as it can be VERIFIED, not when its
 * whole callee tree is done: a ported caller reaches each unported callee through a
 * slot here.  rhost binds every slot to the retail VA (tools/rhost/rh_hooks.c), so the
 * ported caller runs inside retail under a full-state shadow; the port binary binds a
 * slot when the callee gets a port (until then the caller is not on the port's live
 * path).  RETIRE a slot when its function is ported: callers then call the C function
 * directly.  Each live slot is part of PORT-DEBT(engine-call-slots).
 *
 * Conventions are retail's (checked against each callee's asm prologue/retn — IDA's
 * types are not trusted here: 0x440f40 is typed fastcall but is thiscall, 0x47adb0
 * stdcall but passes ecx through).
 */
#ifndef OSS_ENGINE_CALLS_H
#define OSS_ENGINE_CALLS_H

#include <stdint.h>

#include "world_layout.h"

#if defined(__i386__)
#define ENG_THISCALL __attribute__((thiscall))
#define ENG_STDCALL  __attribute__((stdcall))
#else
#define ENG_THISCALL
#define ENG_STDCALL
#endif

typedef struct engine_calls {
    /* roots */
    uint32_t (ENG_STDCALL *tick)(void);   /* GetTickCount — the engine's only clock          */
    void       *sys;                      /* &0x8a6b60, the system singleton (sound `this`)  */
    wl_keycfg ***keycfg;                  /* &0x8a6e80: the config is **(*keycfg) (asm: load
                                           * the global, then two derefs)                     */
    /* unported callees */
    int (ENG_THISCALL *sound_411390)(void *sys, uint32_t id, int32_t a3, int32_t a4);
    int (ENG_THISCALL *integrate_442a70)(wl_char *self, wl_cmd *cmd, wl_body *in, wl_body *out,
                                         int32_t mode, int32_t n);
    int (ENG_THISCALL *probe_440f40)(wl_char *self, uint32_t n, wl_body *b, int32_t *out);
    int (ENG_THISCALL *skill_ok_402a60)(wl_char *self, int32_t cond, wl_body *b, int32_t a3, int32_t a4);
    int (ENG_STDCALL  *party_slot_ok_4071f0)(wl_party *p, int32_t mode, int32_t slot);
    int (ENG_THISCALL *skill_479f90)(wl_char *self, wl_body *b, int32_t a3);
    int (ENG_THISCALL *skill_47a300)(wl_char *self, int32_t a2, int32_t a3);
    int (ENG_THISCALL *skill_47a420)(wl_char *self, int32_t a2, int32_t a3);
    int (ENG_THISCALL *skill_47a7f0)(wl_char *self, int32_t id, uint32_t now, int32_t slot);
    int (ENG_THISCALL *target_ok_47adb0)(wl_char *cand, wl_char *self, int32_t spell, int32_t a4,
                                         int32_t a5, int32_t a6, int32_t a7, int32_t a8);
    /* sound + particles (0x466930) */
    void **(ENG_THISCALL *sfx_voice_40b710)(void *slot, int32_t a2);
    int (ENG_THISCALL *sfx_place_40bb90)(wl_char *self, int32_t vol, int32_t pan, int32_t a4, wl_body *bodies,
                                         int32_t *out_vol, int32_t *out_pan);
    int (ENG_THISCALL *dsb_volume_5bb870)(void *buf, int32_t v);
    int (ENG_THISCALL *dsb_pan_5bb880)(void *buf, int32_t v);
    int (ENG_THISCALL *dsb_freq_5bb890)(void *buf, int32_t v);
    int (ENG_THISCALL *dsb_play_5bb8c0)(void *buf);
    int (ENG_THISCALL *particle_557370)(wl_char *self, wl_char *owner, int32_t id, wl_body *b, int32_t dx, int32_t dy,
                                        int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12,
                                        int32_t a13, int32_t a14);
} engine_calls;

extern engine_calls g_eng;

#endif /* OSS_ENGINE_CALLS_H */
