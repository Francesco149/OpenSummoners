/*
 * src/osr_recon.h — the ".osr" frame RECONSTRUCTOR (Trace Studio v2 M4b/M4c).
 *
 * The Windows side of --osr-replay: given a booted DDraw god-object (a `zdd`
 * with a lockable/DC-able dest surface) and a captured `.osr`, replay the
 * recorded draw stream through the port's OWN bit-exact sinks — the zdd.c blit
 * primitives + real GDI TextOutA — to rebuild each frame 1:1, then snapshot the
 * wanted frames to BMP (the studio's Python side converts BMP→PNG, the
 * established port-frame workflow).
 *
 * Win32-only (it #includes <windows.h> + drives DDraw/GDI), so it is NOT in the
 * host unit suite — the pure record PARSE is host-tested via src/osr_replay.c.
 * This module is the SINK that turns parsed records into pixels.
 */
#ifndef OPENSUMMONERS_OSR_RECON_H
#define OPENSUMMONERS_OSR_RECON_H

#include <stddef.h>

#include "zdd.h"

/* Reconstruct frames from `osr_path` onto `dest` (the lockable offscreen surface
 * — g_zdd->primary_obj after a windowed-mode boot), writing BMPs named
 * `recon_<flip>_t<tick>.bmp` into `out_dir`.  `z` is the DDraw god-object used
 * to allocate source surfaces.  If `want`/`n_want` is non-empty, only those
 * FLIP frames are rendered+snapshotted (the others still stream so their
 * first-sighting SHEET/FONT records build the persistent tables); an empty list
 * renders every frame.  Returns an OSR_REPLAY_* code from the underlying stream.
 * Frees all surfaces/fonts it created before returning. */
int osr_recon_run(zdd *z, zdd_object *dest, const char *out_dir,
                  const char *osr_path,
                  const unsigned *want, size_t n_want);

#endif /* OPENSUMMONERS_OSR_RECON_H */
