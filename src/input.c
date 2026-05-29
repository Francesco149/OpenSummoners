/*
 * src/input.c — input-event ring poll (FUN_0043c110).
 *
 * See input.h for the subsystem framing and the record/manager layout.
 *
 * Retail (84 bytes), with raw offsets:
 *
 *     iVar2  = 0x3f;                       ; slot index, top of ring
 *     piVar3 = this + 0x108;               ; &ring[63]
 *     while ( *(*piVar3+0) != id           ; record.id  != polled id
 *          || *(*piVar3+8) != 1            ; record.flag != 1
 *          || (uint)(now - *(*piVar3+4)) > 100 ) {   ; record too old
 *         iVar2--; piVar3 -= 4;            ; walk down a slot
 *         if (iVar2 < 0) return 0;         ; whole ring scanned, miss
 *     }
 *     **(this + 0xc + iVar2*4) = 0;        ; ring[iVar2]->id = 0 (consume)
 *     return 1;
 *
 * The three compares are OR-folded into one loop condition (the loop
 * continues while *any* of "wrong id / not pressed / too old" holds, i.e.
 * stops on the first fully-matching slot).  The age test is unsigned, so
 * a record whose timestamp is "in the future" relative to `now` (e.g.
 * across a GetTickCount rollover, or a stale slot) wraps to a huge delta
 * and is rejected — identical to retail's `ja`.
 *
 * Note the scan order: it starts at the *top* slot (index 63, address
 * this+0x108) and decrements, so the highest-indexed matching record
 * wins.  We preserve that exactly; if two slots hold a matching event,
 * retail consumes the higher-indexed one.
 */
#include "input.h"

int input_poll_consume(input_mgr *m, uint32_t now, int32_t button_id)
{
    /* iVar2 = 0x3f; piVar3 = &ring[63] (this+0x108).  Walk down. */
    for (int i = INPUT_RING_LEN - 1; i >= 0; i--) {
        input_event *rec = m->ring[i];                 /* piVar1 = *piVar3 */

        if (rec->id == button_id                       /* *piVar1   == id  */
            && rec->flag == 1                          /* piVar1[2] == 1   */
            && (uint32_t)(now - rec->ts) <= 100) {     /* age <= 100 ms    */
            rec->id = 0;                               /* consume-on-read  */
            return 1;
        }
    }
    return 0;
}
