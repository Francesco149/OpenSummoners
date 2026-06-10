// web/components/StatePanel.mjs — per-frame port-vs-retail state at the cursor.
// Always available (the pairing emits flips/segment/drift per frame); a
// --call-trace session additionally carries the flow-trace fields, highlighted
// when port≠retail.
import { html, useState } from "/vendor/htm-preact-standalone.mjs";
import { fmt } from "/web/util.mjs";

const hex = (v) => (typeof v === "number" ? "0x" + (v >>> 0).toString(16).padStart(8, "0") : v);

export function StatePanel({ view, cur }) {
  const [filter, setFilter] = useState("");
  const row = view.state[cur];
  if (!row)
    return html`<section class="panel"><h3>per-frame state</h3>
      <div class="state">(no state at this frame)</div></section>`;

  let keys = [...new Set([
    ...Object.keys(row.port || {}), ...Object.keys(row.retail || {})])]
    .filter((k) => k !== "sim_tick").sort();
  if (filter) keys = keys.filter((k) => k.toLowerCase().includes(filter.toLowerCase()));

  return html`<section class="panel"><h3>per-frame state <span class="dim">@ f${cur}</span></h3>
    <div class="state-meta">
      <span>seg <b>${row.seg || "?"}</b></span>
      <span>· drift ${row.drift ?? 0}</span>
      ${row.retail?.sim_tick !== undefined && html`<span
        class=${row.port?.sim_tick !== undefined && row.port.sim_tick !== row.retail.sim_tick ? "bad" : ""}
        >· sim_tick ${row.port?.sim_tick !== undefined
          ? `port ${row.port.sim_tick} / retail ${row.retail.sim_tick}`
          : `retail ${row.retail.sim_tick}`}</span>`}
      ${row.anchor && html`<span class="accent"> · ⚓ ${row.anchor}</span>`}
    </div>
    ${(row.port_rng !== undefined || row.retail_rng !== undefined) && html`
      <div class=${"state-meta " + (row.port_rng === row.retail_rng ? "ok" : "bad")}>
        anchor rng: port ${hex(row.port_rng)} · retail ${hex(row.retail_rng)}
        ${row.port_rng === row.retail_rng ? " ✓" : " ✗ DESYNC"}
      </div>`}
    <input class="filter" placeholder="filter fields…" value=${filter}
      onInput=${(e) => setFilter(e.target.value)} />
    <div class="state"><table><tr><th>field</th><th>retail</th><th>port</th></tr>
    ${keys.map((k) => {
      const r = row.retail?.[k], p = row.port?.[k];
      const cls = (r === undefined || p === undefined) ? ""
        : (JSON.stringify(r) === JSON.stringify(p) ? "same" : "diff");
      return html`<tr class=${cls} key=${k}><td>${k}</td><td>${fmt(r)}</td><td>${fmt(p)}</td></tr>`;
    })}</table></div>
    ${!view.callTrace && html`<div class="dim" style="margin-top:.4rem">
      (re-capture with --call-trace for flow-trace fields)</div>`}
  </section>`;
}
