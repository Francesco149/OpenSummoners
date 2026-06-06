# The DDraw blit-command + state trace (Phase-B B3)

The render-stream half of the divergence loop. SotES renders through a
DirectDraw7 **software blitter**, so the analog of openrecet's `d3d-trace` is a
**per-blit log**: every source-bearing blit, on both sides, with the source's
cross-side identity + the geometry + the DDraw "state". `tools/render_diff.py`
then names the first divergent blit and classifies it. Pairs with `flow_diff.py`
(the LOGIC drill-in): **render_diff says which draw is wrong; flow_diff says which
logic produced it.**

## The cross-side identity — `(resource_id, frame)` + a decode fingerprint

A blit serialises its source as a raw cel pointer, which is allocation-dependent
(differs port↔retail, run↔run). The fix is openrecet's `tex_name` trick — drop
the pointer from the diff key, key on a **load-stable name** — adapted to SotES:

- **`(resource_id, frame)`** — `resource_id` is the PE "DATA" resource a sprite
  bank decodes from (`ar_sprite_slot +0x40`), identical in both binaries; `frame`
  is the cel index within the bank. Both sides register at the ONE universal
  resolver `FUN_00418470` (`ar_sprite_slot_frame`, `__thiscall`: ECX = the bank
  slot, arg0 = frame, retval = the cel), so every cel — tiles, parallax,
  prologue, menu, actors — is tagged uniformly. Port: `src/render_id.c` populated
  in `ar_sprite_slice`. Retail: `g_render_id_map` in the Frida agent
  (`installRenderIdHook`).

- **`dhash`** — an FNV-1a fingerprint of the **decoded sheet pixels** (per bank,
  computed once at decode in `ar_sprite_slice`). This is the improvement over
  openrecet's name-only scheme, and the reason to do it for a *software* blitter:
  openrecet's GPU textures are expensive to hash and a name can only flag the
  WRONG sprite; our pixels sit in a CPU buffer at decode time, so a near-free
  content hash additionally catches the RIGHT sprite decoding to the WRONG pixels
  — the recurring SotES residual class (palette grade / 24bpp decode, ckpt 67/68).
  `render_diff` classes that as `[decode]`. (Port-side today; the retail-side
  decode-hash is the next layer — see "Open".)

## What each blit event carries

Emitted at the five source-bearing primitives via `zdd_emit_blit` (`src/zdd.c`),
read on retail per the blit VAs in `tools/flow/retail_fields.json`:

| VA | name | mode |
|----|------|------|
| `0x5b9a40` | blt_onto    | 0 plain Blt (state-flag carry) — parallax sink |
| `0x5b9b70` | blt_keyed   | 1 positioned + KEYSRC |
| `0x5b9ae0` | blt_rects   | 2 explicit rects |
| `0x5b9bf0` | blt_clipped | 3 clipped + KEYSRC — **the backdrop-tile path** |
| `0x5bd550` | blt_alpha   | 4 software alpha/colorize (`bmode` = blend mode) |

Fields: `res`/`frame`/`dhash` (identity), `dx`/`dy`/`reqw`/`reqh`/`sx`/`sy` (the
RAW call geometry — clip math is a bit-exact port, deterministic from these + the
src metrics, so we emit inputs not the post-clip rect, and retail's onEnter reads
the same without an onLeave hook), `ow`/`oh`/`ox`/`oy` (the source cel's
placement metrics, read off ECX on retail via `src:"thisderef"`), and the DDraw
**state**: `st` (`+0xd4`, `0x8000` = KEYSRC armed), `ckey` (the bound color key),
`bmode` (the alpha blend mode, −1 if N/A).

Both streams are ordinary `call_trace.jsonl` — the blit events ride the existing
field-bearing transport (`CALL_TRACE_BEGIN/FIELD/END` + the agent's per-VA field
reader), so they compose with `flow_diff` and need no second emitter. Two new
agent field sources were added (each auto-installs its hook, no ad-hoc flag, the
`rngcalls` pattern): `renderid` (the registry lookup of the blit's source object,
ECX or `args[index]`, `key`:res|frame) and `thisderef` (a field off ECX).

