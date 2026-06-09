// web/components/TracePanel.mjs — edit the session's WORKING input traces.
// Each side's trace is flat JSONL ({"frame": N, "ids": [...]}, # comments ok)
// timed on ITS OWN flip axis (port vs retail boot/load skew). Saving validates
// server-side (non-decreasing frames etc.) and marks the session STALE until
// the next re-capture.  Button ids: 1=up 3=down 0x24=confirm/Z 0x22=abort.
import { html, useState, useEffect } from "/vendor/htm-preact-standalone.mjs";
import { postJSON } from "/store.mjs";
import { toast } from "/web/util.mjs";

function SideEditor({ sess, side, text0 }) {
  const [text, setText] = useState(text0 || "");
  const [dirty, setDirty] = useState(false);
  useEffect(() => { setText(text0 || ""); setDirty(false); }, [text0]);
  const save = () => {
    postJSON(`/s/${sess}/trace`, { side, text }).then((r) => {
      if (!r.ok) return toast(`${side}: ${r.error || "save failed"}`, true);
      toast(`${side} trace saved (${r.n_lines} lines) — STALE until re-capture`);
      setDirty(false);
    });
  };
  return html`<div class="trace-side">
    <h4>${side} trace ${dirty ? html`<span class="stale-dot">●</span>` : ""}</h4>
    <textarea spellcheck="false" rows="14" value=${text}
      onInput=${(e) => { setText(e.target.value); setDirty(true); }}></textarea>
    <button onClick=${save} disabled=${!dirty}>save ${side}</button>
  </div>`;
}

export function TracePanel({ sess, traces }) {
  return html`<section class="panel trace-panel"><h3>working input traces</h3>
    <div class="trace-cols">
      <${SideEditor} sess=${sess} side="port" text0=${traces.port} />
      <${SideEditor} sess=${sess} side="retail" text0=${traces.retail} />
    </div>
  </section>`;
}
