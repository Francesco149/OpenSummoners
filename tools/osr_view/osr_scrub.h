/*
 * tools/osr_view/osr_scrub.h — the C facade for the .osr reconstruction + scrub
 * engine, callable from the C++ ImGui viewer.
 *
 * The engine (DDraw reconstruction via recon_apply + a frame index + a lazy
 * keyframe cache) is C; the UI is C++/ImGui.  This header is the clean boundary:
 * extern "C", no zdd.h / Win32 internals leak across — the C++ side only opens a
 * scrub session and asks for "frame N as RGBA pixels".
 */
#ifndef OPENSUMMONERS_OSR_SCRUB_H
#define OPENSUMMONERS_OSR_SCRUB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct osr_scrub osr_scrub;

/* Open a scrub session: init DDraw (cooperative level on `hwnd`), index the
 * .osr's frames, and load every dedup'd source surface / font / blend descriptor.
 * Returns NULL on failure.  `hwnd` is the viewer's window (an HWND, void* to keep
 * this header Win32-free). */
osr_scrub *osr_scrub_open(void *hwnd, const char *osr_path);
void       osr_scrub_close(osr_scrub *s);

int  osr_scrub_frame_count(const osr_scrub *s);
int  osr_scrub_width(const osr_scrub *s);
int  osr_scrub_height(const osr_scrub *s);

/* Frame metadata for index `idx` (0-based). */
void osr_scrub_frame_info(const osr_scrub *s, int idx,
                          uint32_t *flip, uint32_t *tick);

/* Reconstruct frame `idx` and write it as tightly-packed RGBA8 (width*height*4
 * bytes, row-major top-down) into `out_rgba` (caller-owned, big enough).  Uses
 * the keyframe cache + forward replay so a forward step is one frame and a seek
 * is a short replay.  Returns 1 on success. */
int  osr_scrub_render_rgba(osr_scrub *s, int idx, uint32_t *out_rgba);

/* Accumulated profiling timings (ms): the one-time index/open, and the running
 * totals for the clear / blit-replay / RGBA-readback phases across all renders
 * (plus the render count).  Any out-pointer may be NULL. */
void osr_scrub_prof(const osr_scrub *s, double *index_ms, double *clear_ms,
                    double *replay_ms, double *readback_ms, long *renders);

/* ── draw-level drill-in (M7, the openrecet N3 self-serve debug loop) ──────── */

enum { OSR_DRAW_CLEAR = 0, OSR_DRAW_BLIT = 1, OSR_DRAW_TEXT = 2 };

/* One ordered draw op in a frame (the metadata the inspector lists). */
typedef struct osr_draw_info {
    int      kind;            /* OSR_DRAW_CLEAR / _BLIT / _TEXT */
    int      dx, dy, w, h;    /* dest rect (frame coords) for highlight; CLEAR = full */
    uint32_t res;
    int      frame;
    uint32_t va, dhash;
    char     label[80];       /* human one-liner */
} osr_draw_info;

/* Number of ordered draws (BLIT/TEXT/CLEAR) in the frame that idx resolves to
 * (a re-present resolves to the last non-empty frame, same as the renders). */
int osr_scrub_frame_ndraws(osr_scrub *s, int idx);

/* Fill `out[0..min(count,cap))` with the frame's ordered draws; returns the TOTAL
 * draw count (may exceed cap — pass a big enough buffer or call ndraws first). */
int osr_scrub_frame_draws(osr_scrub *s, int idx, osr_draw_info *out, int cap);

/* Reconstruct the frame applying only the first `kdraws` draws (kdraws < 0 = all)
 * → RGBA8 into out.  The render-up-to-K used to watch a frame build up. */
int osr_scrub_render_rgba_upto(osr_scrub *s, int idx, int kdraws, uint32_t *out);

/* Which draw last changed pixel (px,py) in the frame? Returns the 0-based draw
 * index (matching osr_scrub_frame_draws ordering), or -1 if no draw touches it.
 * Single incremental pass (clear → apply each draw → sample the pixel). */
int osr_scrub_pick_draw(osr_scrub *s, int idx, int px, int py);

#ifdef __cplusplus
}
#endif

#endif /* OPENSUMMONERS_OSR_SCRUB_H */
