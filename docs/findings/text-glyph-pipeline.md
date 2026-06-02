# Text / glyph pipeline — the cell-grid text system

> The shared gate for every **dynamic-text** menu (new-game config, options,
> save/load) AND the prologue narration.  The title top-level menu does NOT
> use it — its labels are baked into the menu-bg sprite (quirk: see
> `title-scene.md`).  This doc maps the whole subsystem; the **build** half
> (`0x40fa00` layout + `0x40fd20` token search) is ported in
> `src/glyph_text.{c,h}` (checkpoint 34).  The **render** half (`0x48e200`)
> and the **escape-expanders** (`0x4034f0` / `0x4051d0`) are the next chips.

## The big realization — it's one object, already half-modelled

The container that `0x40fa00` builds into, that `0x40f800` appends rows to,
and that `0x48e200` renders is the **same `menu_ctrl` / `menu_node`** object
already modelled in `menu_list.h`.  The three functions are all `__thiscall`
on it (`in_ECX`).  The relevant overlaid views (see `menu_list.md`):

| off     | field (menu_list.h)     | role in the text system                       |
|---------|-------------------------|-----------------------------------------------|
| `+0x174`| `list` (`menu_list_hdr`)| descriptor: `alloc_a`(+4)=row cap, `alloc_b`(+8)=**column count**, `count`(+0x10)=**current row count** |
| `+0x178`| `entries` (`menu_entry`)| per-**column** metadata: `pos`(+0)=x, `+4`=y, `+0xc`=monospace flag, `+0x10`/`+0x14..` = per-cell colour overrides (render-time) |
| `+0x17c`| `rows` (`menu_row`)     | per-row records; `cells`(+0xc) → `menu_cell[col]` |
| (cell)  | `menu_cell.obj0` (+0x00)| **the glyph buffer** `0x40fa00` builds (`glyph_buf`) |
| (cell)  | `menu_cell.obj54/obj20` | secondary widgets `0x40f800` allocates (scroll/anim state) |
| `+0x180`..`+0x1ac` | `menu_node` display config | text/shadow COLORREFs the GDI renderer selects |

So porting text rendering is **not** a new container — it is (1) the glyph
buffer builder, (2) the row-append that wires the secondary widgets, and
(3) the GDI draw that walks rows×cols and `TextOutA`s each glyph.

## Text is rendered through Win32 GDI

`ar_register_fonts` (`FUN_00579bd0`, already ported in `asset_register.c`)
builds **8 GDI `HFONT`s** (`CreateFontIndirectA`, faces Courier New / Times /
Arial — see `ar_make_font`) into `g_ar_gdi_slots[1..8]`, plus two
font-**texture** sprite slots (ids 0x457/0x455, idx 42/43) for the
sprite-cell render path.  The renderer `0x48e200` `SelectObject`s an `HFONT`
into the back-buffer HDC and `TextOutA`s each glyph.  **The drop-in can call
real GDI** — it is a Windows program — so no rasteriser needs porting.

## Build path

### `glyph_buf` — the cell's obj0 (0xc B, `operator_new(0xc)`)

```
+0x00 records   void*   operator_new(0xb40) = 0x50 records × 0x24 B
+0x04 cap       int32   0x50 (80)
+0x08 count     u16     number of glyph records built (glyph count)
+0x0a len       u16     source byte length (strlen of the input)
```

### `glyph_record` — one laid-out glyph (0x24 B)

```
+0x00 ch[3]     1-2 byte glyph + NUL  (TextOutA reads it as a C string)
+0x1c flag1c    u16   escape-consumed marker (0 = raw glyph, render it)
+0x20 color20   i32   per-glyph colour (written only by the escape expander)
```
Other bytes are scratch the expander uses; the raw pass leaves them
untouched (retail: `operator_new` garbage — our port `calloc`s for host-test
determinism, an observable-equivalent divergence since the renderer only
reads `ch`/`count`/`flag1c`).

### `FUN_0040fa00` → `glyph_cell_layout(c, row, col, str)` (800 B) — PORTED

**Ghidra param names are swapped** (recovered from the caller `0x40f800:31`,
which passes `(new_row, col_iter, &text)`): retail `param_1` = **row**,
`param_2` = **col**, `param_3` = string.  The body:

