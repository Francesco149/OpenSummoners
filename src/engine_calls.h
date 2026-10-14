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

/* thiscall slots typed by stack-arg count (the integrator's per-type handlers take
 * pointers and ints; the slot passes them as intptr_t words, as retail pushes them) */
typedef int (ENG_THISCALL *eng_tc0)(void *self);
typedef int (ENG_THISCALL *eng_tc1)(void *self, intptr_t);
typedef int (ENG_THISCALL *eng_tc2)(void *self, intptr_t, intptr_t);
typedef int (ENG_THISCALL *eng_tc3)(void *self, intptr_t, intptr_t, intptr_t);
typedef int (ENG_THISCALL *eng_tc4)(void *self, intptr_t, intptr_t, intptr_t, intptr_t);
typedef int (ENG_THISCALL *eng_tc5)(void *self, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
typedef int (ENG_THISCALL *eng_tc6)(void *self, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
typedef int (ENG_THISCALL *eng_tc7)(void *self, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
typedef int (ENG_THISCALL *eng_tc8)(void *self, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t,
                                    intptr_t);
typedef int (ENG_THISCALL *eng_tc9)(void *self, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t,
                                    intptr_t, intptr_t);
typedef int (ENG_THISCALL *eng_tc12)(void *self, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t,
                                     intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
typedef int (ENG_THISCALL *eng_tc16)(void *self, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t,
                                     intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t,
                                     intptr_t);

typedef struct engine_calls {
    /* a retail .data address the ported code passes on (tables handed to unported
     * callees, clip records): rhost = identity; the port maps its own copies */
    const void *(*data)(uint32_t va);
    /* the CRT heap (operator new 0x5bf00d / delete 0x5bef0e): scratch buffers retail
     * allocates and frees must come from the same allocator to keep its heap identical */
    void *(*op_new)(uint32_t n);
    void  (*op_delete)(void *p);
    /* roots */
    uint32_t (ENG_STDCALL *tick)(void);   /* GetTickCount — the engine's only clock          */
    void       *sys;                      /* &0x8a6b60, the system singleton (sound `this`)  */
    void       *log;                      /* &0x8a9530, the error log (0x406440 `this`)      */
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
    int (ENG_THISCALL *dsb_stop_5bb8e0)(void *buf);
    int (ENG_THISCALL *wave_load_409e90)(wl_vpool *pool);   /* loads a pool's buffers on first use */
    int (ENG_THISCALL *log_fail_406440)(void *log, const char *msg, const char *tail);
    int (ENG_THISCALL *particle_557370)(wl_char *self, wl_char *owner, int32_t id, wl_body *b, int32_t dx, int32_t dy,
                                        int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12,
                                        int32_t a13, int32_t a14);
    /* the integrator 0x442a70's callees: per-type behaviour handlers + helpers (by VA) */
    eng_tc5 h445fd0; eng_tc3 h467260; eng_tc0 h4467d0; eng_tc6 h44a740; eng_tc8 h45a300;
    eng_tc4 h44bc10; eng_tc4 h44c250; eng_tc4 h44bf60; eng_tc7 h446900; eng_tc7 h452020;
    eng_tc7 h449810; eng_tc6 h4539b0; eng_tc7 h44d8f0; eng_tc7 h448dc0; eng_tc5 h452730;
    eng_tc4 h467380; eng_tc4 h457ff0; eng_tc5 h45e830; eng_tc7 h455d80; eng_tc6 h44b160;
    eng_tc7 h4516e0; eng_tc7 h4484b0; eng_tc6 h4512d0; eng_tc6 h44a200; eng_tc6 h44e8d0;
    eng_tc8 h44f690; eng_tc6 h450810; eng_tc5 h44d2e0; eng_tc5 h456d30; eng_tc6 h461790;
    eng_tc3 h426440; eng_tc4 h445e50;
    int (ENG_THISCALL *skill_cond_5b5990)(wl_char *self, int32_t code, int32_t a3, void *a4, int32_t a5);
    /* the character apply pass 0x485fc0's callees (HP/MP, damage, respawn, voices, effects) */
    eng_tc3 h5b54d0; eng_tc3 h5b5580; eng_tc2 h487730; eng_tc3 h5b4d00; eng_tc5 h407ba0; eng_tc1 h41e180;
    eng_tc2 h4874b0; eng_tc1 h487570; eng_tc6 h4099a0; eng_tc3 h40a010; eng_tc1 h446400; eng_tc2 h5b5600;
    eng_tc2 h40d960; eng_tc0 h487530; eng_tc1 h487dc0;
    eng_tc0 h4878b0; eng_tc8 h476880; eng_tc0 h485f70;
    eng_tc6 h489280; eng_tc2 h487250; eng_tc0 h4895c0; eng_tc6 h41ef80;
    /* the tick driver 0x46cd70's band updaters */
    eng_tc4 h4710c0; eng_tc0 h46da00;
    eng_tc1 h54c640;
    int (ENG_STDCALL *h5567a0)(intptr_t a1, intptr_t box, intptr_t a3);
    int (ENG_STDCALL *h4593c0)(intptr_t body);
    int (ENG_STDCALL *h4872d0)(intptr_t a1, intptr_t body);
    /* the actor update 0x54f980's callees: the per-code behaviours, the looping-sound start,
     * list removal, the second error logger */
    eng_tc2 h408100; eng_tc3 h408b90; eng_tc1 h40bd90; eng_tc0 h40be00; eng_tc0 h41ea70; eng_tc5 h41ee60;
    eng_tc7 h450ef0; eng_tc4 h453960; eng_tc5 h54f140; eng_tc3 h552c70; eng_tc5 h552f60; eng_tc16 h5531b0;
    eng_tc3 h5534d0; eng_tc6 h553520; eng_tc7 h553ec0; eng_tc5 h554570; eng_tc6 h554c40; eng_tc3 h5556d0;
    eng_tc12 h555790; eng_tc2 h555e90; eng_tc0 h555fc0; eng_tc1 h556180; eng_tc2 h556380; eng_tc3 h5563d0;
    int (ENG_STDCALL *h556e50)(intptr_t owner, intptr_t id, intptr_t probe);
    /* the particle tick 0x46e510's callees */
    eng_tc7 h426db0;
    /* the behaviour AI 0x47b990's callees (+ the path follower 0x441f50 under 0x484c10, the error
     * logger 0x407f30 under 0x47e330) */
    eng_tc1 h47da60; eng_tc1 h482240; eng_tc1 h482460; eng_tc1 h482ac0;
    eng_tc2 h4060d0; eng_tc2 h47de50; eng_tc2 h47f670; eng_tc2 h47f9d0; eng_tc2 h481ac0; eng_tc2 h482e10; eng_tc2 h4834f0;
    eng_tc2 h484ea0; eng_tc2 h4854e0; eng_tc2 h485990; eng_tc2 h441f50;
    eng_tc3 h47d790; eng_tc3 h481310; eng_tc3 h482880; eng_tc3 h407f30;
    eng_tc4 h47b7c0; eng_tc4 h47e010; eng_tc4 h47eae0; eng_tc4 h483fb0;
    eng_tc5 h483de0; eng_tc5 h485350;
    eng_tc6 h483b50;
    eng_tc7 h482380; eng_tc7 h484530;
    eng_tc8 h47e7c0;
    eng_tc9 h47ecd0;
} engine_calls;

extern engine_calls g_eng;

#endif /* OSS_ENGINE_CALLS_H */
