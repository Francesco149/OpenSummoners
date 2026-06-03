/*
 * tools/extract/cursor_trim_probe.c — feed the real Lizsoft 0x455 atlas blob
 * into the PORT's actual bs_trim_opaque_rect + cell-geometry logic and dump the
 * per-frame trim metrics, to compare against the verified Python proof
 * (cursor_frame_match.py: frame 17 = 22x41 @ (4,3)).
 *
 * This isolates whether the C trim path produces the same bbox as Python — i.e.
 * whether the "scale_flag=1 render bug" is in the trim/slice (hypothesis from
 * HANDOFF) or somewhere downstream (blit / metrics / capture-misread).
 *
 * Build (host, not Windows):
 *   cc -std=c11 -I src tools/extract/cursor_trim_probe.c src/bitmap_session.c \
 *      -o /tmp/cursor_trim_probe
 * Run:
 *   /tmp/cursor_trim_probe runs/extract/sotesd/type=DATA/1109.bin
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "bitmap_session.h"

/* host stubs for the win32-backed primitives (only the trim path is exercised;
 * these are never called on it, but the TU references them). */
void *bs_local_alloc_zeroed(uint32_t bytes) { return calloc(1, bytes); }
void  bs_local_free(void *p) { free(p); }
const void *bs_load_pe_resource(void *hModule, uint16_t resource_id,
                                const char *type) {
    (void)hModule; (void)resource_id; (void)type; return NULL;
}

#define W   128
#define H   288
#define COLS 4
#define ROWS 6
#define CW  32
#define CH  48
#define PIX_OFF 1142          /* 32 magic + 1024 pal + 64 + 14 + 8 */

int main(int argc, char **argv)
{
    if (argc != 2) { fprintf(stderr, "usage: %s <1109.bin>\n", argv[0]); return 2; }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror("fopen"); return 1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *blob = malloc((size_t)sz);
    if (fread(blob, 1, (size_t)sz, f) != (size_t)sz) { perror("fread"); return 1; }
    fclose(f);

    if (sz != PIX_OFF + W * H) {
        fprintf(stderr, "!! size %ld != %d\n", sz, PIX_OFF + W * H);
        return 1;
    }

    /* The Lizsoft DATA blob's pixel area is the raw 8bpp DIB in STORAGE order
     * (BMP-style bottom-up: storage row 0 = bottom of the displayed image).
     * The decoder (bs_decode_resource) installs these bytes verbatim into
     * session->pixels and the trim scanner treats them as a bottom-up DIB
     * (bottom = pixels + (biHeight-1)*stride). So construct the session exactly
     * as the decoder would: pixels = blob[PIX_OFF:], stride = biWidth, 8bpp. */
    bitmap_session s;
    memset(&s, 0, sizeof s);
    s.pixels     = blob + PIX_OFF;
    s.stride     = W;            /* (8/8)*128 */
    s.biWidth    = W;
    s.biHeight   = H;
    s.biBitCount = 8;

    /* colorkey 0 = palette index 0 transparent (the cursor bank registers
     * colorkey=0). */
    const uint32_t key = 0;

    printf("frame  (r,c)  base(x,y)   trim[x_left,x_right,y_top,y_bottom]"
           "  -> w x h @ (offx,offy)  key/opq\n");
    for (int fr = 0; fr < COLS * ROWS; fr++) {
        int r = fr / COLS, c = fr % COLS;
        int base_x = c * CW, base_y = r * CH;
        bs_trim_rect t;
        /* slicer passes (height=cell_w, width=cell_h) per the literal-mirror
         * comment in ar_sprite_slice — i.e. arg order is (base_x, base_y,
         * cell_w, cell_h). */
        bs_trim_opaque_rect(&s, key, base_x, base_y, CW, CH, &t);

        /* build_cell's trim_count>1 geometry: src_x=x_left, src_y=y_top,
         * w=(x_right-x_left)+1, h=(y_bottom-y_top)+1. */
        int w = (t.x_right - t.x_left) + 1;
        int h = (t.y_bottom - t.y_top) + 1;
        if (w < 0) w = 0;
        if (h < 0) h = 0;
        char tag[40] = "";
        if (fr >= 16 && fr <= 19)
            snprintf(tag, sizeof tag, "   <-- cursor frame %d", fr);
        printf("  %2d   (%d,%d)  (%3d,%3d)  [%3d,%3d,%3d,%3d]  -> %2d x %2d @ (%d,%d)  %d/%d%s\n",
               fr, r, c, base_x, base_y,
               t.x_left, t.x_right, t.y_top, t.y_bottom,
               w, h, t.x_left, t.y_top, t.found_key, t.found_opaque, tag);
    }
    free(blob);
    return 0;
}
