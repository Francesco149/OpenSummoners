# tools/archive — superseded tooling (kept for the record, do not extend)

Nothing here should be used for new work. Each entry names what replaced it.
(Per the 2026-06-10 USER directive: stale/ad-hoc tooling gets archived, not
silently abandoned.)

## Superseded FLOWS (no files — they were ad-hoc, never committed)

- **The side-by-side video flow (the "video capture thing", ckpt 89).**
  Hand-built `/tmp` scripts that frame-matched retail|port PNG pairs into
  `/tmp/intro_sidebyside.mp4` + feed montages for USER review. **Superseded by
  the TRACE STUDIO** (`tools/trace_studio.py`, docs/trace-studio.md): one
  command captures both sides, pairs anchor-segmented, encodes scrub videos,
  and serves a browser studio where divergences are flagged as marks/notes —
  with re-capture from the same session. Don't build one-off comparison
  videos by hand; capture a studio session.

## Marked for removal (tooling-debt — flags on live tools)

- **The one-shot probe flags in `tools/frida_capture.py` + the agent**
  (`--cursor-probe --fade-probe --pace-probe --textout-probe --box-probe
  --res-probe --parallax-probe --rand-probe`): each answered a specific
  RE question (R1/R3 pacing, the newgame box chrome, PE-resource loads, the
  town parallax, …) whose answers now live in `docs/findings/` +
  `engine-quirks.md`. All are superseded by the **field-spec mechanism**
  (`tools/flow/retail_fields.json` + `--field-spec[-only]`) — see the standing
  rule in `docs/parity-harness.md` ("don't add a bespoke --foo-probe flag").
  Removal is a mechanical follow-up chip: delete the CaptureConfig fields,
  arg rows, handler branches, and the agent installers; verify with a studio
  smoke capture. Deferred from the studio checkpoint so the live capture path
  stayed untouched while the first USER review round ran.
