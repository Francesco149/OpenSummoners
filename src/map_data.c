/* map_data.c — see map_data.h. */
#include "map_data.h"

#include <stdlib.h>
#include <string.h>

/* A sequential read cursor over the locked resource bytes — the pure-C stand-in
 * for FUN_005b6340 mode 1 (memcpy from the locked resource, advancing the
 * pointer).  Bounds-checked against `len` (a port hardening; see map_data.h). */
typedef struct md_cur {
    const uint8_t *buf;
    size_t len;
    size_t pos;
    int    err;
} md_cur;

/* Copy `n` bytes from the stream into `dst` (dst may be NULL to skip).  Sets
 * the cursor's error flag and reads nothing on overrun. */
static void md_read(md_cur *c, void *dst, size_t n)
{
    if (c->err || n > c->len - c->pos) { c->err = 1; return; }
    if (dst) memcpy(dst, c->buf + c->pos, n);
    c->pos += n;
}

static uint32_t md_dw(const uint8_t *p, unsigned off)
{
    uint32_t v;
    memcpy(&v, p + off, 4);
    return v;
}

void map_data_free(map_data *m)
{
    if (!m) return;
    free(m->cells);
    m->cells = NULL;
    if (m->layers) {
        for (uint32_t i = 0; i < m->count; i++) {
            free(m->layers[i].a);
            free(m->layers[i].b);
            free(m->layers[i].c);
            free(m->layers[i].d);
        }
        free(m->layers);
        m->layers = NULL;
    }
    m->count = 0;
}

/* Allocate + read one sub-array of `n` elements * `stride` bytes (0 -> NULL,
 * matching FUN_00587970's `if (count == 0) ptr = 0` arms). */
static uint8_t *md_read_sub(md_cur *c, uint32_t n, unsigned stride)
{
    if (n == 0) return NULL;
    size_t bytes = (size_t)n * stride;
    uint8_t *p = malloc(bytes);
    if (!p) { c->err = 1; return NULL; }
    md_read(c, p, bytes);
    return p;
}

int map_data_parse(map_data *m, const uint8_t *buf, size_t len)
{
    memset(m, 0, sizeof *m);
    md_cur c = { buf, len, 0, 0 };

    /* [0x00:0x04] magic, [0x04:0x34] header, [0x34:0x68] maphdr — the three
     * FUN_005b6340 reads at FUN_00587970:79-81. */
    md_read(&c, &m->magic, 4);
    md_read(&c, m->header, sizeof m->header);
    md_read(&c, m->maphdr, sizeof m->maphdr);
    if (c.err) { map_data_free(m); return -1; }

    m->dim0  = md_dw(m->maphdr, 0x20);
    m->dim1  = md_dw(m->maphdr, 0x24);
    m->dim2  = md_dw(m->maphdr, 0x28);
    m->count = md_dw(m->maphdr, 0x2c);

    /* cell array = dim2*dim1*dim0 * 0x1c (FUN_00587970:82-85).  Compute in 64
     * bits and reject anything that can't fit in the resource. */
    uint64_t cells = (uint64_t)m->dim0 * m->dim1 * m->dim2 * MD_CELL_SIZE;
    if (cells > len) { map_data_free(m); return -1; }
    m->cells_len = (size_t)cells;
    if (m->cells_len) {
        m->cells = malloc(m->cells_len);
        if (!m->cells) { map_data_free(m); return -1; }
        md_read(&c, m->cells, m->cells_len);
    }

    /* Reject an implausible layer count up front (a malformed header could
     * otherwise drive a huge calloc); each layer is at least MD_LAYER_HDR. */
    if ((uint64_t)m->count * MD_LAYER_HDR > len) { map_data_free(m); return -1; }
    if (m->count) {
        m->layers = calloc(m->count, sizeof *m->layers);
        if (!m->layers) { map_data_free(m); return -1; }
    }

    /* Per layer: read the 0x3c header, then the four sized sub-arrays
     * (FUN_00587970:93-144). */
    for (uint32_t i = 0; i < m->count; i++) {
        map_layer *L = &m->layers[i];
        md_read(&c, L->hdr, sizeof L->hdr);
        if (c.err) break;
        L->n_a = md_dw(L->hdr, 0x1c);
        L->n_b = md_dw(L->hdr, 0x20);
        L->n_c = md_dw(L->hdr, 0x24);
        L->n_d = md_dw(L->hdr, 0x28);
        L->a = md_read_sub(&c, L->n_a, 0x4);
        L->b = md_read_sub(&c, L->n_b, 0xc);
        L->c = md_read_sub(&c, L->n_c, 0x100);
        L->d = md_read_sub(&c, L->n_d, 0x8);
        if (c.err) break;
    }
    if (c.err) { map_data_free(m); return -1; }

    m->consumed = c.pos;
    return 0;
}

size_t map_data_cell_index(const map_data *m, uint32_t x, uint32_t y, uint32_t z)
{
    /* (dim1*z + y)*dim0 + x — FUN_00587e00:595, z-major (plane, row, col). */
    return ((size_t)m->dim1 * z + y) * m->dim0 + x;
}

int map_data_cell(const map_data *m, uint32_t x, uint32_t y, uint32_t z,
                  map_cell *out)
{
    if (!m->cells || x >= m->dim0 || y >= m->dim1 || z >= m->dim2)
        return -1;
    const uint8_t *p = m->cells + map_data_cell_index(m, x, y, z) * MD_CELL_SIZE;
    out->f00     = md_dw(p, 0x00);
    out->tile_id = md_dw(p, 0x04);
    out->f08     = md_dw(p, 0x08);
    out->arg_0c  = md_dw(p, 0x0c);
    out->shape   = md_dw(p, 0x10);
    out->arg_14  = md_dw(p, 0x14);
    out->arg_18  = md_dw(p, 0x18);
    return 0;
}

char *map_data_name(const map_data *m, char out[0x21])
{
    memcpy(out, m->maphdr, 0x20);
    out[0x20] = '\0';
    /* trim trailing spaces / NULs */
    for (int i = 0x1f; i >= 0 && (out[i] == ' ' || out[i] == '\0'); i--)
        out[i] = '\0';
    return out;
}
