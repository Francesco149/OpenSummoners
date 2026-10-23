/* host_stubs.c — link stubs for the ports that live in Win32-only modules (engine_rest, sound_dev, script_io,
 * save_game: they include <windows.h>, so only the mingw builds compile them) but that host-linked modules call
 * (the chapter scripts, bound to those ports by decomp2c).  The host suite never runs those paths: each stub
 * aborts.  A new "undefined reference" from a host-linked module to a mingw-only port gets its stub here, with
 * the signature from the module's header. */
#include <stdio.h>
#include <stdlib.h>

#include "engine_rest.h"
#include "save_game.h"
#include "script_io.h"
#include "sound_dev.h"

#define STUB(name) do { fprintf(stderr, "host_stubs: %s is mingw-only\n", name); abort(); } while (0)

int32_t el_41c390(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, char * Format, int32_t a7, int32_t a8, int32_t a9, int32_t a10) { STUB("el_41c390"); }
void el_49d330(int32_t a1, int32_t a2) { STUB("el_49d330"); }
int32_t el_4675c0(wl_char *self, int32_t * a2, uint16_t a3) { STUB("el_4675c0"); }
int32_t el_467750(wl_char *self, int32_t a2, uint16_t a3) { STUB("el_467750"); }
int32_t el_49eea0(wl_char *self, int32_t a2, int32_t a3) { STUB("el_49eea0"); }
void el_4cc0e0(wl_char *self, int32_t a2) { STUB("el_4cc0e0"); }
int32_t sd_5bbc20(wl_char *self) { STUB("sd_5bbc20"); }
int32_t sd_5bbc60(wl_char *self, uint16_t a2) { STUB("sd_5bbc60"); }
int32_t sd_5bbc90(wl_char *self, uint16_t a2) { STUB("sd_5bbc90"); }
void sd_5bbcd0(wl_char *self, int32_t a2) { STUB("sd_5bbcd0"); }
int32_t sd_5bcb80(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6) { STUB("sd_5bcb80"); }
int32_t si_408730(wl_char *self, int32_t a2) { STUB("si_408730"); }
void si_40a5d0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6) { STUB("si_40a5d0"); }
int32_t si_41a160(wl_char *self) { STUB("si_41a160"); }
int32_t si_41d4a0(wl_char *self, int32_t a2, int32_t a3) { STUB("si_41d4a0"); }
int32_t sv_41a430(wl_char *self, int32_t a2, int32_t a3) { STUB("sv_41a430"); }
