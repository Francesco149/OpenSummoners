# OpenSummoners — port ledger

> **DERIVED FILE** — regenerate with `python3 tools/gen_port_ledger.py`.
> See `STATUS.md` for the headline.

Per-engine-function port status, derived from `functions.csv` (universe)
and `FUN_<va>` provenance references in `src/` (each port carries a
`FUN_<va>` comment). A function is **tested** when its src module
has a matching `tests/test_<stem>.c`. This is the answer to
*"is FUN_x done?"* at a glance.

## Summary

- engine-proper functions (below `0x5bdab0`): **1490** — the real port universe
- library tail (MSVC CRT, linked not ported): 268
- non-thunk engine functions total: 1758 (of 1768 incl. thunks)
- touched: **126** (7.8% of engine-proper) — tested 123, ported 3
- code bytes touched: **9.8%** (165,961 / 1,694,868 B of engine-proper)
- unported: **1632**
- orphan refs in src/ not in this table: 0

## tested (123) — ported + host unit suite

| VA | name | size | src |
|----|------|-----:|-----|
| 0x406440 | FUN_00406440 | 184 | cs_dispatch.c, cs_dispatch.h, cs_dispatch_win32.c |
| 0x408b90 | FUN_00408b90 | 419 | wnd_proc.h |
| 0x40e0c0 | FUN_0040e0c0 | 555 | menu_list.c, menu_list.h |
| 0x40f3e0 | FUN_0040f3e0 | 434 | menu_list.c, menu_list.h |
| 0x40f5c0 | FUN_0040f5c0 | 563 | menu_list.c, menu_list.h |
| 0x411f40 | FUN_00411f40 | 444 | menu_list.c, menu_list.h |
| 0x412c10 | FUN_00412c10 | 46 | obj_container.c, obj_container.h |
| 0x414080 | FUN_00414080 | 63 | obj_container.c, obj_container.h |
| 0x4178e0 | FUN_004178e0 | 194 | asset_register.c, asset_register.h, bitmap_session.c (+1) |
| 0x4179b0 | FUN_004179b0 | 415 | asset_register.c, asset_register.h, wnd_proc.h |
| 0x417b50 | FUN_00417b50 | 109 | asset_register.c, asset_register.h |
| 0x418470 | FUN_00418470 | 40 | asset_register.c, asset_register.h |
| 0x4192b0 | FUN_004192b0 | 52 | menu_list.c, menu_list.h |
| 0x426110 | FUN_00426110 | 610 | cs_dispatch.c, cs_dispatch.h, cs_dispatch_win32.c |
| 0x43c110 | FUN_0043c110 | 84 | input.c, input.h |
| 0x43ca40 | FUN_0043ca40 | 970 | menu_list.c, menu_list.h |
| 0x43ce50 | FUN_0043ce50 | 220 | menu_list.c, menu_list.h |
| 0x491770 | FUN_00491770 | 52 | asset_register.c, asset_register.h |
| 0x560900 | FUN_00560900 | 251 | cs_dispatch.c, cs_dispatch.h, cs_dispatch_win32.c |
| 0x562210 | FUN_00562210 | 688 | app_pump.h |
| 0x562a10 | FUN_00562a10 | 92 | asset_register.c, asset_register.h |
| 0x562ea0 | FUN_00562ea0 | 4062 | asset_register.c, asset_register.h, main.c (+3) |
| 0x563ef0 | FUN_00563ef0 | 445 | asset_register.c, asset_register.h |
| 0x56aea0 | FUN_0056aea0 | 3441 | input.h, main.c, obj_container.h (+3) |
| 0x56e190 | FUN_0056e190 | 26411 | asset_register.c, asset_register.h |
| 0x5748c0 | FUN_005748c0 | 237 | asset_register.c, asset_register.h |
| 0x5749b0 | FUN_005749b0 | 2342 | asset_register.c, asset_register.h |
| 0x579a00 | FUN_00579a00 | 451 | asset_register.c, asset_register.h |
| 0x579bd0 | FUN_00579bd0 | 741 | asset_register.c, asset_register.h |
| 0x579ec0 | FUN_00579ec0 | 126 | asset_register.c, asset_register.h |
| 0x579f40 | FUN_00579f40 | 214 | asset_register.c, asset_register.h |
| 0x57a030 | FUN_0057a030 | 340 | asset_register.c, asset_register.h |
| 0x57a1a0 | FUN_0057a1a0 | 189 | asset_register.c, asset_register.h |
| 0x57a260 | FUN_0057a260 | 195 | asset_register.c, asset_register.h |
| 0x57a330 | FUN_0057a330 | 3919 | asset_register.c, asset_register.h |
| 0x57b280 | FUN_0057b280 | 6073 | asset_register.c, asset_register.h |
| 0x57ca40 | FUN_0057ca40 | 24884 | asset_register.c, asset_register.h |
| 0x582b80 | FUN_00582b80 | 384 | asset_register.c, asset_register.h |
| 0x582d00 | FUN_00582d00 | 15 | asset_register.c, asset_register.h |
| 0x582d10 | FUN_00582d10 | 379 | asset_register.c, asset_register.h |
| 0x582e90 | FUN_00582e90 | 3560 | cs_dispatch.c, cs_dispatch.h, main.c (+2) |
| 0x586010 | FUN_00586010 | 6133 | asset_register.c, asset_register.h |
| 0x587e00 | FUN_00587e00 | 18055 | asset_register.h |
| 0x58ffa0 | FUN_0058ffa0 | 23 | wnd_proc.h, wnd_proc_win32.c |
| 0x5a4770 | FUN_005a4770 | 45963 | app_pump.h, main.c |
| 0x5b1030 | FUN_005b1030 | 156 | app_pump.c, app_pump.h, title_scene.c (+2) |
| 0x5b12e0 | FUN_005b12e0 | 441 | wnd_proc.c, wnd_proc.h |
| 0x5b14c0 | FUN_005b14c0 | 287 | wnd_proc.h, wnd_proc_win32.c |
| 0x5b5ac0 | FUN_005b5ac0 | 39 | zdd.h, zdd_win32.c |
| 0x5b5d90 | FUN_005b5d90 | 33 | asset_register.c, asset_register.h |
| 0x5b5f50 | FUN_005b5f50 | 71 | asset_register.c, asset_register.h |
| 0x5b6e70 | FUN_005b6e70 | 9 | bitmap_session.h |
| 0x5b6e90 | FUN_005b6e90 | 24 | bitmap_session.h |
| 0x5b6f00 | FUN_005b6f00 | 7 | bitmap_session.h |
| 0x5b6f10 | FUN_005b6f10 | 26 | bitmap_session.h |
| 0x5b71f0 | FUN_005b71f0 | 117 | bitmap_session.c, bitmap_session.h, bitmap_session_win32.c |
| 0x5b7800 | FUN_005b7800 | 359 | asset_register.h, bitmap_session.c, bitmap_session.h |
| 0x5b7b90 | FUN_005b7b90 | 53 | bitmap_session.h |
| 0x5b7c10 | FUN_005b7c10 | 186 | bitmap_session.c, bitmap_session.h |
| 0x5b7ee0 | FUN_005b7ee0 | 153 | main.c, zdd.h |
| 0x5b7f80 | FUN_005b7f80 | 94 | zdd.c, zdd.h |
| 0x5b7fe0 | FUN_005b7fe0 | 90 | zdd.h |
| 0x5b8040 | FUN_005b8040 | 139 | zdd.h |
| 0x5b80d0 | FUN_005b80d0 | 826 | zdd.c, zdd.h |
| 0x5b8480 | FUN_005b8480 | 1088 | zdd.c, zdd.h, zdd_win32.c |
| 0x5b88c0 | FUN_005b88c0 | 57 | zdd.h, zdd_win32.c |
| 0x5b8900 | FUN_005b8900 | 74 | cs_dispatch.c, zdd.h, zdd_win32.c |
| 0x5b8950 | FUN_005b8950 | 126 | zdd.h, zdd_win32.c |
| 0x5b89d0 | FUN_005b89d0 | 71 | main.c, zdd.h, zdd_win32.c |
| 0x5b8a20 | FUN_005b8a20 | 181 | zdd.c, zdd.h, zdd_win32.c |
| 0x5b8ae0 | FUN_005b8ae0 | 30 | pixel_drawer.c, pixel_drawer.h |
| 0x5b8b00 | FUN_005b8b00 | 59 | zdd.c, zdd.h |
| 0x5b8b40 | FUN_005b8b40 | 184 | cs_dispatch.c, zdd.c, zdd.h |
| 0x5b8c00 | FUN_005b8c00 | 372 | zdd.c, zdd.h, zdd_win32.c |
| 0x5b8da0 | FUN_005b8da0 | 33 | zdd.h |
| 0x5b8dd0 | FUN_005b8dd0 | 33 | cs_dispatch.c, zdd.h |
| 0x5b8e00 | FUN_005b8e00 | 157 | zdd.h, zdd_win32.c |
| 0x5b8ea0 | FUN_005b8ea0 | 285 | zdd.c, zdd.h |
| 0x5b8fc0 | FUN_005b8fc0 | 335 | call_trace.h, main.c, title_scene.h (+3) |
| 0x5b9130 | FUN_005b9130 | 151 | main.c, wnd_proc.h, wnd_proc_win32.c (+3) |
| 0x5b91d0 | FUN_005b91d0 | 109 | zdd.h |
| 0x5b9240 | FUN_005b9240 | 60 | zdd.h |
| 0x5b9350 | FUN_005b9350 | 50 | zdd.c, zdd.h |
| 0x5b9390 | FUN_005b9390 | 75 | cs_dispatch.c, zdd.h, zdd_win32.c |
| 0x5b93e0 | FUN_005b93e0 | 42 | zdd.h, zdd_win32.c |
| 0x5b9410 | FUN_005b9410 | 49 | zdd.c, zdd.h |
| 0x5b9490 | FUN_005b9490 | 63 | zdd.c, zdd.h, zdd_win32.c |
| 0x5b94d0 | FUN_005b94d0 | 15 | zdd.c, zdd.h, zdd_win32.c |
| 0x5b94e0 | FUN_005b94e0 | 29 | wnd_proc.h, zdd.c, zdd.h (+1) |
| 0x5b9500 | FUN_005b9500 | 17 | wnd_proc.h, zdd.c, zdd.h (+1) |
| 0x5b9520 | FUN_005b9520 | 157 | zdd.c, zdd.h, zdd_win32.c |
| 0x5b95c0 | FUN_005b95c0 | 110 | zdd.c, zdd.h |
| 0x5b9740 | FUN_005b9740 | 153 | zdd.h, zdd_win32.c |
| 0x5b97e0 | FUN_005b97e0 | 66 | zdd.c, zdd.h |
| 0x5b9830 | FUN_005b9830 | 138 | zdd.c, zdd.h, zdd_win32.c |
| 0x5b98c0 | FUN_005b98c0 | 73 | zdd.c, zdd.h |
| 0x5b9a40 | FUN_005b9a40 | 112 | zdd.c, zdd.h |
| 0x5b9ab0 | FUN_005b9ab0 | 10 | zdd.h |
| 0x5b9ac0 | FUN_005b9ac0 | 22 | zdd.c, zdd.h |
| 0x5b9ae0 | FUN_005b9ae0 | 140 | zdd.c, zdd.h |
| 0x5b9b70 | FUN_005b9b70 | 122 | zdd.c, zdd.h |
| 0x5ba290 | FUN_005ba290 | 30 | wnd_proc.h, wnd_proc_win32.c |
| 0x5bad20 | FUN_005bad20 | 26 | wnd_proc.h |
| 0x5bad40 | FUN_005bad40 | 26 | wnd_proc.h |
| 0x5bae30 | FUN_005bae30 | 60 | wnd_proc.h |
| 0x5bae70 | FUN_005bae70 | 30 | wnd_proc.h |
| 0x5bbd20 | FUN_005bbd20 | 140 | wnd_proc.h, wnd_proc_win32.c |
| 0x5bcff0 | FUN_005bcff0 | 42 | pixel_drawer.c, pixel_drawer.h |
| 0x5bd020 | FUN_005bd020 | 26 | pixel_drawer.c, pixel_drawer.h |
| 0x5bd040 | FUN_005bd040 | 801 | pixel_drawer.c, pixel_drawer.h |
| 0x5bd380 | FUN_005bd380 | 39 | pixel_drawer.c, pixel_drawer.h |
| 0x5bd3b0 | FUN_005bd3b0 | 30 | pixel_drawer.c, pixel_drawer.h |
| 0x5bd3d0 | FUN_005bd3d0 | 241 | pixel_drawer.c, pixel_drawer.h |
| 0x5bd4d0 | FUN_005bd4d0 | 71 | pixel_drawer.c, pixel_drawer.h |
| 0x5bd550 | FUN_005bd550 | 302 | zdd.c, zdd.h, zdd_win32.c |
| 0x5bd680 | FUN_005bd680 | 1072 | zdd.c, zdd.h |
| 0x5bef0e | FUN_005bef0e | 11 | asset_register.c, asset_register.h, cs_dispatch.c (+3) |
| 0x5bf4e8 | FUN_005bf4e8 | 19 | cs_dispatch_win32.c, wnd_proc.h |
| 0x5bf5db | FUN_005bf5db | 17 | app_pump.c, app_pump.h, app_pump_win32.c (+5) |
| 0x5bf5fd | FUN_005bf5fd | 153 | wnd_proc.h |
| 0x5c0907 | FUN_005c0907 | 45 | zdd.c, zdd.h |
| 0x5c0934 | FUN_005c0934 | 92 | zdd.c |
| 0x5c123f | FUN_005c123f | 47 | asset_register.c |

## ported (3) — reimplemented, no host test yet

| VA | name | size | src |
|----|------|-----:|-----|
| 0x5bf3ee | FUN_005bf3ee | 82 | cs_dispatch_win32.c |
| 0x5bf440 | FUN_005bf440 | 86 | cs_dispatch_win32.c |
| 0x5bf496 | FUN_005bf496 | 50 | cs_dispatch_win32.c |

