/*
 * src/obj_container.c — generic engine container leaves (FUN_00412c10,
 * FUN_00414080).  See obj_container.h for the framing and layouts.
 *
 * FUN_00412c10 (46 bytes):
 *
 *     uVar1 = this->count;                         ; u16 at +0x4e
 *     if (this->capacity <= uVar1) return NULL;    ; u16 at +0x4c
 *     slot = this->slots[uVar1];                   ; (void**) at +0x48
 *     slot->owner = this;                          ; slot[+0]
 *     *(u16*)&slot->index = uVar1;                 ; low u16 of slot[+4]
 *     slot->field8 = 0;                            ; slot[+8]
 *     this->count = uVar1 + 1;
 *     return slot;
 *
 * Note the index store is a 16-bit write into the dword at slot+4, so the
 * top half (slot+6) is left as-is — modelled as the untouched _hi6.
 *
 * FUN_00414080 (63 bytes):
 *
 *     n = this->count;                             ; u16 at +0x06
 *     for (i = 0; i < n; i++) {                     ; array (void**) at +0x00
 *         entry = this->entries[i];
 *         entry->selected = (i == n - 1) ? 1 : 0;  ; entry[+8]
 *         n = this->count;                         ; re-read each iteration
 *     }
 *
 * The count re-read each pass is faithful to the disasm (it reloads the
 * u16 at +6 before the loop test); count cannot change mid-loop here, so
 * it is behaviourally a plain "mark index n-1, clear the rest".
 */
#include "obj_container.h"

pool_slot *obj_pool_acquire(obj_pool *p)
{
    uint16_t n = p->count;                  /* uVar1 = *(u16*)(this+0x4e) */
    if (p->capacity <= n) {                 /* *(u16*)(this+0x4c) <= uVar1 */
        return (pool_slot *)0;
    }
    pool_slot *slot = p->slots[n];          /* *(this->slots + n*4)       */
    slot->owner  = p;                       /* slot[0] = this             */
    slot->index  = n;                       /* *(u16*)(slot+4) = uVar1    */
    slot->field8 = 0;                       /* slot[2] = 0                */
    p->count = (uint16_t)(n + 1);           /* *(u16*)(this+0x4e)++       */
    return slot;
}

void sel_list_mark_last(sel_list *l)
{
    uint16_t n = l->count;                          /* *(u16*)(this+6) */
    for (uint16_t i = 0; i < n; i++) {
        sel_entry *e = l->entries[i];               /* *(this->entries + i*4) */
        e->selected = (i == (uint16_t)(n - 1));     /* 1 on last, else 0      */
        n = l->count;                               /* re-read count          */
    }
}
