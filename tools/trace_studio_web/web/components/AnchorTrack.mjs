// web/components/AnchorTrack.mjs — the TAS-anchor overlay strip: one tick per
// anchor (manifest.anchors_ordinals), positioned at its viewer ordinal. Click
// to seek; the active segment (between ticks) is highlighted. This replaces
// openrecet's per-segment filmstrip segmentation — our videos are one
// continuous paired stream, the anchors are an overlay.
import { html } from "/vendor/htm-preact-standalone.mjs";

export function AnchorTrack({ view, cur, setCur }) {
  const N = view.totalFrames;
  const anchors = view.anchorsOrdinals;
  if (!anchors.length) return null;
  return html`<div class="anchor-track">
    ${anchors.map((a) => {
      const pct = N > 1 ? (a.ordinal / (N - 1)) * 100 : 0;
      const active = anchors.filter((x) => x.ordinal <= cur).slice(-1)[0] === a;
      return html`<div class=${"anchor-tick" + (active ? " active" : "")}
        style=${`left:${pct}%`} key=${a.name}
        title=${`${a.name} @ f${a.ordinal} (port flip ${a.port_flip} · retail flip ${a.retail_flip})`}
        onClick=${() => setCur(a.ordinal)}>
        <span class="anchor-lbl">${a.name}</span>
      </div>`;
    })}
  </div>`;
}
