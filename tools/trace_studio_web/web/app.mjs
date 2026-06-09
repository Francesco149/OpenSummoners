// web/app.mjs — the SPA root (composition only). Adapted from openrecet's
// studio v2: same scrub core (Filmstrip + VideoStage + ScrubBar + DiffRibbon),
// plus the AnchorTrack overlay; the side panels are MarkBar (the USER's
// divergence-flagging channel), IteratePanel (worklist + re-capture),
// TracePanel (working input traces), StatePanel + VerdictPanel.
import { html, render, useState, useEffect } from "/vendor/htm-preact-standalone.mjs";
import { qparam } from "/store.mjs";
import { useStudioModel, useRegistries, useJobs, jobOf } from "/web/model.mjs";
import { SessionPicker } from "/web/components/SessionPicker.mjs";
import { Filmstrip } from "/web/components/Filmstrip.mjs";
import { AnchorTrack } from "/web/components/AnchorTrack.mjs";
import { VideoStage } from "/web/components/VideoStage.mjs";
import { ScrubBar } from "/web/components/ScrubBar.mjs";
import { DiffRibbon } from "/web/components/DiffRibbon.mjs";
import { JobTray } from "/web/components/JobTray.mjs";
import { CheatSheet } from "/web/components/CheatSheet.mjs";
import { IteratePanel } from "/web/components/IteratePanel.mjs";
import { TracePanel } from "/web/components/TracePanel.mjs";
import { MarkBar } from "/web/components/MarkBar.mjs";
import { StatePanel } from "/web/components/StatePanel.mjs";
import { VerdictPanel } from "/web/components/VerdictPanel.mjs";

const SESS = qparam("session");

function useWide(minPx) {
  const [wide, setWide] = useState(true);
  useEffect(() => {
    const mq = window.matchMedia(`(min-width:${minPx}px)`);
    const on = () => setWide(mq.matches);
    on();
    mq.addEventListener("change", on);
    return () => mq.removeEventListener("change", on);
  }, [minPx]);
  return wide;
}

function layout(p) {
  const { wide, SESS, view, cur, setCur, N, panels, setPendingBox, pendingBox,
          registries, marks, setMarks, manifest, reload, capJob, pollJobs } = p;
  const videoBlock = html`<div class="vidblock">
    <${VideoStage} sess=${SESS} view=${view} cur=${cur} panels=${panels} onBox=${setPendingBox} />
    <${ScrubBar} N=${N} cur=${cur} setCur=${setCur} />
    <${DiffRibbon} view=${view} cur=${cur} setCur=${setCur} />
    <div class="hint">←/→ ±10 · ,/. ±1 · Home/End · 1/2/3 panels ·
      drag a box on a frame → attach to a mark${pendingBox ? ` · box ${pendingBox.join(",")}` : ""}</div>
  </div>`;
  const statePanel = html`<${StatePanel} view=${view} cur=${cur} />`;
  const verdictPanel = html`<${VerdictPanel} view=${view} />`;
  const tools = html`<div class="panels">
    <${MarkBar} sess=${SESS} view=${view} cur=${cur} setCur=${setCur}
      markTypes=${registries.marks} marks=${marks} setMarks=${setMarks}
      pendingBox=${pendingBox} setPendingBox=${setPendingBox} />
    <${IteratePanel} sess=${SESS} manifest=${manifest} stale=${manifest.stale}
      reload=${reload} capJob=${capJob} pollJobs=${pollJobs} worklist=${view.worklist} />
  </div>`;

  if (wide) return [html`<div class="workarea">
    <div class="scrub-col">
      ${videoBlock}
      ${tools}
      <div class="below-panels">${verdictPanel}</div>
    </div>
    <aside class="ref-col">${statePanel}</aside>
  </div>`];
  return [
    videoBlock,
    tools,
    html`<div class="two-col">${verdictPanel}${statePanel}</div>`,
  ];
}

