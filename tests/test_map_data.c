/*
 * test_map_data.c — host tests for the runtime map-data parser (FUN_00587970,
 * src/map_data.c).
 *
 * The parser logic is exercised here against hand-built synthetic blobs (small
 * dims + a couple of layers with distinct sub-array shapes), which lets the
 * tests assert every read offset + the exact byte consumption without bundling
 * the 152 KB real resource.  The REAL DATA-1022 town map (88x19x3, 86 layers)
 * is validated separately + reproducibly by tools/extract/map_data.py against
 * vendor/unpacked/sotes.unpacked.exe (consumed == 152936, name
 * "MSD_SOTES_MAPDATA").
 */
#include "map_data.h"
#include "t.h"

static void put_u32(uint8_t *p, unsigned off, uint32_t v)
{
    p[off + 0] = (uint8_t)(v);
    p[off + 1] = (uint8_t)(v >> 8);
    p[off + 2] = (uint8_t)(v >> 16);
    p[off + 3] = (uint8_t)(v >> 24);
}

/* Build a synthetic map-data blob into a caller buffer; returns total length.
 * dims = 2x3x1 (6 cells * 0x1c = 168), 2 layers:
 *   layer0: n_a=2 (+0x1c), n_b=1 (+0x20)        -> sub = 2*4 + 1*0xc   = 20
 *   layer1: n_c=1 (+0x24), n_d=2 (+0x28)        -> sub = 1*0x100 + 2*8 = 272
 * Cells + sub-arrays are filled with index-derived marker bytes so the test can
 * confirm the copy is correct and owns its own memory. */
#define SYN_CELLS (2u * 3u * 1u * MD_CELL_SIZE)            /* 168 */
#define SYN_L0    (2u * 4u + 1u * 0xcu)                    /* 20  */
#define SYN_L1    (1u * 0x100u + 2u * 8u)                  /* 272 */
#define SYN_LEN   (0x68u + SYN_CELLS \
                   + (MD_LAYER_HDR + SYN_L0) \
                   + (MD_LAYER_HDR + SYN_L1))

static size_t build_synthetic(uint8_t *b)
{
    memset(b, 0, SYN_LEN);
    put_u32(b, 0x00, 0x30);                 /* magic                         */
    /* header [0x04:0x34] left as filler 0xAB to prove it's preserved        */
    memset(b + 0x04, 0xAB, 0x30);
    /* maphdr [0x34:0x68] */
    memcpy(b + 0x34, "TESTMAP", 7);         /* name, space/NUL padded        */
    put_u32(b, 0x34 + 0x20, 2);             /* dim0                          */
    put_u32(b, 0x34 + 0x24, 3);             /* dim1                          */
    put_u32(b, 0x34 + 0x28, 1);             /* dim2                          */
    put_u32(b, 0x34 + 0x2c, 2);             /* count                         */

    size_t pos = 0x68;
    for (unsigned i = 0; i < SYN_CELLS; i++) b[pos + i] = (uint8_t)(0x40 + (i & 0x3f));
    pos += SYN_CELLS;

    /* layer 0 header + sub-arrays */
    put_u32(b, pos + 0x00, 0xA0);           /* hdr dword0 (an id)            */
    put_u32(b, pos + 0x1c, 2);              /* n_a                           */
    put_u32(b, pos + 0x20, 1);              /* n_b                           */
    /* n_c, n_d = 0 */
    {
        size_t sub = pos + MD_LAYER_HDR;
        for (unsigned i = 0; i < SYN_L0; i++) b[sub + i] = (uint8_t)(0x10 + i);
        pos = sub + SYN_L0;
    }

    /* layer 1 header + sub-arrays */
    put_u32(b, pos + 0x00, 0xB1);
    put_u32(b, pos + 0x24, 1);              /* n_c                           */
    put_u32(b, pos + 0x28, 2);              /* n_d                           */
    {
        size_t sub = pos + MD_LAYER_HDR;
        for (unsigned i = 0; i < SYN_L1; i++) b[sub + i] = (uint8_t)(0x80 + (i & 0x7f));
        pos = sub + SYN_L1;
    }
    return pos;
}

