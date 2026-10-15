/*
 * scene_loop.{c,h} — the SCENE LOOP 0x59f2c0: a game session from the title (new game, or a save
 * loaded) through every room (retail layout; rhost-verified inside retail: the loop in REPLACE by
 * the corpus hash, its helpers per call).
 *
 * The session object (`ss`: +0 the room loop object, +4..+0xc three KEEP lists (0x88 B: +0 map
 * id, +4 up to 32 NPC ids, u16 +0x84 count, u16 +0x86 age), +0x10 the AREA table (0x5400c B:
 * +0 the 0x693848 map table, 336-byte areas from +4, u16 count +0x54004), +0x14 the persistent
 * FILL table (0x7808 B: 512 event flags {id, value, -} count +0x1800, 2048 actor states {handle,
 * state, map} count +0x7804)).  The party ROOM record (0x4120 B, 8 slots of 0xeec) is made here.
 * Start: the area table is copied from 0x6940c8 and cross-referenced (0x585000: the map table's
 * defaults, the reverse zone links); flags 100100101 (cleared) / 100100102 (+4 = 20).  New game:
 * the slots reset, the room's defaults, the start area (+0x4104), the area key / room setup
 * (0x4c5350 / 0x4e59a0).  A save (`save` = its id): the 0x25c-byte header (magic 10001 + the id
 * at [139] / [140], section sizes) and its sections into the scene object, then 0x561d20 applies
 * it.  Then the room-entry pipeline (0x55f790, the save 0x41a890 10020; 10020 with `demo`: the
 * demo scene's flags / states; 10030: every mapped actor state dropped), one input manager, and
 * the ROOM LOOP: the area (+0x4024) -> its record, flag set, the keep list (dropped when older
 * than 2 rooms), the leader's input, a new room loop object, load_room; by its result: 1 / 2 /
 * 3 / 10 = the next area from the room loop's +0x900; 4 / 5 = the retry point (+0x40d0, when the
 * room has one: the party healed, 10% gold lost, `*out` = 1), the defeat counter +0x40b0, the
 * defeat scene (0x59cfc0 + save 10030); the keep snapshot, the room cleanup (every entity
 * released, the room loop freed, resource group 3 unloaded); 1 / 2 / 10 loop (a map change drops
 * the mapped actor states), 3 = the menu 0x5a00c0 (6 ends), else end.  End: 0x562ba0 (the world,
 * the room loop, the keep lists, group 2 unloaded, the tables) and the room deleted.
 *
 *   FUN_0059f2c0  scene_loop           FUN_0041a7f0  scene_obj_init
 *   FUN_004c57f0  flag_find            FUN_004c5830  flag_add
 *   FUN_00560e90  room_flag_set        FUN_00560e60  pslot_reset
 *   FUN_005612b0  room_party_defaults  FUN_005611d0  room_inputs_alloc
 *   FUN_00561280  input_ctor           FUN_004017a0  input_dtor
 *   FUN_005614e0  room_delete          FUN_00561c90  area_find
 *   FUN_005851e0  input_init           FUN_0059e1a0  room_area_enter
 *   FUN_005a3bb0  fill_clear_mapped    FUN_005a3bf0  room_has_retry
 *   FUN_005a3c10  room_retry_point     FUN_005a3de0  keep_find
 *   FUN_004e61a0  room_next_set        FUN_004e1950  room_party_heal
 *   FUN_00419c00  room_gold_add        FUN_005a3c40  roomloop_new
 *   FUN_00561150  room_from_scene      FUN_005a4460  obj_delete
 *   FUN_005a4480  obj_free             FUN_004cc5a0  pslot_input_set
 *   FUN_005a3e20  keep_snapshot        FUN_005a3ff0  room_end_cleanup
 *   FUN_005a4200  roomloop_pools_free  FUN_00585f00  roomloop_free
 *   FUN_00562ba0  session_end          FUN_00585000  area_xref
 *   FUN_00562a70  frame_cache_clear    FUN_005b6300  file_read_section
 *   FUN_00562e60  pool_obj_delete      FUN_00562e40  world_delete
 *   FUN_0055f790  room_to_scene
 */
#ifndef OSS_SCENE_LOOP_H
#define OSS_SCENE_LOOP_H

