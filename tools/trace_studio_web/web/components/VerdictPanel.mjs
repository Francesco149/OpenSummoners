// web/components/VerdictPanel.mjs — the anchor-RNG/pairing verdict text
// (+ flow_diff output on --call-trace sessions).
import { html } from "/vendor/htm-preact-standalone.mjs";

export function VerdictPanel({ view }) {
  const v = view.manifest.verdict;
  const t = v && v.text;
  return html`<section class="panel"><h3>RNG / pairing verdict</h3>
    <pre class="verdict">${t || "(no verdict — capture failed before pairing?)"}</pre></section>`;
}