int test_map_data_header(void)
{
    uint8_t b[SYN_LEN];
    size_t n = build_synthetic(b);
    T_ASSERT_EQ_U(n, SYN_LEN);

    map_data m;
    T_ASSERT_EQ_I(map_data_parse(&m, b, n), 0);

    T_ASSERT_EQ_U(m.magic, 0x30);
    T_ASSERT_EQ_U(m.dim0, 2);
    T_ASSERT_EQ_U(m.dim1, 3);
    T_ASSERT_EQ_U(m.dim2, 1);
    T_ASSERT_EQ_U(m.count, 2);
    T_ASSERT_EQ_U(m.cells_len, SYN_CELLS);
    /* parse consumes the resource exactly, the trustworthiness invariant */
    T_ASSERT_EQ_U(m.consumed, n);

    char name[0x21];
    T_ASSERT(strcmp(map_data_name(&m, name), "TESTMAP") == 0);
    /* opaque header block preserved verbatim */
    for (int i = 0; i < 0x30; i++) T_ASSERT_EQ_U(m.header[i], 0xAB);

    map_data_free(&m);
    return 0;
}

int test_map_data_cells_copied(void)
{
    uint8_t b[SYN_LEN];
    size_t n = build_synthetic(b);
    map_data m;
    T_ASSERT_EQ_I(map_data_parse(&m, b, n), 0);

    /* cells are an owned COPY: bytes match, but clobbering the source buffer
     * must not change the parsed cells. */
    for (unsigned i = 0; i < SYN_CELLS; i++)
        T_ASSERT_EQ_U(m.cells[i], (uint8_t)(0x40 + (i & 0x3f)));
    memset(b, 0, n);
    T_ASSERT_EQ_U(m.cells[0], 0x40);
    T_ASSERT_EQ_U(m.cells[SYN_CELLS - 1], (uint8_t)(0x40 + ((SYN_CELLS - 1) & 0x3f)));

    map_data_free(&m);
    return 0;
}

int test_map_data_layers(void)
{
    uint8_t b[SYN_LEN];
    size_t n = build_synthetic(b);
    map_data m;
    T_ASSERT_EQ_I(map_data_parse(&m, b, n), 0);

    /* layer 0: a=2, b=1, c=d=0 */
    const map_layer *L0 = &m.layers[0];
    T_ASSERT_EQ_U(L0->n_a, 2);
    T_ASSERT_EQ_U(L0->n_b, 1);
    T_ASSERT_EQ_U(L0->n_c, 0);
    T_ASSERT_EQ_U(L0->n_d, 0);
    T_ASSERT(L0->a != NULL && L0->b != NULL);
    T_ASSERT_EQ_P(L0->c, NULL);   /* zero count -> NULL pointer arm           */
    T_ASSERT_EQ_P(L0->d, NULL);
    T_ASSERT_EQ_U(L0->a[0], 0x10);              /* first sub-array byte        */
    /* b array starts after a (2*4 = 8 bytes) */
    T_ASSERT_EQ_U(L0->b[0], (uint8_t)(0x10 + 8));

    /* layer 1: c=1, d=2, a=b=0 */
    const map_layer *L1 = &m.layers[1];
    T_ASSERT_EQ_U(L1->n_a, 0);
    T_ASSERT_EQ_U(L1->n_b, 0);
    T_ASSERT_EQ_U(L1->n_c, 1);
    T_ASSERT_EQ_U(L1->n_d, 2);
    T_ASSERT_EQ_P(L1->a, NULL);
    T_ASSERT_EQ_P(L1->b, NULL);
    T_ASSERT(L1->c != NULL && L1->d != NULL);
    T_ASSERT_EQ_U(L1->c[0], 0x80);
    /* d array starts after c (0x100 bytes) */
    T_ASSERT_EQ_U(L1->d[0], (uint8_t)(0x80 + (0x100 & 0x7f)));

    map_data_free(&m);
    return 0;
}

int test_map_data_truncated(void)
{
    uint8_t b[SYN_LEN];
    size_t n = build_synthetic(b);
    map_data m;

    /* truncate mid-resource: every prefix shorter than a layer's sub-arrays
     * must be rejected (overrun guard), not read past the buffer. */
    T_ASSERT_EQ_I(map_data_parse(&m, b, n - 1), -1);
    map_data_free(&m);   /* must be safe after a failed parse */

    /* truncate inside the fixed header */
    T_ASSERT_EQ_I(map_data_parse(&m, b, 0x40), -1);
    map_data_free(&m);

    /* a header claiming dims that overflow the buffer is rejected */
    uint8_t c[SYN_LEN];
    build_synthetic(c);
    put_u32(c, 0x34 + 0x20, 0x10000);   /* dim0 huge -> cells > len           */
    T_ASSERT_EQ_I(map_data_parse(&m, c, SYN_LEN), -1);
    map_data_free(&m);
    return 0;
}
