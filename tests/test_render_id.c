/*
 * tests/test_render_id.c — host tests for src/render_id.c.
 *
 * The registry is the cross-side identity backbone of the DDraw blit trace:
 * register at the resolver chokepoint, look up at each blit.  These tests
 * exercise register/lookup/forget/overwrite + the recycled-pointer hazard
 * (a forgotten-then-reused address must not carry the stale identity), the
 * resource_id→sheet-hash side table, and the FNV-1a fingerprint properties.
 */
#include "../src/render_id.h"
#include "t.h"

#include <string.h>

int test_render_id_register_and_lookup(void)
{
    render_id_reset();
    int dummy_a, dummy_b;
    const void *cel_a = &dummy_a, *cel_b = &dummy_b;

    render_id_register(cel_a, 0x55, 3, 0xdeadbeef);
    render_id_register(cel_b, 0x58, 0, 0x12345678);

    render_id id;
    T_ASSERT(render_id_lookup(cel_a, &id) == 1);
    T_ASSERT_EQ_U(id.resource_id, 0x55);
    T_ASSERT_EQ_U(id.frame, 3);
    T_ASSERT_EQ_U(id.dhash, 0xdeadbeef);

    T_ASSERT(render_id_lookup(cel_b, &id) == 1);
    T_ASSERT_EQ_U(id.resource_id, 0x58);
    T_ASSERT_EQ_U(id.dhash, 0x12345678);

    /* unknown pointer */
    int dummy_c;
    T_ASSERT(render_id_lookup(&dummy_c, &id) == 0);
    /* NULL is never registered */
    T_ASSERT(render_id_lookup(NULL, &id) == 0);
    return 0;
}

int test_render_id_overwrite(void)
{
    render_id_reset();
    int dummy;
    const void *cel = &dummy;

    render_id_register(cel, 0x55, 1, 0xaaaa);
    render_id_register(cel, 0x100, 7, 0xbbbb);   /* same ptr, new identity */

    render_id id;
    T_ASSERT(render_id_lookup(cel, &id) == 1);
    T_ASSERT_EQ_U(id.resource_id, 0x100);
    T_ASSERT_EQ_U(id.frame, 7);
    T_ASSERT_EQ_U(id.dhash, 0xbbbb);
    return 0;
}

int test_render_id_forget_and_recycle(void)
{
    render_id_reset();
    int dummy;
    const void *cel = &dummy;

    render_id_register(cel, 0x55, 1, 0xaaaa);
    render_id_forget(cel);

    render_id id;
    T_ASSERT(render_id_lookup(cel, &id) == 0);   /* gone */

    /* The recycled-pointer hazard: the SAME address is handed back by the
     * allocator for a different sprite.  A stale lookup must not survive a
     * forget — and a fresh register must take. */
    render_id_register(cel, 0x99, 4, 0xcccc);
    T_ASSERT(render_id_lookup(cel, &id) == 1);
    T_ASSERT_EQ_U(id.resource_id, 0x99);
    T_ASSERT_EQ_U(id.frame, 4);
    return 0;
}

int test_render_id_forget_probes_past_tombstone(void)
{
    /* Two pointers that may or may not collide; forget the first and make
     * sure the second is still reachable (the tombstone must not terminate
     * the probe chain). */
    render_id_reset();
    int slots[8];
    for (int i = 0; i < 8; i++)
        render_id_register(&slots[i], (uint16_t)(0x40 + i), (uint16_t)i, (uint32_t)i);

    render_id_forget(&slots[0]);

    render_id id;
    for (int i = 1; i < 8; i++) {
        T_ASSERT(render_id_lookup(&slots[i], &id) == 1);
        T_ASSERT_EQ_U(id.resource_id, 0x40 + i);
    }
    T_ASSERT(render_id_lookup(&slots[0], &id) == 0);
    return 0;
}

int test_render_id_sheet_hash_table(void)
{
    render_id_reset_sheets();
    T_ASSERT_EQ_U(render_id_sheet_hash(0x55), 0);   /* unknown */

    render_id_set_sheet_hash(0x55, 0xfeedface);
    render_id_set_sheet_hash(0x58, 0x0badf00d);
    T_ASSERT_EQ_U(render_id_sheet_hash(0x55), 0xfeedface);
    T_ASSERT_EQ_U(render_id_sheet_hash(0x58), 0x0badf00d);

    /* overwrite (a re-decode produces a fresh digest) */
    render_id_set_sheet_hash(0x55, 0x11112222);
    T_ASSERT_EQ_U(render_id_sheet_hash(0x55), 0x11112222);

    /* resource_id 0 is never a real bank — set/get are no-ops */
    render_id_set_sheet_hash(0, 0x99999999);
    T_ASSERT_EQ_U(render_id_sheet_hash(0), 0);
    return 0;
}

int test_render_id_fnv1a_known_vector(void)
{
    /* FNV-1a 32-bit reference vectors. */
    T_ASSERT_EQ_U(render_id_hash("", 0), 2166136261u);          /* empty = offset basis */
    T_ASSERT_EQ_U(render_id_hash("a", 1), 0xe40c292cu);
    T_ASSERT_EQ_U(render_id_hash("foobar", 6), 0xbf9cf968u);
    return 0;
}

int test_render_id_fnv1a_sensitive_and_seeded(void)
{
    /* A single changed pixel byte must change the digest — the whole point
     * of the decode fingerprint (catch "right sprite, wrong pixels"). */
    uint8_t a[64], b[64];
    for (int i = 0; i < 64; i++) { a[i] = (uint8_t)i; b[i] = (uint8_t)i; }
    b[37] ^= 0x01;
    T_ASSERT(render_id_hash(a, 64) != render_id_hash(b, 64));

    /* Same bytes, different geometry seed → different digest (so a sheet
     * reinterpreted at a different pitch hashes apart). */
    uint32_t h1 = render_id_hash_seed(640u, a, 64);
    uint32_t h2 = render_id_hash_seed(320u, a, 64);
    T_ASSERT(h1 != h2);

    /* Determinism. */
    T_ASSERT_EQ_U(render_id_hash(a, 64), render_id_hash(a, 64));
    return 0;
}
