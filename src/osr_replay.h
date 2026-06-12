/*
 * src/osr_replay.h — the ".osr" draw-stream STREAMING reader (Trace Studio v2 M4).
 *
 * The reconstructor (`opensummoners.exe --osr-replay`, docs/plans/trace-studio-v2.md
 * §M4) reads a captured `.osr` and rebuilds each frame 1:1 by replaying the recorded
 * draw ops through the port's own bit-exact sinks (zdd.c blits + real GDI text).
 *
 * Why STREAMING, not load-the-whole-file: a real capture `.osr` is HUGE — a
 * nav→town run is ~600 MB (14M+ BLITs / 550k+ TEXT records) — and the port is a
 * 32-bit process, so the file CANNOT be slurped into memory.  Instead this reader
 * walks the framed record stream sequentially and dispatches each record to a
 * visitor `osr_replay_sink`.  The dedup'd SHEET/FONT records are small + bounded
 * (a few hundred sheets, a handful of fonts) and arrive in-stream BEFORE the BLIT
 * that first references them (the capture grabs+emits a source SHEET, then the
 * BLIT — see tools/capture_proxy/engine_hooks.h eh_blit_record), so the sink can
 * accumulate a persistent dhash→surface / font_ref→HFONT table as it goes and have
 * every reference resolved by the time a frame is reconstructed.
 *
 * Pure C, NO Win32 — so it links into the host unit suite (test_osr_replay.c) and
 * the cross build alike.  The Win32 sink (surface build, blit replay, GDI text,
 * BMP snapshot) lives in the reconstructor driver (src/main.c), where g_zdd and
 * GDI are available; this file only parses + dispatches.
 */
#ifndef OPENSUMMONERS_OSR_REPLAY_H
#define OPENSUMMONERS_OSR_REPLAY_H

#include <stdio.h>

#include "osr_format.h"

/* ── the streaming visitor ───────────────────────────────────────────────────
 * Every callback may be NULL (the streamer skips it).  The bytes behind the
 * variable-length records — osr_sheet.bytes, osr_text.str, osr_anchor.name —
 * are valid ONLY for the duration of the call (the streamer reuses one growable
 * read buffer per record); a sink that needs to retain them must copy. */
typedef struct osr_replay_sink {
    void *user;
    void (*on_header)(void *user, const osr_header *h);
    void (*on_frame_begin)(void *user, const osr_framebeg *fb);
    void (*on_present)(void *user, const osr_present *pr);
    void (*on_blit)(void *user, const osr_blit *b);
    void (*on_text)(void *user, const osr_text *t);
    void (*on_sheet)(void *user, const osr_sheet *s);
    void (*on_font)(void *user, const osr_font *f);
    void (*on_anchor)(void *user, const osr_anchor *a);
    void (*on_seed)(void *user, const osr_seed *s);
} osr_replay_sink;

/* Return codes. */
enum {
    OSR_REPLAY_OK         =  0,
    OSR_REPLAY_ERR_OPEN   = -1,   /* NULL FILE/path/sink, or fopen failed */
    OSR_REPLAY_ERR_HEADER = -2,   /* short read or bad magic/version */
    OSR_REPLAY_ERR_OOM    = -3    /* read-buffer realloc failed */
};

/* Stream the whole `.osr` from `f` (positioned at the start): read+validate the
 * 64-byte header (→ on_header), then loop the framed records, decoding each and
 * dispatching to the matching sink callback.  A TRUNCATED tail (a hard-killed
 * capture — the harness SIGKILLs the game) stops the loop cleanly and still
 * returns OSR_REPLAY_OK: the contract is "valid up to the last fully-written
 * record" (osr_format.h).  Unknown/reserved record types are skipped by length.
 * Does NOT close `f`. */
int osr_replay_stream(FILE *f, const osr_replay_sink *sink);

/* Convenience: fopen(path,"rb") → osr_replay_stream → fclose.  Returns the same
 * codes (OSR_REPLAY_ERR_OPEN if the file can't be opened). */
int osr_replay_file(const char *path, const osr_replay_sink *sink);

#endif /* OPENSUMMONERS_OSR_REPLAY_H */
