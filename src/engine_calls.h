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
typedef int (ENG_THISCALL *eng_tc10)(void *self, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t,
                                     intptr_t, intptr_t, intptr_t);
typedef int (ENG_THISCALL *eng_tc12)(void *self, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t,
                                     intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
typedef int (ENG_THISCALL *eng_tc19)(void *self, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t,
                                     intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t,
                                     intptr_t, intptr_t, intptr_t, intptr_t);
typedef int (ENG_THISCALL *eng_tc14)(void *self, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t,
                                     intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
typedef int (ENG_THISCALL *eng_tc25)(void *self, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t,
                                     intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t,
                                     intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t,
                                     intptr_t, intptr_t);
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
    int (ENG_THISCALL *integrate_442a70)(wl_char *self, wl_cmd *cmd, wl_body *in, wl_body *out,
                                         int32_t mode, int32_t n);
    int (ENG_STDCALL  *party_slot_ok_4071f0)(wl_party *p, int32_t mode, int32_t slot);
    int (ENG_THISCALL *skill_479f90)(wl_char *self, wl_body *b, int32_t a3);
    int (ENG_THISCALL *skill_47a300)(wl_char *self, int32_t a2, int32_t a3);
    int (ENG_THISCALL *skill_47a420)(wl_char *self, int32_t a2, int32_t a3);
    int (ENG_THISCALL *skill_47a7f0)(wl_char *self, int32_t id, uint32_t now, int32_t slot);
    int (ENG_THISCALL *target_ok_47adb0)(wl_char *cand, wl_char *self, int32_t spell, int32_t a4,
                                         int32_t a5, int32_t a6, int32_t a7, int32_t a8);
    /* the DirectSound boundary (0x5bb870.. take a buffer wrapper; create / duplicate take the device) */
    int (ENG_THISCALL *dsb_create_5bb250)(void *dev, intptr_t voices, intptr_t module, intptr_t res, intptr_t a5,
                                         intptr_t a6);
    int (ENG_THISCALL *dsb_dup_5bb2f0)(void *dev, intptr_t dst, intptr_t src);
    int (ENG_THISCALL *dsb_volume_5bb870)(void *buf, int32_t v);
    int (ENG_THISCALL *dsb_pan_5bb880)(void *buf, int32_t v);
    int (ENG_THISCALL *dsb_freq_5bb890)(void *buf, int32_t v);
    int (ENG_THISCALL *dsb_play_5bb8c0)(void *buf);
    int (ENG_THISCALL *dsb_stop_5bb8e0)(void *buf);
    int (ENG_THISCALL *log_fail_406440)(void *log, const char *msg, const char *tail);
    /* the DirectSound buffer release 0x5bb3a0 (COM Release + the device's buffer count) */
    eng_tc0 dsb_release_5bb3a0;
    /* KERNEL32 / GDI32 through retail's import table (OutputDebugStringA, DeleteObject) */
    void (ENG_STDCALL *dbg_out)(const char *s);
    int  (ENG_STDCALL *gdi_delete)(void *obj);
    /* the DirectInput boundary: keyboard state 0x5ba4c0 / buffered data 0x5ba2d0, joystick
     * poll 0x5ba650 / read 0x49bf90 */
    eng_tc0 h5ba4c0; eng_tc0 h5ba2d0; eng_tc0 h5ba650; eng_tc1 h49bf90;
    /* the gimmick pass's callees: the event-script engine 0x40c380, the shot emitter 0x40ac90,
     * the shot spawner 0x55cfc0 */
    eng_tc0 h40c380; eng_tc6 h40ac90; eng_tc19 h55cfc0;
    /* the touch pass's: the input re-poll 0x40fe00 (`this` = sys), the key use 0x54ed80 */
    eng_tc0 h40fe00; eng_tc0 h54ed80;
    /* the room loop's: the joystick input producer 0x468b10, the BGM manager update 0x5bc0c0 */
    eng_tc2 h468b10; eng_tc0 h5bc0c0;
    /* the encounter pass 0x58e780's: the battle voice line 0x409520 (`this` = the room), the BGM
     * change 0x40a5d0 (`this` = sys) */
    eng_tc9 h409520; eng_tc5 h40a5d0;
    /* the room loop 0x58f360's: the message pump 0x5b1030, the render 0x48c150, the text draw
     * 0x495e40, 0x59d160, Flip 0x5b8fc0 (`this` = the ZDD) and the camera easer 0x43d1d0 (rhost
     * binds these two to its own call stubs: the virtual clock / the tick count), the leader swap
     * 0x59aa40, the pause switch 0x58ffa0, the party / item menus 0x590480 / 0x590f20 / 0x590ca0,
     * the main menu 0x59d540 / 0x591120 / 0x562a70 / 0x58ffc0 / 0x59a910 / 0x415350, the shots
     * 0x590230, the warp 0x59c750, the pickups 0x59ca90, the scene runner 0x59ad60, 0x5908a0,
     * the screen effects 0x499ab0, a part's facing 0x4842c0, the
     * scene object 0x41a7f0 / start 0x59cfc0 */
    void (*h5b1030)(void);
    eng_tc3 h48c150; eng_tc7 h495e40; eng_tc0 h59d160; eng_tc1 flip_5b8fc0; eng_tc0 easer_43d1d0;
    eng_tc3 h59aa40; eng_tc1 h58ffa0; eng_tc0 h590480; eng_tc1 h590f20; eng_tc0 h590ca0;
    eng_tc2 h59d540; eng_tc0 h591120; eng_tc0 h562a70; eng_tc0 h58ffc0; eng_tc0 h59a910; eng_tc0 h415350;
    eng_tc0 h590230; eng_tc1 h59c750; int (ENG_STDCALL *h59ca90)(void *room); eng_tc1 h59ad60;
    eng_tc0 h5908a0; eng_tc0 h499ab0; eng_tc1 h4842c0; eng_tc0 h41a7f0;
    int (ENG_STDCALL *h59cfc0)(void *scene, void *room, intptr_t zone, intptr_t a4);
    /* the integrator 0x442a70's callees: per-type behaviour handlers + helpers (by VA) */
    eng_tc3 h467260; eng_tc6 h44a740; eng_tc8 h45a300;
    eng_tc4 h44bc10; eng_tc4 h44c250; eng_tc4 h44bf60; eng_tc7 h446900; eng_tc7 h452020;
    eng_tc7 h449810; eng_tc6 h4539b0; eng_tc7 h44d8f0; eng_tc7 h448dc0; eng_tc5 h452730;
    eng_tc4 h457ff0; eng_tc5 h45e830; eng_tc7 h455d80; eng_tc6 h44b160;
    eng_tc7 h4516e0; eng_tc7 h4484b0; eng_tc6 h4512d0; eng_tc6 h44a200; eng_tc6 h44e8d0;
    eng_tc8 h44f690; eng_tc6 h450810; eng_tc5 h44d2e0; eng_tc5 h456d30; eng_tc6 h461790;
    eng_tc4 h445e50;
    /* the character apply pass 0x485fc0's callees (HP/MP, damage, respawn, voices, effects) */
    eng_tc3 h5b4d00; 
    eng_tc2 h5b5600;
    eng_tc1 h487dc0;
    eng_tc0 h4878b0; 
    eng_tc0 h4895c0; 
    /* the tick driver 0x46cd70's band updaters */
    eng_tc4 h4710c0; eng_tc0 h46da00;
    /* the actor update 0x54f980's callees: the per-code behaviours, the looping-sound start,
     * list removal, the second error logger */
    eng_tc3 h408b90;
    eng_tc5 h54f140; eng_tc3 h552c70; eng_tc5 h552f60; 
    eng_tc3 h5534d0; eng_tc6 h553520; eng_tc7 h553ec0; eng_tc5 h554570; eng_tc6 h554c40; eng_tc3 h5556d0;
    
    int (ENG_STDCALL *h556e50)(intptr_t owner, intptr_t id, intptr_t probe);
    /* the cached-frame release 0x5b9390 (DirectDraw boundary; under the grid refresh 0x54c640) */
    eng_tc0 h5b9390;
    /* the behaviour AI 0x47b990's callees (+ the path follower 0x441f50 under 0x484c10, the error
     * logger 0x407f30 under 0x47e330) */
    eng_tc1 h47da60; eng_tc1 h482240; eng_tc1 h482460; eng_tc1 h482ac0;
    eng_tc2 h47de50; eng_tc2 h47f670; eng_tc2 h47f9d0; eng_tc2 h481ac0; eng_tc2 h482e10; eng_tc2 h4834f0;
    eng_tc2 h484ea0; eng_tc2 h4854e0; eng_tc2 h485990; eng_tc2 h441f50;
    eng_tc3 h47d790; eng_tc3 h481310; eng_tc3 h482880; eng_tc3 h407f30;
    eng_tc4 h47e010; eng_tc4 h47eae0; eng_tc4 h483fb0;
    eng_tc5 h483de0; eng_tc5 h485350;
    eng_tc6 h483b50;
    eng_tc7 h482380; eng_tc7 h484530;
    eng_tc8 h47e7c0;
    eng_tc9 h47ecd0;
    /* the room setup 0x586010's roots: the world pointer 0x8a9b50 (it deletes and re-creates the
     * world), the loading-log flag 0x8a6b54 (0 = every setup step is logged; set after the first
     * room loop), the loading log 0x8a6620 (0x406440 / 0x408b90 `this`) */
    wl_world  **world;
    int32_t    *quiet;
    void       *dlog;
    /* ... and its callees: the joystick create 0x5ba120 / acquire 0x5ba290 (DirectInput), the map
     * the room population's inits (character 0x41f200, device 0x431e30), the
     * map sprite's palette read 0x4178e0 (the bitmap resource)
     * and frames 0x418470 (DirectDraw surfaces), the parallax grade 0x417c40, the party
     * member create 0x58e170, 0x56dc20, the scene runner's stop 0x41a890, the pool-head record
     * clear 0x40e0c0; the fatal paths: the sprintf 0x5bf3ee, the error box 0x426110, the log reset
     * 0x560900, exit 0x5bf5db; KERNEL32's resource calls (the map resource reader) */
    eng_tc3 h5ba120; eng_tc0 h5ba290;
    eng_tc14 h41f200; eng_tc25 h431e30;
    eng_tc1 h4178e0; eng_tc1 h418470; eng_tc2 h417c40;
    eng_tc5 h58e170; eng_tc3 h56dc20;
    void *(ENG_STDCALL *find_res)(void *mod, const char *name, const char *type);
    void *(ENG_STDCALL *load_res)(void *mod, void *res);
    void *(ENG_STDCALL *lock_res)(void *glob);
    int32_t *tod;                         /* &0x8a93fc, the time of day (the palette tint's selector) */
    eng_tc4 h41a890; eng_tc0 h40e0c0;
    int (*h5bf3ee)(char *buf, const char *fmt, ...);
    eng_tc3 h426110; eng_tc0 h560900;
    void (*h5bf5db)(int code);
} engine_calls;

extern engine_calls g_eng;

#endif /* OSS_ENGINE_CALLS_H */
