/*
 * scene_cast.{c,h} — the cutscene CAST step on the retail layout (rhost-verified inside retail): what a
 * character the scene runner 0x439690 holds in its cast does each frame — walk to a point (path search over
 * the grid's walk / ladder cells + link cells, then the NPC navigator per step; teleport there when it takes
 * too long or no path exists), turn to a facing, wait out its action, play a stance / pose skill.
 *
 *   FUN_0043e5b0  cast_step           FUN_0043ef20  path_search         FUN_0043f370  path_dfs
 *   FUN_00441f00  body_near_x         FUN_00442600  cast_face_cmd
 *
 * The cast command (set by the scene commands, script_cast_cmd): char +0x15b64 the type (1 walk to, 2 face,
 * 3 wait for the action, 4 stance, 5 pose skill), u16 +0x15b68 its step, u16 +0x15b6a a repeat count, +0x15b6c
 * / +0x15b70 the target x / y, +0x15b74 the final facing (99 any), +0x15b78 no-run, +0x15b7c / +0x15b80 /
 * +0x15b84 the teleport point (when +0x15b7c: else the target), +0x15b88 the time limit (frames; 0 none),
 * +0x15b8c its clock, +0x15b90 the teleported phase, +0x15b94 the pose repeats, +0x15b98 / +0x15ba0 / +0x15ba4
 * the skill / its record / its argument, +0x15b9c wait for the ground first.  The path (char +0x142a0: up to 64
 * 20-byte steps {u16 kind 0 walk / 1 link, next cell, this cell}, u16 +0x1429c count, u16 +0x147a0 index,
 * +0x147a4 / +0x147a8 the goal).  Returns 1 when the command is done (its key command then cleared).
 *
 * The path finder (world+0x1050, 0xa52c B): a depth-first search from the body's feet cell to the goal cell,
 * horizontal moves (a ladder / slope cell steps the row up or down) and the link cells' jumps (+4 cost), every
 * found path shrinking the cost bound so the search ends on a cheapest one; no path: the cell it got closest to
 * is searched once more.  Consecutive walk steps in one direction are merged.
 */
#ifndef OSS_SCENE_CAST_H
#define OSS_SCENE_CAST_H

#include <stdint.h>

#include "world_layout.h"

typedef struct path_rec {
    uint16_t kind;            /* +0x00  0 walk, 1 a link cell's jump                          */
    uint16_t _p02;
    int32_t  nc, nr;          /* +0x04  the next cell                                        */
    int32_t  c, r;            /* +0x0c  this cell                                            */
} path_rec;

typedef struct path_finder {
    uint8_t  *grid;           /* +0x0000 */
    uint16_t  seen[160 * 128];/* +0x0004  the cost each cell was entered at (col + 160 row)    */
    int32_t   tx, ty;         /* +0xa004  the goal cell                                       */
    int32_t   best_c, best_r; /* +0xa00c  the closest cell reached                            */
    int32_t   best;           /* +0xa014  its distance (-1: none yet)                         */
    int32_t   links;          /* +0xa018  follow the link cells                               */
    int32_t   check_c;        /* +0xa01c  refuse cells whose feet row is region-C solid       */
    uint16_t  len;            /* +0xa020  the found path's length                             */
    uint16_t  _pa022;
    path_rec  rec[64];        /* +0xa024 */
    int32_t   wcells;         /* +0xa524  the body's width in cells                           */
    int32_t   h;              /* +0xa528  the body's height                                   */
} path_finder;

/* |x - body x| inside 1600 (bodies over 4000 wide: half the width + 1600) */
int body_near_x(const wl_body *b, int32_t x);                                          /* 0x441f00 */
/* facing `dir` (99: any) reached; else the walk key toward it into cmd->lr */
int cast_face_cmd(const wl_body *b, int32_t dir, wl_cmd *cmd);                         /* 0x442600 */
/* the search: the steps into out[] (*count), returns whether the goal was reached */
int path_search(path_finder *pf, int32_t x, int32_t y, int32_t w, int32_t h, int32_t tx, int32_t ty, int32_t links,
                int32_t maxd, uint8_t *grid, uint16_t *count, path_rec *out, int32_t check_c); /* 0x43ef20 */
int path_dfs(path_finder *pf, int32_t c, int32_t r, int32_t depth, uint16_t *len, int32_t cost, int32_t bound,
             uint16_t *cost_out);                                                      /* 0x43f370 */
/* one frame of the character's cast command; 1 = done */
int cast_step(wl_char *c);                                                             /* 0x43e5b0 */

#endif /* OSS_SCENE_CAST_H */
