/*
 * script_helpers.{c,h} — the chapter scripts' (src/chapter_scripts.c) callees with no file / sound / DirectX call
 * below them, on the retail layout (rhost-verified inside retail): scene / script commands, the flag table, the
 * runner's small getters.  Names by VA.
 *
 *   FUN_00401d80  sc_401d80   FUN_00402030  sc_402030   FUN_004020a0  sc_4020a0   FUN_004024e0  sc_4024e0
 *   FUN_00402610  sc_402610   FUN_00402f40  sc_402f40   FUN_00406500  sc_406500   FUN_00406ab0  sc_406ab0
 *   FUN_00406c70  sc_406c70   FUN_0041a3a0  sc_41a3a0   FUN_0041e2b0  sc_41e2b0   FUN_0041e340  sc_41e340
 *   FUN_0041eb30  sc_41eb30   FUN_0041ece0  sc_41ece0   FUN_0041efe0  sc_41efe0   FUN_0041f050  sc_41f050
 *   FUN_00439510  sc_439510   FUN_00439560  sc_439560   FUN_0049cf60  sc_49cf60   FUN_0049cf80  sc_49cf80
 *   FUN_0049e250  sc_49e250   FUN_0049e6c0  sc_49e6c0   FUN_004a5e40  sc_4a5e40   FUN_004a5e70  sc_4a5e70
 *   FUN_004a5ea0  sc_4a5ea0   FUN_004a5ec0  sc_4a5ec0   FUN_004a5f80  sc_4a5f80   FUN_004a63d0  sc_4a63d0
 *   FUN_004ac610  sc_4ac610   FUN_004ac660  sc_4ac660   FUN_004b3b10  sc_4b3b10   FUN_004b3f30  sc_4b3f30
 *   FUN_00402f60  sc_402f60   FUN_004b4570  sc_4b4570   FUN_004bbe00  sc_4bbe00   FUN_004bbe40  sc_4bbe40
 *   FUN_004bbe80  sc_4bbe80   FUN_004c5300  sc_4c5300   FUN_004cbfb0  sc_4cbfb0   FUN_004cbfd0  sc_4cbfd0
 *   FUN_0041f000  sc_41f000   FUN_004cbff0  sc_4cbff0   FUN_004cc010  sc_4cc010   FUN_004cc030  sc_4cc030
 *   FUN_004cc250  sc_4cc250   FUN_004cc470  sc_4cc470   FUN_004cca20  sc_4cca20   FUN_004cca50  sc_4cca50
 *   FUN_004d6490  sc_4d6490   FUN_005b4330  sc_5b4330
 *
 * Reached by the rhost corpus, shadow-verified per call: sc_401d80 sc_402030 sc_4020a0 sc_4024e0 sc_406500
 * sc_41e2b0 sc_41e340 sc_41ece0 sc_41efe0 sc_49e250 sc_4a63d0.  The rest are converted (the same decomp2c pass)
 * but no corpus scenario reaches them yet: UNVERIFIED until one does.
 */
#ifndef OSS_SCRIPT_HELPERS_H
#define OSS_SCRIPT_HELPERS_H

#include <stdint.h>

#include "world_layout.h"

int32_t sc_401d80(int32_t a1, int32_t a2, int32_t a3);
int32_t sc_402030(wl_char *self);
void sc_4020a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9);
void sc_4024e0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
void sc_402610(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t sc_402f40(wl_char *self);
int32_t sc_406500(wl_char *self, int32_t a2, int32_t a3);
int32_t sc_406ab0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t sc_406c70(int32_t a1, int32_t a2, int32_t a3, uint16_t a4, int32_t a5);
int32_t sc_41a3a0(wl_char *self, int32_t a2);
int32_t sc_41e2b0(wl_char *self, int32_t a2);
int32_t sc_41e340(int32_t a1, int32_t a2);
void sc_41eb30(int32_t a1);
int32_t sc_41ece0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t sc_41efe0(int32_t a1, int32_t a2);
int32_t sc_41f050(int32_t a1, int32_t a2);
int32_t sc_439510(wl_char *self, int32_t a2);
int32_t sc_439560(wl_char *self, int32_t a2);
int32_t sc_49cf60(int32_t a1);
void sc_49cf80(int32_t a1, int32_t a2, int32_t a3, int32_t a4);
int32_t sc_49e250(wl_char *self);
int32_t sc_49e6c0(wl_char *self);
int32_t sc_4a5e40(intptr_t ecx_, int32_t a1, int32_t a2, int32_t a3);
int32_t sc_4a5e70(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t sc_4a5ea0(wl_char *self);
int32_t sc_4a5ec0(wl_char *self);
int32_t sc_4a5f80(wl_char *self, int32_t a2);
int32_t sc_4a63d0(wl_char *self, int32_t a2);
int32_t sc_4ac610(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t sc_4ac660(wl_char *self, int32_t a2, char * a3);
int32_t sc_4b3b10(void);
int32_t sc_4b3f30(wl_char *self);
int32_t sc_402f60(wl_char *self, char * a2, char * a3);
int32_t sc_4b4570(wl_char *self);
void sc_4bbe00(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
void sc_4bbe40(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9);
int32_t sc_4bbe80(int32_t * a1, int32_t * a2);
void sc_4c5300(int32_t a1);
int32_t sc_4cbfb0(intptr_t ecx_, int32_t a1, int32_t a2);
int32_t sc_4cbfd0(intptr_t ecx_, int32_t a1, int32_t a2);
int32_t sc_41f000(int32_t a1, int32_t a2);
int32_t sc_4cbff0(wl_char *self, int32_t a2, int32_t a3);
int32_t sc_4cc010(wl_char *self, int32_t a2, int32_t a3);
int32_t sc_4cc030(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t sc_4cc250(wl_char *self, int32_t a2, int32_t a3);
int32_t sc_4cc470(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t sc_4cca20(wl_char *self, int32_t a2, int32_t a3);
int32_t sc_4cca50(wl_char *self, int32_t a2, int16_t a3);
int32_t sc_4d6490(intptr_t ecx_, int32_t a1, int32_t a2);
int32_t sc_5b4330(int32_t a1, int32_t a2, int32_t a3, int32_t a4);

#endif /* OSS_SCRIPT_HELPERS_H */
