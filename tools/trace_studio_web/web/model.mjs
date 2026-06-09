// web/model.mjs — manifest + sidecars → the client view.
//
// The coordinate contract (docs/plans/trace-studio.md): the viewer ordinal IS
// the frame label on all three panels (frame files, state.jsonl rows,
// diff.per_frame and marks all key by it); per-side absolute flips live in the
// state row. One gameplay segment per session (the anchor structure is an
// overlay track, not video segmentation), so locate() is near-trivial — kept
// shaped like openrecet's so its components transfer.
import { useMemo, useState, useEffect } from "/vendor/htm-preact-standalone.mjs";
import { useSession, useStatus, getJSON } from "/store.mjs";

// verdict → filmstrip fill class.
export function classify(verdict) {
  if (!verdict || verdict.available === false) return "gray";
  const t = verdict.text || "";
  if (/DESYNC|DRIFT/.test(t)) return "red";
  if (verdict.ok === false) return "red";
  if (/ALIGNED/.test(t)) return "green";
  return "gray";
}

export function buildView(data) {
  const m = data.manifest;
  if (!m) return null;
  const fps = m.fps || 30;
  const nFrames = m.n_frames || 0;
  const frames = m.frame_range || [0, Math.max(0, nFrames - 1)];

  const diffByLabel = new Map();
  for (const d of (m.diff?.per_frame || [])) diffByLabel.set(d.frame, d);

  const seg = {
    idx: 0, nFrames, cadence: 1, frames, videos: { ...(m.videos || {}) },
    verdict: m.verdict, verdictClass: classify(m.verdict), offsetGlobal: 0,
    labelOf: (k) => frames[0] + k,
    videoTime: (k) => (Math.min(Math.max(k, 0), nFrames - 1) + 0.5) / fps,
    diffAt: (k) => diffByLabel.get(frames[0] + k) || null,
  };
  const segments = [seg];
  const timeline = [{ kind: "gameplay", idx: 0, frames, n_frames: nFrames }];

  const locate = (g) => {
    const gg = Math.max(0, Math.min(nFrames - 1, g | 0));
    return { seg, k: gg };
  };

  return {
    manifest: m, fps, target: m.target, callTrace: !!m.call_trace,
    segments, seams: [], timeline, totalFrames: nFrames || 1, diffByLabel,
    locate, globalOf: (s, k) => k,
    state: data.state || [],
    anchors: data.anchors || { port: [], retail: [] },
    anchorsOrdinals: m.anchors_ordinals || [],
    traces: data.traces || { port: "", retail: "" },
    marks: data.marks || [],
    worklist: data.worklist || "",
  };
}

export function useStudioModel(sess) {
  const data = useSession(sess);
  const view = useMemo(() => buildView(data), [
    data.manifest, data.state, data.marks, data.anchors, data.traces,
  ]);
  return { ...data, view };
}

// ─── registries + jobs (same wire shapes as openrecet) ───────────────────────
export function useRegistries() {
  const [reg, setReg] = useState({ marks: [], analyzers: [] });
  useEffect(() => { getJSON("/api/registries").then(setReg).catch(() => {}); }, []);
  return reg;
}

export function useJobs(intervalMs = 1500) {
  return useStatus("/api/jobs", intervalMs);
}
export const jobOf = (status, id) =>
  (status && status.jobs ? status.jobs.find((j) => j.id === id) : null) || null;
