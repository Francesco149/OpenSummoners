// web/components/IteratePanel.mjs — the fix→re-capture→re-check loop buttons.
// "✎ worklist" renders the marks into worklist.md (the Claude hand-off);
// re-capture re-drives the session's WORKING traces with the CURRENT build
// (--only port = fast loop vs the cached retail capture).
import { html, useState } from "/vendor/htm-preact-standalone.mjs";
import { postJSON } from "/store.mjs";
import { toast } from "/web/util.mjs";
import { recapture as doRecapture } from "/web/actions.mjs";

export function IteratePanel({ sess, manifest, stale, reload, capJob, pollJobs, worklist }) {
  const [wlOpen, setWlOpen] = useState(false);
  const recapture = (only) => doRecapture(sess, { only, pollJobs });
  const apply = () => {
    toast("writing worklist…");
    postJSON(`/s/${sess}/apply`, {}).then((r) => {
      if (!r.ok) return toast("apply: " + (r.error || "fail"), true);
      toast(`worklist.md: ${r.n_marks} mark(s)`); reload();
    });
  };
  const clone = () => {
    const name = prompt("clone session as:"); if (!name) return;
    postJSON(`/s/${sess}/clone`, { name }).then((r) => r.ok
      ? (location.search = "?session=" + encodeURIComponent(r.name))
      : toast("clone: " + (r.error || "fail"), true));
  };

  const running = capJob && capJob.running;
  const done = capJob && !capJob.running && capJob.session;
  return html`<section class="panel"><h3>iterate
      ${stale && html`<span class="stale-dot" title="trace edits not yet captured">● STALE</span>`}
      ${running && html`<span class="dim"> · ⟳ ${capJob.elapsed_s}s</span>`}</h3>
    <div class="mark-row">
      <button onClick=${apply} title="render the marks into worklist.md (the Claude hand-off)">✎ worklist</button>
      <button onClick=${() => recapture()}>⟳ re-capture</button>
      <button onClick=${() => recapture("port")}
        title="re-run only the port (current build) vs the cached retail (fast)">⟳ port only</button>
    </div>
    <div class="mark-row">
      <button onClick=${clone}>⎘ clone</button>
    </div>
    ${worklist && html`<details open=${wlOpen} onToggle=${(e) => setWlOpen(e.currentTarget.open)}>
      <summary class="dim">worklist.md</summary>
      <pre class="verdict">${worklist}</pre>
    </details>`}
    <div class="rec-status">${done
      ? (capJob.rc === 0 || capJob.rc === null ? `✓ ${capJob.session}` : `✗ rc=${capJob.rc}`)
      : "—"}</div>
  </section>`;
}
