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

## Confirmed entries

| # | scene / frame | port cap | retail golden | diff | residual delta (understood) | verified |
|---|---------------|----------|---------------|------|------------------------------|----------|
| 1 | **Title menu — idle, settled** (logo + character art + "Secret of the Elemental Stone" + Start/Continue/Bonus Menu/Options/Exit + copyright) | Flip 200 | `title-idle` Flip 1900 | 2964/307200 px (1.0%), mean\|abs\|/ch 0.09 | **only** the menu-selection indicator at the "Start" row: the triangular cursor (▶), the "Start" text highlight, and the sparkle arcs beside it — i.e. the deferred `MENU_CURSOR`/`SPARKLE` sink arms. Everything else (art, logo, all other menu text, background) is **bit-identical**. | ckpt 26 (2026-06-02) |

## Pending (rendered but not yet 1:1 — the known gaps)

These are the deltas that, once fixed, should move entry #1 (and future entries)
to fully bit-identical:

- **Menu selection cursor** (triangular ▶ + row highlight) — `MENU_CURSOR` sink
  arm, CURSOR bank (pool 20). Deferred no-op today.
- **Selection sparkle** (the arcs beside the cursor) — `SPARKLE` sink arm; needs
  the runtime alpha ramps `0x8a92b8`/`0x8a9308` populated.
- **Lizsoft studio splash** + **intro fade pacing** — `LOGO` arm + the stubbed
  pace pump `0x5b1030` (the port doesn't render the studio splash and rushes the
  intro). Frame-for-frame intro parity needs these.

When a gap closes, re-diff the affected frames and update the table.
