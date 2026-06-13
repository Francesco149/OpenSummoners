/*
 * src/osr_replay.c — the ".osr" draw-stream streaming reader (Trace Studio v2 M4).
 *
 * See osr_replay.h for the design rationale (why streaming, not load-the-file).
 * This is the pure-C parse+dispatch core; the Win32 reconstruction sink lives in
 * the --osr-replay driver (src/main.c).
 */
#include "osr_replay.h"

#include <stdlib.h>

int osr_replay_stream(FILE *f, const osr_replay_sink *sink)
{
    if (f == NULL || sink == NULL) return OSR_REPLAY_ERR_OPEN;

    /* Header first — a short read or a bad magic/version is a hard error (a
     * valid capture always has a complete 64-byte header). */
    uint8_t hbuf[OSR_HEADER_SIZE];
    if (fread(hbuf, 1, OSR_HEADER_SIZE, f) != OSR_HEADER_SIZE)
        return OSR_REPLAY_ERR_HEADER;
    osr_header h;
    if (!osr_header_decode(hbuf, OSR_HEADER_SIZE, &h))
        return OSR_REPLAY_ERR_HEADER;
    if (sink->on_header) sink->on_header(sink->user, &h);

    /* One growable read buffer reused for every record's payload — grows to the
     * largest record seen (a 640×480 RGB565 backdrop SHEET ≈ 600 KB) and never
     * shrinks, so steady state does no allocation. */
    uint8_t *buf = NULL;
    size_t   cap = 0;
    int      rc  = OSR_REPLAY_OK;

    uint8_t fh[8];
    while (fread(fh, 1, sizeof fh, f) == sizeof fh) {
        uint32_t type = osr_get_u32(fh);
        uint32_t len  = osr_get_u32(fh + 4);

        if (len > cap) {
            uint8_t *nb = (uint8_t *)realloc(buf, len);
            if (nb == NULL) { rc = OSR_REPLAY_ERR_OOM; break; }
            buf = nb;
            cap = len;
        }
        /* A truncated payload (hard-killed capture) ends the stream cleanly. */
        if (len != 0 && fread(buf, 1, len, f) != len) break;

        switch (type) {
        case OSR_FRAMEBEG: {
            osr_framebeg fb;
            if (sink->on_frame_begin && osr_dec_framebeg(buf, len, &fb))
                sink->on_frame_begin(sink->user, &fb);
            break;
        }
        case OSR_PRESENT: {
            osr_present pr;
            if (sink->on_present && osr_dec_present(buf, len, &pr))
                sink->on_present(sink->user, &pr);
            break;
        }
        case OSR_CLEAR: {
            osr_clear cl;
            if (sink->on_clear && osr_dec_clear(buf, len, &cl))
                sink->on_clear(sink->user, &cl);
            break;
        }
        case OSR_BLIT: {
            osr_blit b;
            if (sink->on_blit && osr_dec_blit(buf, len, &b))
                sink->on_blit(sink->user, &b);
            break;
        }
        case OSR_TEXT: {
            osr_text t;
            if (sink->on_text && osr_dec_text(buf, len, &t))
                sink->on_text(sink->user, &t);
            break;
        }
        case OSR_SHEET: {
            osr_sheet s;
            if (sink->on_sheet && osr_dec_sheet(buf, len, &s))
                sink->on_sheet(sink->user, &s);
            break;
        }
        case OSR_SNAP: {
            osr_snap sn;
            if (sink->on_snap && osr_dec_snap(buf, len, &sn))
                sink->on_snap(sink->user, &sn);
            break;
        }
        case OSR_BLEND: {
            osr_blend bl;
            if (sink->on_blend && osr_dec_blend(buf, len, &bl))
                sink->on_blend(sink->user, &bl);
            break;
        }
        case OSR_FONT: {
            osr_font fo;
            if (sink->on_font && osr_dec_font(buf, len, &fo))
                sink->on_font(sink->user, &fo);
            break;
        }
        case OSR_ANCHOR: {
            osr_anchor a;
            if (sink->on_anchor && osr_dec_anchor(buf, len, &a))
                sink->on_anchor(sink->user, &a);
            break;
        }
        case OSR_SEED: {
            osr_seed s;
            if (sink->on_seed && osr_dec_seed(buf, len, &s))
                sink->on_seed(sink->user, &s);
            break;
        }
        default:
            /* Unknown/reserved type (CLEAR/PALETTE/INPUT land later) — the
             * payload is already consumed; skip it.  Forward-compatible by
             * design (osr_format.h frames every record with its length). */
            break;
        }
    }

    free(buf);
    return rc;
}

int osr_replay_file(const char *path, const osr_replay_sink *sink)
{
    if (path == NULL) return OSR_REPLAY_ERR_OPEN;
    FILE *f = fopen(path, "rb");
    if (f == NULL) return OSR_REPLAY_ERR_OPEN;
    int r = osr_replay_stream(f, sink);
    fclose(f);
    return r;
}
