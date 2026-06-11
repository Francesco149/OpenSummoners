/*
 * portrait.c — the dialogue-portrait resolver (see portrait.h).
 */
#include "portrait.h"
#include "portrait_face_data.h"

#include <stddef.h>

int portrait_resolve(int32_t head_state, int32_t face_id, portrait_variant var)
{
    int n = 0;
    const portrait_face_rec *t = portrait_face_table(&n);
    if (t == NULL)
        return -1;
    for (int i = 0; i < n; i++) {
        if (t[i].head == head_state && t[i].face == face_id) {
            switch (var) {
            case PORTRAIT_VAR_A: return (int)t[i].var_a;
            case PORTRAIT_VAR_C: return (int)t[i].var_c;
            case PORTRAIT_VAR_B:
            default:             return (int)t[i].var_b;
            }
        }
    }
    return -1;   /* no record — retail's no-portrait path (0x49d6e0:132-134) */
}