#include <stdint.h>

#include "world_layout.h"

/* 0x59f2c0 (thiscall session, 5 args): the result of the last room / menu */
int32_t scene_loop(uint8_t *ss, int32_t save, uint16_t sub, int32_t demo, int32_t *out, int32_t start);
uint8_t *scene_obj_init(uint8_t *o);                                 /* 0x41a7f0: the 0xe290 scene object */
uint8_t *flag_find(uint8_t *t, int32_t id);                          /* 0x4c57f0 */
uint8_t *flag_add(uint8_t *t, int32_t id);                           /* 0x4c5830: the LAST free, else append */
uint8_t *room_flag_set(wl_room *r, int32_t id, int32_t state);       /* 0x560e90 */
void pslot_reset(wl_pslot *s);                                       /* 0x560e60 */
void room_party_defaults(wl_room *r);                                /* 0x5612b0 */
void room_inputs_alloc(wl_room *r, uint16_t n);                      /* 0x5611d0 */
void input_ctor(uint8_t *in);                                        /* 0x561280 */
void input_dtor(uint8_t *in);                                        /* 0x4017a0 */
void room_delete(wl_room *r, int32_t flags);                         /* 0x5614e0 */
uint8_t *area_find(uint8_t *areas, int32_t id);                      /* 0x561c90 */
uint8_t *input_init(wl_room *r, uint16_t i, int32_t a, int32_t b, int32_t c); /* 0x5851e0 */
void room_area_enter(int32_t *key, int32_t mode, wl_room *r);        /* 0x59e1a0 (`key` = {mode, room}) */
void fill_clear_mapped(uint8_t *fill);                               /* 0x5a3bb0 */
int room_has_retry(const wl_room *r);                                /* 0x5a3bf0 */
void room_retry_point(wl_room *r);                                   /* 0x5a3c10 */
uint8_t *keep_find(uint8_t *ss, int32_t map);                        /* 0x5a3de0 */
void room_next_set(wl_room *r, int32_t area, int32_t gate, int32_t a7); /* 0x4e61a0 */
void room_party_heal(wl_room *r, int32_t all);                       /* 0x4e1950 */
int room_gold_add(wl_room *r, int32_t n, int32_t count);             /* 0x419c00 */
wl_roomloop *roomloop_new(wl_roomloop *rl);                          /* 0x5a3c40 (the 0x910 object) */
void room_from_scene(wl_room *r, const uint8_t *so);                 /* 0x561150 */
void obj_delete(uint8_t *o, int32_t flags);                          /* 0x5a4460 */
void obj_free(uint8_t *o);                                           /* 0x5a4480 */
int pslot_input_set(wl_pslot *s, void *in);                          /* 0x4cc5a0 */
void keep_snapshot(uint8_t *ss, wl_roomloop *rl);                    /* 0x5a3e20 */
void room_end_cleanup(uint8_t *ss);                                  /* 0x5a3ff0 */
void roomloop_pools_free(wl_roomloop *rl);                           /* 0x5a4200 */
void roomloop_free(wl_roomloop *rl);                                 /* 0x585f00 */
void session_end(uint8_t *ss);                                       /* 0x562ba0 */
void area_xref(uint8_t *areas, const uint8_t *rows, uint8_t *rec);   /* 0x585000 */
void frame_cache_clear(uint8_t *c);                                  /* 0x562a70 */
int file_read_section(void *rd, void *dst, int32_t n, uint32_t cap); /* 0x5b6300 */
void pool_obj_delete(uint8_t *p, int32_t flags);                     /* 0x562e60 */
void world_delete(wl_world *w, int32_t flags);                       /* 0x562e40 */
/* 0x55f790: the room record -> the scene object (what a save stores): the room fields, the play
 * time (+0x7c = (now - +0x4068) / 1000 + +0x406c), the items (+0x2010, 16 B), each slot's member
 * (+0x20a4 + 0x11c x k: id, code, leader, stats, skills, ...; its 12-byte skill records and the
 * 16-byte equipment marks of 0x628708's 18 rows at +0x2984 + 0x420 x k), the fill table, the flags */
void room_to_scene(const wl_room *r, uint8_t *so);

#endif /* OSS_SCENE_LOOP_H */
