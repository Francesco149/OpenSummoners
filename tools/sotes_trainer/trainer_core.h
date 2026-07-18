// trainer_core.h — the in-process C API the Dear ImGui UI (trainer_ui.cpp) calls.
//
// trainer.c hosts the engine mechanics (find player / read map / teleport / load / ...) and a
// line-JSON socket (the LLM/MCP interface).  The ImGui UI is a SECOND front-end onto the SAME
// process: it links trainer.c and calls these typed accessors + actions directly (no socket
// round-trip).  Reads fill plain structs; blocking actions (load/newgame) must be called off
// the UI render thread.  All game addresses/mechanics live in trainer.c + SE_CODE_MAP.md.
#ifndef TRAINER_CORE_H
#define TRAINER_CORE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

// ── structured reads (fill *out; return 1 ok / 0 no data) ────────────────────
typedef struct {
    int      ok;
    uint32_t actor, stat_block;
    int      world_x, world_y;        // centi-px (px*100)
    int      hp, hp_max, mp, mp_max;
    int      combat_level_max, exp_cur, exp_max;
} tc_player;

typedef struct {
    uint32_t exit_key, target_room, return_key;
    int      hijacked;                // 1 = we changed this portal's destination
    uint32_t orig_target;             // its original destination (valid iff hijacked)
    int      slot;                    // exit-slot index (0..19) for hijack/revert
    int      has_door;                // 1 = a live door anchor was located for this exit
    int      door_x, door_y;          // world centi-px: door_x = anchor center X, door_y = feet
} tc_exit;

typedef struct {
    int      ok;
    uint32_t render_root, room_record;
    uint32_t room_key, area, scene, tileset, parallax;
    int      n_exits;
    tc_exit  exits[20];
} tc_map;

typedef struct { uint32_t key, area, scene; } tc_room;

typedef struct {
    int      slot, present, valid, party_count, level0;
    uint32_t handle, file_size;
    char     party0[24];              // first party member's name (empty if none)
} tc_save;

typedef struct {
    int      hooks, at_title, player_present;
    int      autoskip, mousefly, dlgskip, god, keepactive, attract, fastskip, warpgate;
    uint32_t delta, base, ti_mgr, pk_mgr, game_hwnd;
    int      cam_ok, cam_x, cam_y;    // camera top-left (centi-px) — the mouse-fly origin
} tc_status;

// a party member (for the character picker that chooses who teleport/mouse-fly/god operate on)
typedef struct {
    uint32_t code;                    // 0xc35a Arche / 0xc35b Sana / 0xc35c Stella
    char     name[12];
    int      active;                  // the CONTROLLED member (input chain -> live input mgr)
    int      is_target;               // the member the trainer currently operates on
    int      combat_level_max, world_x, world_y;
} tc_char;

int tc_get_player(tc_player *out);
int tc_get_map(tc_map *out);
int tc_get_status(tc_status *out);
int tc_get_saves(tc_save *out, int cap);   // returns count (reads user\savedataNN.sdt, no engine load)
int tc_get_rooms(tc_room *out, int cap);    // returns count (scans for the MASTER room table = all rooms)
int tc_get_chars(tc_char *out, int cap);    // returns count of present party members (marks active/target)

// which member teleport / mouse-fly / god / the reads operate on: 0 = the ACTIVE (controlled)
// member; else a specific party code.
void     tc_set_target(uint32_t code);
uint32_t tc_get_target(void);

// ── actions ──────────────────────────────────────────────────────────────────
void tc_teleport(int x_cpx, int y_cpx, int set_x, int set_y, int relative);
void tc_set_toggle(const char *name, int on);   // invincible/autoskip/mousefly/dlgskip/god/keepactive/attract/fastskip
int  tc_get_toggle(const char *name);
int  tc_load(int slot, char *msg, int msgcap);   // BLOCKING menu-drive (slot<0 = default/newest); 1 = loaded
int  tc_newgame(void);                           // BLOCKING; 1 = started
void tc_hijack_exit(int slot, uint32_t target);  // change portal `slot`'s destination room
void tc_revert_exit(int slot);                   // restore portal `slot`'s original destination
int  tc_teleport_to_door(int slot);              // teleport onto portal `slot`'s door anchor (1 = ok)
void tc_setstat(const char *which, int value, int lock);   // hp/hp_max/mp/mp_max/level
void     tc_set_cam_off(uint32_t off);           // live-tune the render_root->camera pointer offset
uint32_t tc_get_cam_off(void);

// ── UI thread entry (trainer_ui.cpp; spawned from DllMain) ───────────────────
void trainer_ui_start(void);

#ifdef __cplusplus
}
#endif
#endif
