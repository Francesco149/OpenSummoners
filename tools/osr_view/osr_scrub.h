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

#ifdef __cplusplus
}
#endif

#endif /* OPENSUMMONERS_OSR_SCRUB_H */
