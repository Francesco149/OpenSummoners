/* base_stat_table.h — the retail character BASE-STAT table DAT_0067ac58.
 *
 * The table data is GENERATED into base_stat_table.c by tools/dump_stat_table.py
 * (like portrait_face_data.c / world_tables_data.c).  FUN_00426fd0 looks it up by
 * (code, level) to fill a member's +0x750 stat block; the freeroam HUD leader
 * panel (FUN_00494e60; src/hud.c + main.c game_render_hud) reads HP / MP / level /
 * EXP from the result — pure table lookups, not computed.  The (code,level)->row
 * select lives in party.c (base_stat_find), the SAME split as the dramatist table.
 */
#ifndef OSS_BASE_STAT_TABLE_H
#define OSS_BASE_STAT_TABLE_H

#include <stdint.h>
#include <stddef.h>

/* One row of DAT_0067ac58 — the columns FUN_00426fd0 consumes (retail stride is
 * 0xdc bytes; only the fields the init reads are kept). */
typedef struct base_stat_row {
    uint32_t code;      /* +0x00 — character code (key)                       */
    int32_t  level;     /* +0x04 — row[1] (the row-selector key) -> stats +0xe0
                         *   combat_level_max (the MAX COMBAT LEVEL / star max — not
                         *   the display Lv, which is +0xe0 + level_bonus)          */
    int32_t  hp;        /* +0x08 — row[2]  -> stats +0x58 max & +0x54 cur     */
    int32_t  mp;        /* +0x0c — row[3]  -> stats +0x60 max & +0x5c cur     */
    int32_t  stat[4];   /* +0x10..+0x1c — row[4..7] -> stats +0x64/68/6c/70   */
    int32_t  exp_max;   /* +0x20 — row[8]  -> stats +0xec                     */
} base_stat_row;

extern const base_stat_row BASE_STAT_TABLE[];
extern const size_t        BASE_STAT_TABLE_COUNT;

/* FUN_00426fd0's row select: the first row with code==`code` AND (row.level==
 * `level` OR level==0), else the fallback row 0 (retail's `&DAT_0067ac58`).
 * Never NULL.  `level`==0 picks the character's lowest matching row. */
const base_stat_row *base_stat_find(uint32_t code, int level);

#endif /* OSS_BASE_STAT_TABLE_H */