## render_diff.py

Filters both traces to the blit VAs, aligns each frame's blit sequence by
`(va, res, frame)` with `difflib.SequenceMatcher` (unnamed cels fall back to
positional alignment within their VA), then classifies the first divergence per
aligned pair, in priority order:

- `[sprite]` — one side drew a sprite the other didn't (insert/delete/replace)
- `[decode]` — same sprite, different `dhash` (both present) — wrong pixels
- `[rect]` — same sprite, different geometry
- `[state]` — same sprite, different colorkey / KEYSRC arm / blend mode

Only fields present on BOTH sides compare, so a port-only field (`dhash` before
the retail hash lands; the informational `mode`) never false-flags. Host-tested
(`tools/test_render_diff.py`, 9 checks).

## How to run

```
# port (inside nix develop so OPENSUMMONERS_GAME_DIR is set → sotesd.dll loads;
# --input-trace needs an ABSOLUTE path, the port loads it before the game-dir chdir):
nix develop --command ./build/opensummoners-launcher.exe --timeout-ms 65000 -- /tmp/oss.exe \
    --hide-window --frames 1100 --input-trace "$PWD/tests/scenarios/in-game-intro/trace-port.jsonl" \
    --call-trace blit_port.jsonl --call-trace-frames "900,1000,1050"
#   (the .jsonl lands in the launcher's CWD — call_trace opens it before the chdir)

# retail (the blit VAs + the resolver hook auto-install from the field spec):
OPENSUMMONERS_DURATION_MS=40000 nix develop --command bash tools/run-retail.sh \
    --no-turbo --hide-window --seed-pin --call-trace --field-spec-only \
    --call-trace-frames "400,600,800,1000,1200" --run-dir /tmp/blit_retail --exact-run-dir

# diff (align by identity, classify):
nix develop --command python3 tools/render_diff.py \
    --retail /tmp/blit_retail/call_trace.jsonl --port blit_port.jsonl --first
```

## Verified (2026-06-06)

End-to-end on real captures from both binaries (on the title screen):
- **Port** emitted `res=2331 (0x91b) frame=6 dhash=0x77c7a334` (640×480 background,
  blt_keyed) + `res=2332 (0x91c) frame=0 dhash=0x16a9512b bmode=1` (160×32 alpha).
- **Retail** emitted `res=2331 (0x91b)` — the **identical resource_id** — for the
  background, plus 56 `blt_clipped res=0x91b frame=5` 4×48 strips (a title element
  drawn column-by-column). The `thisderef` object metrics + the `arg` rects read
  correctly, confirming the calling convention (ECX = src, args[0] = dest, args[1..]
  = coords).
- **render_diff** named every blit by `(res, frame)` and classified the title-phase
  difference (port stalled on the title vs retail's later phase) as `[sprite]`.

This proves the cross-side identity, the registry on both sides, the geometry/
state reads, and the classifier. dhash is deterministic across frames (the decode
is stable).

## Open / next layers

- **Retail dhash** — hook the decoder `FUN_004184a0` and fingerprint the decoded
  sheet buffer at the same pipeline stage as the port (`ar_sprite_slice` after the
  display-depth convert). Until then `dhash` is port-only and `[decode]` can't
  fire cross-side; it still serves as the port's decode self-fingerprint.
- **`0x5bd550` retail spec** — the alpha orchestrator is `__cdecl` (all stack
  args), so its `res`/rect fields need `src:"renderid"`/`arg` with stack indices
  (src = `args[2]`, colorkey = `args[8]`), validated live — deferred (the town
  tiles use `blt_clipped`, the validated thiscall path).
- **A clean same-scene cross-side diff** — needs both sides anchored to the same
  scene at aligned frames (a working in-game input trace; the current port trace
  reaches `game_enter@1116` but the headless town backdrop scene-load is a
  separate open item). The title comparison above already exercises the full path.
