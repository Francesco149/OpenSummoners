# Dialogue body-text row distribution (the line-count-dependent vertical spacing)

**RE'd 2026-06-20** (USER: the town dialogue body spacing is wrong — a 3-line line
shows only 2 lines on port + the spacing is too tall; "RE the exact logic, don't
hardcode from empirical data; it's nuanced logic with cases like the box sizing").

## The bug

The port rendered body rows at a CONSTANT pitch: `box_y + TEXT_DY + r*LINE_H`.
ckpt d16ae1a had set `TEXT_DY=29`, `LINE_H=36` — fitted to the **2-line** case at
tick 717.  That over-spaces the **3-line** lines (retail packs them tighter), so the
3rd row of "We haven't been here since..." (tick ~770) fell out of the box → the port
showed only 2 lines.  The real engine does NOT use a constant pitch: it **vertically
distributes** the rows inside a fixed 3-row grid, so fewer rows ⇒ a larger gap.

## The exact logic (`FUN_0048da70` @ 0x48da70 — the grid text renderer)

The per-row baseline Y (decompile, `48da70.c:83` + `:101`):

```
local_2c (gap):
    g0 = records[+0x1c]                      # the grid's max-gap (see below)
    if g0 == 0 or rows_used == 0: gap = 0
    else:
        cand = ((max_rows - rows_used) * pitch) / (rows_used + 1)   # int div
        gap  = min(g0, cand)
Y(r) = (r+1)*gap + pitch*r + base_y + box_screen_y                 # :101
```

where, for the **dialogue body grid** (the portrait speech bubble, builder
`0x439690` else-branch ~:395-514, grid built by `FUN_0040dee0(0x88,0x14,0x24,3)`):

| field           | value | source |
|-----------------|-------|--------|
| `pitch` (+0x1ac)| **28**| the grid line pitch (font metric).  No static writer sets +0x1ac=0x1c; it is RE-CONFIRMED by the formula's internal consistency — the SAME value is the 3-row pitch (28) AND the 2-row gap candidate `(1·28)/3=9`; only 28 fits both. |
| `base_y` (+0x10)| **20**| `FUN_0040df40` param_2 = builder `local_2cc=0x14`. |
| `max_rows`(R+0xa)| **3** | `FUN_0040df40` param_4 = builder `local_22c=3`. |
| `max_gap` (R+0x1c)| **20**| `FUN_00410610:19` sets `*(grid+0x170+0x1c)=0x14` UNCONDITIONALLY (called at builder :469, right after the grid is built).  (The grid CTOR `FUN_0040df40:62` zeroes it; 00410610 then raises it to 0x14.) |

`rows_used` = the max row index +1 over **ALL** grid records (`48da70.c:63-73`,
counted over the total record set `piVar6[3]+8`), i.e. the line's **TOTAL** row
count — NOT the typewriter-revealed count.  So the layout is fixed when the text is
set; the reveal types into a pre-laid-out grid (proven: tick 661 shows only 'A' of a
3-row line, already at the 3-row offset 20, not the 1-row offset 40).

`box_screen_y` is the box corner (the port's `box_y` from `dialogue_box_position`).

## The three cases (max_rows=3, pitch=28, max_gap=20)

| rows | cand=((3-rows)*28)/(rows+1) | gap=min(20,cand) | row-0 off=20+gap | pitch=28+gap |
|------|----------------------------|------------------|------------------|--------------|
| 1    | 56/2 = 28                  | **20**           | **40**           | —            |
| 2    | 28/3 = 9                   | **9**            | **29**           | **37**       |
| 3    | 0/4 = 0                    | **0**            | **20**           | **28**       |

## VERIFICATION (retail.osr TEXT records, `tools/trace_studio2/dlg_text_probe.py`)

box_y recovered as `name_baseline + 9` (NAME_DY=-9); body-row baselines are the
TextOutA y's whose y+1 is also present (main + the y+1 shadow pass).

- 3-row (tick 762/780 "We.../a.../b..."): rel_off **[20,48,76]**, pitch **[28,28]** ✓
- 2-row (tick 717 "Yay.../Tonk..."): rel_off **[29,66]**, pitch **[37]** ✓
- 1-row genuine (tick 852 'I', 948 'C', 1149 'M'): rel_off **[40]** ✓

All three branches reproduce retail's measured offsets EXACTLY — the formula +
constants are RE'd from the decompile and the data only confirms them.

## Port

`dialogue.{c,h}`: `DIALOGUE_TEXT_DY` 29→**20**, `DIALOGUE_LINE_H` 36→**28**, new
`DIALOGUE_GRID_MAX_GAP`=20; `dialogue_body_gap()` / `dialogue_body_row_dy()` compute
the distribution.  `main.c game_render_dialogue` uses `dialogue_body_row_dy(d,r)`.

## Residual (NOT this fix — a separate, pre-existing box-fill recon nuance)

The tick-780 montage shows a faint frame-shaped diff over the text.  `draw_probe` at
that tick (region 280,180..430,235) shows it is NOT a missing graphic and NOT the
spacing: the dialogue box-content tiles draw at the SAME dst rects + seq order on both
sides, but retail attributes them to **`res=0`** (a backbuffer-region composite the
proxy can't name) while the port draws **`res=1110 fr=4/7`** (its box-fill bank).  The
"framed box" is a RECON-ORDERING artifact of retail's res=0 tiles vs the GDI text in
the reconstruction (the on-screen parchment box is the same).  This box-fill
composite-source difference is pre-existing, affects every dialogue frame equally, and
is unrelated to the row distribution.  (My commit `a91696f` loosely called it the
arrow-art debt — this is the accurate characterization.)  A real follow-up only if the
box FILL itself is later found to diverge on a live frame, not in recon.
