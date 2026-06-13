/*
 * src/osr_emit.h — the PORT-side ".osr" draw-stream emitter (Trace Studio v2 M5).
 *
 * The retail capture proxy (tools/capture_proxy) streams retail's draw calls into
 * a `.osr`; this module makes the PORT write the SAME format from its own sinks,
 * so the M6 studio can tick-join the two sides and scrub port|retail|diff over
 * reconstructed frames.  The sink map mirrors the proxy's hook map one-to-one:
 *
 *   proxy hook (retail VA)               port sink (this module's caller)
 *   ─────────────────────────────────    ─────────────────────────────────────
 *   flip 0x5b8fc0  FRAMEBEG/PRESENT      drive_present (main.c) → osr_emit_flip
 *   blit detours (5 VAs)       BLIT      zdd_emit_blit (zdd.c)  → osr_emit_blit
 *   sheet_grab.h (Lock src)    SHEET     first BLIT per src surface (internal)
 *   clear 0x5b9410             CLEAR     zdd_object_clear → osr_emit_clear
 *   blend_grab.h               BLEND     mode-4 BLIT's desc (internal dedup)
 *   engine_gdi.h IAT  TEXT/FONT          glyph_render_win32 + main.c GDI sites
 *   anchors / seed pins        ANCHOR/SEED   emit_anchor / rng_srand (main.c)
 *
 * Frame structure mirrors the proxy EXACTLY (present-then-framebeg): at flip f a
 * PRESENT closes the open frame and FRAMEBEG(f) opens the next, so the draws
 * listed under FRAMEBEG(f) are the ops issued after flip f, presented by flip
 * f+1 — the same one-flip label offset as retail's stream (immaterial to the
 * sim_tick join; see tools/capture_proxy/engine_hooks.h eh_flip_cb).
 *
 * Identity conventions (must match what the reconstructor expects):
 *   - BLIT.dhash references a SHEET whose pixels are the blit's SOURCE SURFACE
 *     content (the per-CEL surface, NOT the bank-level render_id sheet hash) —
 *     recon_apply resolves the source by this key.  The hash mirrors
 *     sheet_grab.h: FNV-1a over (w:u32, h:u32, bitcount:u16) then pitch*h raw
 *     surface bytes.  Pitch differs cross-side, so port and retail dhashes are
 *     not byte-comparable (PORT-DEBT(osr-sheet-dhash-xside)); (res,frame) stays
 *     the cross-side join.
 *   - BLIT/CLEAR/TEXT are emitted only when the dest is the PRIMARY compose
 *     surface (osr_emit_set_primary), mirroring retail's observed stream (dst
 *     100% backbuffer).  Offscreen cel composition reaches the file via the
 *     cel's SHEET grab at its first primary-dest blit — the same accepted
 *     staleness window as the proxy (re-composed cels keep first-grab pixels,
 *     policed retail-side by --validate snaps).
 *
 * Pure C, NO Win32: surface pixels are read through a caller-injected reader
 * (main.c registers a zdd_object_lock-based one; the host test registers a
 * canned one), so this file links into the host suite (test_osr_emit.c) and
 * every call gates internally on osr_emit_open — all sinks are cheap no-ops
 * when --osr-emit is off (the call_trace discipline).
 */
#ifndef OPENSUMMONERS_OSR_EMIT_H
#define OPENSUMMONERS_OSR_EMIT_H

#include <stdint.h>

#include "osr_format.h"

struct zdd_object;
struct zdd_blend_desc;

/* ── lifecycle ────────────────────────────────────────────────────────────── */

/* Open the output .osr and write its header (side=PORT).  Returns 1 on
 * success, 0 on open failure (the emitter stays inactive).  scenario may be
 * NULL. */
int  osr_emit_open(const char *path, uint32_t seed, uint32_t flags,
                   const char *scenario, uint16_t screen_w, uint16_t screen_h,
                   uint8_t pixfmt);
void osr_emit_close(void);
int  osr_emit_is_active(void);

/* The PRIMARY compose surface — the BLIT/CLEAR/TEXT dest filter.  Interned as
 * dst_handle 1. */
void osr_emit_set_primary(const struct zdd_object *primary);

