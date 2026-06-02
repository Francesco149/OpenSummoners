# Parity ledger — frames/scenes confirmed pixel-1:1 vs retail

A running list of port renders that have been **visually diffed against the
retail golden and confirmed identical** (bit-identical, or identical modulo a
named, understood delta). This is a **regression guard**: once something is on
this list, later work must not silently break it. Re-verify entries with
`--capture-frames` + `tools/push_comparison.py` (port|retail amplified diff)
after changes that touch the relevant render path.

> How an entry earns its place: capture the port frame (`--capture-frames`),
> diff against the retail golden in `runs/<scenario>/frames/` via
> `tools/push_comparison.py --port … --retail …`, eyeball the amplified diff,
> and record the `differ_px` / `mean|abs|` numbers + any understood residual
> delta. "1:1" means the diff is empty OR fully explained by a listed,
> intended-to-be-fixed gap.

## Method / reference

- Port capture: `opensummoners(-debug).exe --capture-frames "N,…"` →
  `<gamedir>/port_frame_NNNNN.bmp` (Flip-indexed). Convert BMP→PNG (PIL).
- Retail goldens: `runs/title-idle/frames/frame_*.png` etc. (also Flip-indexed,
  captured by the Frida harness — see `docs/parity-harness.md`).
- Diff + push: `tools/push_comparison.py --port P.png --retail R.png …`.
- NB the port and retail **phase timelines differ** (the port rushes the intro;
  see HANDOFF "intro pacing"), so a port Flip N is compared against the retail
  Flip that shows the *same scene state*, not the same index. Record both.

> **The bar is `differ_px == 0`.** A frame is only "confirmed 1:1" when the
> amplified diff is empty. Any residual — however small — is an OPEN item below
> with concrete hypotheses, never hand-waved as "close enough" ([[feedback_bit_exact]]).

## Confirmed bit-exact (differ_px == 0)

| # | scene / frame | port cap | retail golden | verified |
|---|---------------|----------|---------------|----------|
| 1 | Title menu (idle, settled) — phase-matched | port Flip 209 `menu_fade=750` | `cursor-match/frame_01300` `local_58=750` | 2026-06-02 ckpt 28 — `differ_px=0` |
| 1b| Title menu (idle, settled) — phase-matched | port Flip 203 `menu_fade=450` | `cursor-match/frame_01420` & `_01460` `local_58=450` | 2026-06-02 ckpt 28 — `differ_px=0` |

> **R1 CLOSED (ckpt 28).** The residual was the **cursor pulse**. Retail
> animates the cursor `level_num` (`[esp+0x20]`) as a triangle wave — `local_58`
> in `FUN_0056aea0`, driven by phases 8↔9: phase 8 ramps `+50/update` to 1000,
> phase 9 ramps `-50/update` to 0 (`docs/decompiled/by-address/56aea0.c`
> :366-384). `level_div` is the constant `0x4b0` (1200), so `idx=(local_58*20)
> /1200` peaks at **16** (NOT 19) and breathes to 0 (invisible). The port had
> wired the cursor to a static idx-19 full-add, so it was uniformly **over-
> bright** (every differing pixel had port>retail). Method that nailed it (NOT
> eyeballing — RE'd the code + measured retail):
> 1. `frida_capture.py --cursor-probe` hooks `FUN_0056c470`, logs per-frame
>    `level_num`/`level_div` → showed the 0→1000→0 step-50 triangle + the call
>    site `0x56be79`.
> 2. Read `FUN_0056aea0`'s phase FSM → `local_58` is the source; the port
>    already computed it as `title_fade_state.menu_fade` but never passed it.
> 3. Wired `menu_fade`→cursor `level_num` (`title_render_menu`/`title_sink`),
>    captured port frames (which log `menu_fade`), matched to retail goldens
>    captured WITH `--cursor-probe` (so each golden's `local_58` is known), and
>    diffed at equal pulse phase → `differ_px=0`.
>
> The port Flip index ≠ retail Flip index (the port still rushes the intro —
> that is R3 below, the *separate* pacing problem). But at equal pulse phase the
> frames are pixel-identical, which proves the render path is 100% correct.

## Open residuals — under investigation (NOT yet 1:1)

_(R1 moved to Confirmed bit-exact above.)_

## Other rendered-but-not-1:1 gaps (arms not yet wired)

- **Selection sparkle** (arcs beside the cursor) — `SPARKLE` sink arm. The alpha
  ramps are now populated (ckpt 27), so this is wirable; it carries an explicit
  blend-descriptor pointer per command (see `title_sink.h`).
- **Lizsoft studio splash** + **intro fade pacing** — `LOGO` arm + the stubbed
  pace pump `0x5b1030`. The port doesn't render the studio splash and rushes the
  intro; frame-for-frame intro parity (and likely R1's phase alignment) needs
  the pacing ported.

When a residual reaches `differ_px == 0`, move it to "Confirmed bit-exact".