function App() {
  const { view, loading, error, manifest, reload, marks: marks0, traces } =
    useStudioModel(SESS);
  const [cur, setCur] = useState(0);
  const [panels, setPanels] = useState({ port: true, retail: true, diff: true });
  const [pendingBox, setPendingBox] = useState(null);
  const [marks, setMarks] = useState([]);
  const registries = useRegistries();
  const [jobsStatus, pollJobs] = useJobs();
  const wide = useWide(1280);
  useEffect(() => { if (marks0) setMarks(marks0); }, [marks0]);

  const N = view ? view.totalFrames : 1;

  useEffect(() => {
    const onKey = (e) => {
      if (/^(INPUT|TEXTAREA|SELECT)$/.test(e.target.tagName)) return;
      const k = e.key, step = (d) => setCur((c) => Math.max(0, Math.min(N - 1, c + d)));
      if (k === "ArrowLeft") step(-10); else if (k === "ArrowRight") step(10);
      else if (k === ",") step(-1); else if (k === ".") step(1);
      else if (k === "Home") setCur(0); else if (k === "End") setCur(N - 1);
      else if (k === "1") setPanels((p) => ({ ...p, port: !p.port }));
      else if (k === "2") setPanels((p) => ({ ...p, retail: !p.retail }));
      else if (k === "3") setPanels((p) => ({ ...p, diff: !p.diff }));
      else return;
      e.preventDefault();
    };
    document.addEventListener("keydown", onKey);
    return () => document.removeEventListener("keydown", onKey);
  }, [N]);

  if (loading) return html`<div class="pad">loading ${SESS}…</div>`;
  if (error) return html`<div class="pad">
    <${SessionPicker} current=${SESS} /> <div class="err-box">error: ${error}</div></div>`;
  if (!view) return html`<div class="pad">no session — <${SessionPicker} current=${SESS} /></div>`;

  const { seg, k } = view.locate(cur);
  const capJob = jobOf(jobsStatus, "capture");
  const st = view.state[cur] || {};
  return html`<div>
    <header>
      <h1>trace studio <span class="dim">oss</span> · <span class="accent">${SESS}</span></h1>
      <div class="status">
        <span>${view.totalFrames}f · ${view.fps}fps${view.callTrace ? " · flow-trace" : ""}</span>
        <span class="sep">·</span><${SessionPicker} current=${SESS} />
      </div>
    </header>
    <main>
      <div class="note">scenario: ${manifest.scenario}
        · seg <b>${st.seg || "?"}</b>
        · port flip ${st.port ? st.port.flip : "?"}
        · retail flip ${st.retail ? st.retail.flip : "?"}</div>
      ${manifest.capture_error && html`<div class="err-box">⚠ ${manifest.capture_error}</div>`}
      <${JobTray} status=${jobsStatus} pollJobs=${pollJobs} />
      <${Filmstrip} view=${view} cur=${cur} setCur=${setCur} />
      <${AnchorTrack} view=${view} cur=${cur} setCur=${setCur} />
      <details class="trace-editor-fold">
        <summary>⚙ working traces <span class="dim">— edit inputs · re-capture picks them up</span></summary>
        <${TracePanel} sess=${SESS} traces=${traces || { port: "", retail: "" }} />
      </details>
      <div class="layout-bar"><span>panels:</span>
        ${["port", "retail", "diff"].map((p) =>
          html`<button class=${"ly " + (panels[p] ? "on" : "")}
            onClick=${() => setPanels((s) => ({ ...s, [p]: !s[p] }))}>${p}</button>`)}
        <span class="spacer"></span>
        <span class="dim">frame ${k}/${seg.nFrames - 1}</span>
      </div>
      ${layout({
        wide, SESS, view, cur, setCur, N, panels, setPendingBox, pendingBox,
        registries, marks, setMarks, manifest, reload, capJob, pollJobs,
      })}
      <${CheatSheet} />
    </main>
  </div>`;
}

render(html`<${App} />`, document.getElementById("app"));
