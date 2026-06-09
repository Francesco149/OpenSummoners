// web/components/DiffRibbon.mjs — per-frame differ_px strip. The project's bar
// is BIT-EXACT (differ_px == 0), so the heat scale is anchored there: a 0 cell
// is near-black (clean), any non-zero lights up on a log scale. Click a cell to
// seek; "next ⟂" jumps to the next non-clean frame, "⟶ worst" to the max.
import { html, useMemo } from "/vendor/htm-preact-standalone.mjs";
import { toast } from "/web/util.mjs";

const FULL = Math.log10(1 + 640 * 480);     // whole-screen differ → t=1

function heat(differ) {
  if (!differ) return "#10161d";            // bit-exact — near-black
  const t = Math.min(1, Math.log10(1 + differ) / FULL);
  const r = t < 0.5 ? Math.round(510 * t) : 255;
  const g = t < 0.5 ? 200 : Math.round(200 * (1 - (t - 0.5) * 2));
  return `rgb(${r},${g},78)`;
}

export function DiffRibbon({ view, cur, setCur }) {
  const { seg, k } = view.locate(cur);
  const cells = useMemo(() => {
    const out = [];
    for (let i = 0; i < seg.nFrames; i++) {
      const d = seg.diffAt(i);
      out.push({ i, differ: d ? d.differ : 0, gt8: d ? d.gt8 : 0,
                 meanabs: d ? d.meanabs : 0, has: !!d });
    }
    return out;
  }, [seg]);
  const worst = cells.reduce((a, c) => (c.differ > a.differ ? c : a),
                             cells[0] || { i: 0, differ: 0 });
  const here = cells[k] || { differ: 0, gt8: 0, has: false };
  const nClean = cells.filter((c) => c.has && c.differ === 0).length;

  const goWorst = () => setCur(view.globalOf(seg, worst.i));
  const nextDiv = () => {
    for (let i = k + 1; i < cells.length; i++)
      if (cells[i].differ > 0) { setCur(view.globalOf(seg, i)); return; }
    toast(`no non-clean frame after ${k}`);
  };

  return html`<div class="diff-ribbon-wrap">
    <div class="diff-ribbon" title="per-frame differ_px — click to seek (black = bit-exact)">
      ${cells.map((c) => html`<div key=${c.i}
        class=${"diff-cell" + (c.i === k ? " cur" : "")}
        style=${`background:${c.has ? heat(c.differ) : "#0a0d11"}`}
        title=${`f${c.i}: differ_px ${c.differ} · gt8 ${c.gt8}`}
        onClick=${() => setCur(view.globalOf(seg, c.i))}></div>`)}
    </div>
    <div class="diff-bar">
      <span>f${k}: <b>${here.differ}</b> differ_px · ${here.gt8} gt8
        ${here.differ === 0 && here.has ? " ✓ bit-exact" : ""}</span>
      <span class="dim">· ${nClean}/${cells.length} clean</span>
      <span class="spacer"></span>
      <button onClick=${nextDiv} title="next non-clean frame">next ⟂</button>
      <button onClick=${goWorst}
        title=${`worst frame: f${worst.i} (${worst.differ}px)`}>⟶ worst</button>
    </div>
  </div>`;
}
