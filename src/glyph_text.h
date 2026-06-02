/*
 * src/glyph_text.h — the cell-grid text/glyph layout builder.
 *
 * Ports the *build* half of the engine's dynamic-text system: turning a
 * (Shift-JIS) string into a cell's per-glyph layout records that the GDI
 * renderer (0x48e200, not yet ported) walks and TextOutA's.  See
 * `docs/findings/text-glyph-pipeline.md` for the whole-subsystem map.
 *
 * Ported functions (one per address):
 *   - FUN_0040fd20   glyph_token_search   (SJIS-aware substring search)
 *   - FUN_0040fa00   glyph_cell_layout    (string → cell.obj0 glyph records)
 *
 * The container these operate on is the SAME object menu_list.h already
 * models (menu_ctrl / menu_node): the descriptor at +0x174, the per-column
 * `entries` at +0x178, the `rows` at +0x17c, and each cell's `obj0` glyph
 * buffer.  This header therefore builds on menu_list.h rather than
 * re-declaring the container.
 *
 * Win32-free: pure structure manipulation, host-testable under the
 * sanitizer.  The only unported callee — the `#`-colour / control-code
 * escape expander (0x4034f0 / 0x4051d0, driven off the table at
 * 0x5cd978) — is routed through a nullable hook so the layout builder ports
 * and tests now; the NULL default is faithful for escape-free text.
 */
#ifndef OPENSUMMONERS_GLYPH_TEXT_H
#define OPENSUMMONERS_GLYPH_TEXT_H

#include <stdint.h>
#include "menu_list.h"

/* ─── glyph_buf — a cell's obj0 (0xc B, FUN_0040fa00 operator_new(0xc)) ──
 *
 * The buffer of per-glyph layout records the renderer reads.  `records` is
 * an owned operator_new(0xb40) = `cap`(0x50) × 0x24-byte glyph_record. */
typedef struct glyph_buf {
    void    *records;   /* +0x00 — cap × glyph_record (owned) */
    int32_t  cap;       /* +0x04 — record capacity (retail seeds 0x50 = 80) */
    uint16_t count;     /* +0x08 — number of glyph records built (glyph count) */
    uint16_t len;       /* +0x0a — source byte length (strlen of the input) */
} glyph_buf;

/* ─── glyph_record — one laid-out glyph (0x24 B) ────────────────────────
 *
 * `ch` is a NUL-terminated 1-byte (ASCII) or 2-byte (SJIS) fragment that the
 * GDI renderer passes straight to TextOutA.  `flag1c` marks a record the
 * escape expander has consumed (0 = raw glyph, draw it).  `color20` is a
 * per-glyph COLORREF the expander may set; the raw pass never touches it. */
typedef struct glyph_record {
    char     ch[3];        /* +0x00..+0x02 — glyph bytes + NUL */
    uint8_t  _pad03[0x19]; /* +0x03..+0x1b — expander scratch */
    uint16_t flag1c;       /* +0x1c — escape-consumed marker (0 = raw) */
    uint8_t  _pad1e[0x02]; /* +0x1e..+0x1f */
    int32_t  color20;      /* +0x20 — per-glyph colour (escape expander only) */
} glyph_record;            /* 0x24 B */

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(sizeof(glyph_buf)    == 0x0c, "glyph_buf must be 12 bytes");
_Static_assert(offsetof(glyph_buf, cap)   == 0x04, "glyph_buf.cap offset");
_Static_assert(offsetof(glyph_buf, count) == 0x08, "glyph_buf.count offset");
_Static_assert(offsetof(glyph_buf, len)   == 0x0a, "glyph_buf.len offset");
_Static_assert(sizeof(glyph_record) == 0x24, "glyph_record must be 36 bytes");
_Static_assert(offsetof(glyph_record, flag1c)  == 0x1c, "glyph_record.flag1c offset");
_Static_assert(offsetof(glyph_record, color20) == 0x20, "glyph_record.color20 offset");
#endif

/* The retail capacity an obj0 is seeded with (FUN_0040fa00: records =
 * operator_new(0xb40), cap = 0x50).  0xb40 == 0x50 * 0x24. */
#define GLYPH_BUF_CAP 0x50

/* ─── FUN_0040fd20 — SJIS-aware substring search ─────────────────────────
 *
 * Find `needle` in `hay` starting at byte offset `start`, advancing the scan
 * by 2 over SJIS lead bytes (`(signed char) < 0`).  Returns the byte offset
 * of the match, `start` for an empty needle, or -1 if not found.  Faithful
 * to 0x40fd20 (the compare is bounded so it never reads past `hay`'s NUL).
 * Used by the escape pass; the first chip of the expander. */
int32_t glyph_token_search(const char *hay, const char *needle, int32_t start);

/* ─── the unported escape-expansion pass (0x4034f0 / 0x4051d0) ─────
 *
 * The second half of FUN_0040fa00: walk the escape table at 0x5cd978 and
 * rewrite the just-built raw records to expand `#`-colour / ruby / control
 * sequences (per-glyph `color20`, collapsed/`flag1c`-marked records).  Its
 * own subsystem (a 7 KB switch + a 3 KB glyph-string copy); routed through
 * this hook until it lands.  Called once, after the raw pass, with the
 * cell's glyph buffer, the source string, and its byte length.  NULL by
 * default — faithful for escape-free ASCII/SJIS (no token matches → the
 * pass is a no-op), which covers every English menu label. */
typedef void (*glyph_escape_expand_fn)(glyph_buf *obj0, const char *str,
                                       uint16_t byte_len);
extern glyph_escape_expand_fn glyph_escape_expand_hook;

/* ─── FUN_0040fa00 — lay out a string into a grid cell's glyph buffer ────
 *
 * __thiscall(c, row, col, str).  NB the Ghidra decompile names the first two
 * args backwards: retail param_1 is the ROW, param_2 the COLUMN (recovered
 * from the caller 0x40f800, which passes (new_row, col_iter, text)).
 *
 *   1. Bounds: no-op unless `col < hdr->alloc_b` (column count) and
 *      `0 <= row < hdr->count` (current row count).
 *   2. Lazily operator_new the cell's obj0 glyph buffer if NULL
 *      (records = calloc(cap, 0x24), cap = GLYPH_BUF_CAP).
 *   3. Raw pass (no-op if strlen >= cap): split `str` into glyph records —
 *      one 2-byte record per SJIS lead byte, one 1-byte record per ASCII
 *      byte, each NUL-terminated with flag1c = 0.  Sets obj0->len = strlen,
 *      obj0->count = #records.
 *   4. Escape pass via glyph_escape_expand_hook (NULL default = no-op).
 *
 * Faithful to 0x40fa00 modulo the hooked escape pass and the calloc'd record
 * buffer (retail operator_new leaves it garbage; the renderer only reads
 * written fields). */
void glyph_cell_layout(menu_ctrl *c, int32_t row, int32_t col, const char *str);

#endif /* OPENSUMMONERS_GLYPH_TEXT_H */
