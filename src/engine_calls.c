/* engine_calls.c — the slot table (see engine_calls.h).  The port binary fills the roots
 * and any slot whose callee it has ported; rhost fills every slot with the retail VA. */
#include "engine_calls.h"

engine_calls g_eng;
int32_t (*g_undef_read)(int site, int k, int32_t port_value);
