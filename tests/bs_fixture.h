/*
 * tests/bs_fixture.h — shared bitmap_session PE-resource fixtures.
 *
 * test_bitmap_session.c owns the host-side stubs for the Win32 resource
 * primitives (bs_load_pe_resource backed by a small in-memory directory)
 * plus the synthetic compressed-resource builder.  These thin non-static
 * wrappers expose just enough of that machinery for OTHER test TUs (e.g.
 * test_asset_register.c's end-to-end ar_sprite_decode test) to register a
 * fake "DATA" resource without duplicating the builder.
 *
 * All definitions live in test_bitmap_session.c.
 */
#ifndef OPENSUMMONERS_TESTS_BS_FIXTURE_H
#define OPENSUMMONERS_TESTS_BS_FIXTURE_H

#include <stdint.h>

/* Clear the stub PE-resource directory + reset the live-alloc counter. */
void bs_fixture_reset(void);

/* Register one row in the stub directory; bs_load_pe_resource matches on
 * (hModule, id, type) exactly. */
void bs_fixture_register(void *hModule, uint16_t id, const char *type,
                         const void *data);

/* Build a synthetic *compressed* resource (the format bs_decode_resource
 * reads with compressed_flag=1) into a shared static buffer and return a
 * pointer to it.  Pixel data lives at `return + pixel_off + 0x458`; the
 * caller writes the pixel bytes there after calling this.  pixel_off must
 * be <= 0x80.  Returns a non-const pointer so the caller can stamp pixels. */
uint8_t *bs_fixture_build_compressed(uint32_t width, uint32_t height,
                                     uint16_t bit_count, uint32_t pixel_off);

#endif /* OPENSUMMONERS_TESTS_BS_FIXTURE_H */