/* ── the injected surface reader (the SHEET pixel grab) ──────────────────────
 * read() locks/exposes obj's surface pixels and fills `out`; done() releases.
 * Win32 build: zdd_object_lock + zdd_object_get_locked_info(+width).  Host
 * test: canned bytes.  read() returning 0 = grab failed (BLIT.dhash stays 0,
 * mirroring the proxy's failed-Lock path). */
typedef struct osr_emit_surf {
    const void *pixels;
    uint32_t    pitch;       /* bytes per row (may exceed w*bytespp) */
    uint32_t    w, h;        /* surface dimensions in pixels */
    uint16_t    bitcount;    /* bits per pixel (16 = RGB565 here) */
} osr_emit_surf;
typedef int  (*osr_emit_surf_read_fn)(struct zdd_object *obj, osr_emit_surf *out);
typedef void (*osr_emit_surf_done_fn)(struct zdd_object *obj);
void osr_emit_set_surf_reader(osr_emit_surf_read_fn read_fn,
                              osr_emit_surf_done_fn done_fn);

/* ── frame stream (drive_present) ────────────────────────────────────────── */
void osr_emit_flip(uint32_t flip, uint32_t sim_tick);
void osr_emit_anchor(const char *name, uint32_t flip, uint32_t sim_tick,
                     uint32_t rng);
void osr_emit_seed(uint32_t flip, uint32_t before, uint32_t value);

/* ── opt-in engine STATE (M8 — the trace-studio RNG/state panel) ─────────────
 * Arm with osr_emit_state_enable(1) (the port's --osr-state).  Each frame, the
 * caller PUSHES named scalar fields (osr_emit_state_field) BEFORE osr_emit_flip;
 * they are flushed as one OSR_STATE record right after the frame's FRAMEBEG and
 * the accumulator is cleared.  No-op unless armed AND --osr-emit is active.
 * kind ∈ {OSR_ST_HEX, OSR_ST_INT, OSR_ST_F32} (osr_format.h) — extend the field
 * set freely as more engine state is annotated. */
void osr_emit_state_enable(int on);
void osr_emit_state_field(const char *name, uint32_t kind, int64_t ival, double fval);

/* ── draw sinks (zdd.c) ──────────────────────────────────────────────────────
 * osr_emit_blit reads the source identity (render_id), placement metrics and
 * state off `src` itself; `desc` is non-NULL only on the mode-4 alpha path
 * (registers/dedups an OSR_BLEND and stamps blend_ref).  srcw/srch carry the
 * mode-2 RECTS source extent (0 elsewhere).  Emitted at the primitive's entry
 * with the RAW pre-clip geometry, exactly like the proxy's onEnter detours. */
void osr_emit_blit(uint32_t va, uint32_t mode,
                   struct zdd_object *src, struct zdd_object *dest,
                   int32_t dx, int32_t dy, int32_t reqw, int32_t reqh,
                   int32_t sx, int32_t sy, int32_t srcw, int32_t srch,
                   const struct zdd_blend_desc *desc, uint32_t ckey);
void osr_emit_clear(struct zdd_object *dest, uint32_t value);

/* A zdd_object is being destroyed — drop its sheet-grab cache entry so a
 * recycled pointer re-grabs (the ckpt-126 staleness lesson; tombstoned). */
void osr_emit_evict(struct zdd_object *obj);

/* ── GDI text sinks (glyph_render_win32.c + main.c text sites) ──────────────
 * A per-HDC shadow mirrors the proxy's engine_gdi.h: dc_open binds an HDC to
 * the surface it came from (zdd_object_get_dc), the gdi_* setters track the
 * selected font / text colour / bk mode, and gdi_text emits a TEXT record when
 * the HDC's surface is the primary.  font handles map to FONT records
 * registered at creation (osr_emit_font_create; f->font_ref is assigned
 * internally, the caller's value is ignored). */
void osr_emit_font_create(void *hfont, const osr_font *f);
void osr_emit_dc_open(struct zdd_object *obj, void *hdc);
void osr_emit_dc_close(void *hdc);
void osr_emit_gdi_select_font(void *hdc, void *hfont);
void osr_emit_gdi_color(void *hdc, uint32_t color);
void osr_emit_gdi_bkmode(void *hdc, int32_t bk_mode);
void osr_emit_gdi_text(void *hdc, int32_t x, int32_t y,
                       const char *str, int32_t len);

#endif /* OPENSUMMONERS_OSR_EMIT_H */
