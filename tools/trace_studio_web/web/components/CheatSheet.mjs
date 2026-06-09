// web/components/CheatSheet.mjs — the studio loop, step by step, pinned to the
// bottom of the page. Static content.
import { html } from "/vendor/htm-preact-standalone.mjs";

export function CheatSheet() {
  return html`<details class="cheatsheet">
    <summary>📋 studio cheatsheet <span class="dim">— the divergence-flagging loop</span></summary>
    <div class="cheat-body">

      <h4>A · Your loop (the USER)</h4>
      <ol>
        <li><b>Scrub.</b> ←/→ ±10 · ,/. ±1 · Home/End · 1/2/3 toggle panels.
          The ribbon under the videos is per-frame <code>differ_px</code>
          (black = bit-exact); <b>next ⟂</b> jumps to the next non-clean frame.</li>
        <li><b>Flag a divergence:</b> optionally drag a box on a video panel
          (attaches the region + copies a crop ref), type a note, hit
          <b>✎ divergence note</b>. Marks land in the session and render as
          retail|port|white-diff crops you can click to zoom.</li>
        <li><b>✎ worklist</b> (iterate panel) renders all marks into
          <code>worklist.md</code> — that's what Claude reads next session.</li>
      </ol>

      <h4>B · Claude's loop (after your marks)</h4>
      <ol>
        <li>Read <code>worklist.md</code> → for each mark: attribute to a
          PILLAR (logic / phase / RNG / inputs — docs/parity-model.md).</li>
        <li>Drill: <code>render_diff</code> (which BLIT) /
          <code>flow_diff</code> (which LOGIC) at the mark's flips
          (state panel shows both sides' absolute flips).</li>
        <li>RE the responsible code, port the logic (never curve-fit),
          host-test.</li>
        <li><b>⟳ port only</b> re-capture (current build vs cached retail)
          → the ribbon cell goes black → next mark.</li>
      </ol>

      <h4>C · Reading the verdict panel</h4>
      <ul>
        <li><b>rng ALIGNED</b> at an anchor — both sides consumed the same LCG
          draw count to that boundary.</li>
        <li><b>rng DESYNC</b> — an unaccounted rand() consumer in the PRECEDING
          segment (the RNG pillar; chase with tools/rng_consumer_census.py).</li>
        <li><b>boot/title segment redness is documented</b> — retail renders the
          title at ~2.2× per update (parity-ledger R3); pairing there is
          approximate. From <code>newgame_enter</code> on, lockstep pairs 1:1.</li>
        <li><b>drift ±1</b> in the state panel = the port presented a duplicate
          frame (cadence wrinkle, absorbed by the sticky matcher) — not a bug.</li>
      </ul>

      <h4>D · Commands</h4>
      <pre>nix develop --command python3 tools/trace_studio.py capture in-game-intro
nix develop --command python3 tools/trace_studio.py recapture &lt;session&gt; [--only port]
nix develop --command python3 tools/trace_studio.py apply &lt;session&gt;   # marks → worklist.md
nix develop --command python3 tools/trace_studio.py serve --session &lt;session&gt;</pre>

    </div>
  </details>`;
}