1. bounds-check `col < alloc_b`, `row < count`, `row >= 0`; else no-op.
2. lazily `operator_new` the cell's `obj0` glyph buffer if NULL.
3. **raw pass** — split `str` into glyph records: each `signed char < 0` byte
   is an SJIS lead → emit a 2-byte record (`ch[0]=lead, ch[1]=trail`), else a
   1-byte ASCII record (`ch[0]=c, ch[1]=NUL`).  Every record gets `ch[k+1]=0`
   (NUL-term) and `flag1c=0`.  Sets `obj0->len = strlen`, `obj0->count =
   #records`.  Guarded by `strlen < cap` (over-long strings are dropped
   whole — retail has no truncation).
4. **escape pass** — walk the colour/control escape table `&DAT_005cd978`
   (stride 0xa4, NUL-name terminated); for each entry use `0x40fd20` to find
   the token in `str`, and on a hit expand it via `0x4051d0` (copy the
   replacement glyph string) / `0x4034f0` (the 7 KB switch that synthesises
   `#`-colour / ruby / control sequences), rewriting records + `color20` +
   `flag1c`.  **Routed through `glyph_escape_expand_hook` (NULL by default).**
   For escape-free ASCII/SJIS (every English menu label) no token matches and
   the pass is a no-op, so the NULL default is faithful — the raw pass alone
   produces exactly what the GDI renderer draws.

### `FUN_0040fd20` → `glyph_token_search(hay, needle, start)` (143 B) — PORTED

SJIS-aware substring search: find `needle` in `hay` at/after byte offset
`start`, advancing the scan by 2 over SJIS lead bytes; returns the match
offset, `start` for an empty needle, `-1` if absent.  Used only by the
escape pass; ported now as the first chip of the expander.

### `FUN_0040f800` → grid row-append (511 B) — NOT yet ported

Appends a row to the grid (`count++`), re-lays-out every existing column's
`obj0` (calls `0x40fa00`), and `operator_new`s each new cell's secondary
widgets: a `0x54` scroll-state object (+4) and a `0x20` anim object (+8,
with `+0x1c = max(+0x14, min(+0x18,0))`).  Belongs with the menu builders
(`0x411940` / `0x412160` / `0x566570`) — the next checkpoint.

## Render path — `FUN_0048e200` (1221 B) — NOT yet ported

`draw(mode, surf, hdc, x, y, hfont_main, hfont_shadow, blit_param)`.  Walks
`row = hdr.sel2 .. hdr.count` (clamped by `stride`) × `col = 0 .. alloc_b`:

- **GDI text mode** (`mode != 0`): pick `text`/`shadow` COLORREF from node
  config (`+0x180..`) by selection state (focused row, enabled, per-cell
  override), `SelectObject(hdc, hfont_main)`, then for each glyph record
  `TextOutA(hdc, x+7*i, y, record->ch, len)`.  A shadow pre-pass draws the
  glyphs offset by (+0,+1) and (+1,+0); the helpers `0x48e860` / `0x48e6d0`
  are the per-glyph `TextOutA` loops; `+0x14`/`+0x18` gate/offset the shadow.
  Monospace advance is **7 px/char** (hard-coded).
- **sprite-cell mode** (`mode == 0`): blits font-texture frames
  (`0x5b9b70` / `0x5bd550` keyed blit, `0x5b9bf0`) instead of `TextOutA` —
  used when the text layer is composited as sprites rather than GDI'd onto
  the DC.  Lazy-decodes via `0x4184a0` (the sprite decoder, ported).

The renderer needs a **real HDC** (the back-buffer DC) and the registered
`HFONT`s, so it lands with a Win32 adapter (like `title_sink`), not as pure
host code — its row/col walk + colour selection are host-testable with a
`TextOutA` recording stub.

## Verification plan (per the "render a known string, diff" gate)

1. Land the build half (this checkpoint) with host tests on the glyph records.
2. Port `0x48e200` GDI branch + `0x48e860`/`0x48e6d0`; install the registered
   font; render a known string into an offscreen DIB section DC.
3. Capture retail rendering the same string (a font-probe Frida hook on
   `0x48e200`, or just the new-game config menu once it builds) and
   `differ_px`-diff the glyph region — bit-exact is the bar.
4. Only then wire the new-game config scene (`0x564780` case 0x24).

## Files

- `docs/decompiled/by-address/40fa00.c` (build), `40fd20.c` (token search),
  `40f800.c` (row append), `48e200.c` / `48e860.c` / `48e6d0.c` (render),
  `4034f0.c` (7 KB escape switch), `4051d0.c` (3 KB glyph-string copy).
- `src/glyph_text.{c,h}`, `tests/test_glyph_text.c`.
- Container model: `src/menu_list.h` (`menu_ctrl`, `menu_node`, `menu_cell`).
- Font infra: `src/asset_register.{c,h}` (`ar_register_fonts`, `ar_make_font`).
