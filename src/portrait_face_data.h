/* portrait_face_data.h — see portrait_face_data.c (GENERATED). */
#ifndef OPENSUMMONERS_PORTRAIT_FACE_DATA_H
#define OPENSUMMONERS_PORTRAIT_FACE_DATA_H
#include <stdint.h>
/* One face-table record (retail DAT_006b6568, stride 0x10): the (head_state,
 * face_id) key + the 3 portrait-slot variants the 0x49d6e0 resolver picks among
 * by the speaker's anim-state (var_a=+0x8 when body facing==3, var_b=+0xa the
 * default, var_c=+0xc the bVar4 box-state path). */
typedef struct portrait_face_rec {
    int32_t  head;    /* speaker actor +0x1d8 head-state (key dword[0])   */
    int32_t  face;    /* face_id (0x49d6e0 param_8; key dword[1])         */
    uint16_t var_a;   /* +0x08 portrait slot (anim-state facing==3)       */
    uint16_t var_b;   /* +0x0a portrait slot (the default path)           */
    uint16_t var_c;   /* +0x0c portrait slot (the bVar4 box-state path)   */
} portrait_face_rec;
const portrait_face_rec *portrait_face_table(int *n);
#endif
