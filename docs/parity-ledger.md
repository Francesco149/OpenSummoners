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
| _(none yet)_ | — | — | — | — |

## Open residuals — under investigation (NOT yet 1:1)

### R1 — Title menu (idle, settled): 955/307200 px (0.31%), mean|abs|/ch 0.047

Port Flip 200 vs retail `title-idle` golden Flip 1900. The whole frame
(character art, background, logo, all menu text, the body of the selection
cursor) matches; the residual is **edge/anti-alias pixels on the selection
cursor (▶) + the "Start" highlight**. This is NOT closed. Two hypotheses
(per user, ckpt 27) — investigate both:

1. **Phase / timing skew.** The cursor highlight likely **pulses** (its alpha is
   animated via the fade ramp; retail's cursor `level_num` = `[esp+0x20]` is a
   *per-frame, path-dependent* value, not a constant — that's why it couldn't be
   pinned statically). The port rushes the intro (no pacing fidelity yet), so
   port Flip 200 and retail Flip 1900 are almost certainly at **different points
   of the cursor pulse** → different cursor brightness → edge diff. **Test:**
   diff several port menu frames against several retail menu goldens (1300/1600/
   1900); if the residual *varies* with the pairing, it's phase (and the real
   fix is intro-pacing parity, R3 below, so the pulse phases align). If the
   residual is *constant* across pairings, phase is not the cause.
2. **Cursor blend over-brightens.** We drive the cursor `level_num` to idx 19
   (full, mode-1 add-blend). If retail's actual per-frame level is lower, our
   add brightens the cursor edges too much. **Test:** Frida-hook `0x56c470` on
   retail, log arg3 (`level_num`) + arg4 (`level_div`) at the menu; or sweep idx
   16..19 in the port and pick the one that minimises the diff. NB this overlaps
   with (1): if the level is animated, the "right" value is phase-dependent.

Method to capture the port frame: `--capture-frames` (Flip-indexed). The diff
math: `tools/push_comparison.py` / `pixel_diff.amplified_diff`.

## Other rendered-but-not-1:1 gaps (arms not yet wired)

- **Selection sparkle** (arcs beside the cursor) — `SPARKLE` sink arm. The alpha
  ramps are now populated (ckpt 27), so this is wirable; it carries an explicit
  blend-descriptor pointer per command (see `title_sink.h`).
- **Lizsoft studio splash** + **intro fade pacing** — `LOGO` arm + the stubbed
  pace pump `0x5b1030`. The port doesn't render the studio splash and rushes the
  intro; frame-for-frame intro parity (and likely R1's phase alignment) needs
  the pacing ported.

When a residual reaches `differ_px == 0`, move it to "Confirmed bit-exact".
