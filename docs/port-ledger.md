# OpenSummoners — port ledger

> **DERIVED FILE** — regenerate with `python3 tools/gen_port_ledger.py`.
> See `STATUS.md` for the headline.

Per-engine-function port status, derived from `functions.csv` (universe)
and `FUN_<va>` provenance references in `src/` (each port carries a
`FUN_<va>` comment). A function is **tested** when its src module
has a matching `tests/test_<stem>.c`. This is the answer to
*"is FUN_x done?"* at a glance.

## Summary

- engine-proper functions (below `0x5bdab0`): **1532** — the real port universe
- library tail (MSVC CRT, linked not ported): 292
- non-thunk engine functions total: 1824 (of 1824 incl. thunks)
- touched: **1074** (69.3% of engine-proper) — tested 249, ported 825
- code bytes touched: **61.6%** (1,093,848 / 1,776,526 B of engine-proper)
- unported: **750**
- orphan refs in src/ not in this table: 0

## tested (249) — ported + host unit suite

| VA | name | size | src |
|----|------|-----:|-----|
| 0x4031c0 | sub_4031C0 | 815 | glyph_wrap.h, ui_text.h |
| 0x406440 | cs_fail_406440 | 184 | cs_dispatch.c, cs_dispatch.h, cs_dispatch_win32.c |
| 0x408b90 | sub_408B90 | 419 | wnd_proc.h |
| 0x40a540 | sub_40A540 | 133 | char_integrate.h, character.h |
| 0x40dee0 | sub_40DEE0 | 85 | main.c, ui_core.h |
| 0x40e0c0 | menu_ctrl_clear_40E0C0 | 555 | menu_list.c, menu_list.h, room_load.h |
| 0x40e360 | sub_40E360 | 636 | main.c, ui_text.h |
| 0x40e5e0 | wrap_class_40E5E0 | 2553 | glyph_wrap.c, glyph_wrap.h, main.c (+1) |
| 0x40f040 | wrap_parse_40F040 | 379 | glyph_wrap.c, glyph_wrap.h, ui_text.h |
| 0x40f3e0 | sub_40F3E0 | 434 | menu_list.c, menu_list.h, newgame_menu.c (+1) |
| 0x40f5c0 | menu_ctrl_build_40F5C0 | 563 | menu_list.c, menu_list.h, ui_core.h |
| 0x40fa00 | sub_40FA00 | 800 | glyph_text.c, glyph_text.h, newgame_menu.c (+4) |
| 0x40fd20 | glyph_token_search_40FD20 | 143 | glyph_text.c, glyph_text.h |
| 0x411530 | sub_411530 | 38 | char_ai.h |
| 0x411940 | sub_411940 | 770 | main.c, newgame_picker.c, newgame_picker.h (+1) |
| 0x411f40 | sub_411F40 | 444 | menu_list.c, menu_list.h, newgame_menu.c (+2) |
| 0x412160 | sub_412160 | 459 | newgame_menu.c, newgame_menu.h, newgame_picker.c (+2) |
| 0x412c10 | sub_412C10 | 46 | obj_container.c, obj_container.h, ui_core.h |
| 0x414080 | sub_414080 | 63 | obj_container.c, obj_container.h, ui_core.h |
| 0x417870 | sub_417870 | 106 | map_decode.h, sprite_slot.h |
| 0x4178e0 | sub_4178E0 | 194 | asset_register.c, asset_register.h, bitmap_session.c (+1) |
| 0x4179b0 | sub_4179B0 | 415 | asset_register.c, asset_register.h, sprite_slot.h (+1) |
| 0x417b50 | sub_417B50 | 109 | asset_register.c, asset_register.h, sprite_slot.h |
| 0x417bc0 | ar_pal_swap_table_for_va_417BC0 | 123 | asset_register.c, asset_register.h, sprite_slot.h |
| 0x417c40 | game_arm_parallax_grade_417C40 | 1625 | actor_render.h, actor_spawn.c, color_grade.c (+3) |
| 0x418470 | sub_418470 | 40 | asset_register.c, asset_register.h, color_grade.h (+5) |
| 0x4184a0 | sub_4184A0 | 1035 | asset_register.c, asset_register.h, bitmap_session.h (+1) |
| 0x4188b0 | sub_4188B0 | 891 | asset_register.c, asset_register.h |
| 0x4192b0 | sub_4192B0 | 52 | menu_list.c, menu_list.h, newgame_picker.c (+1) |
| 0x419900 | picker_seek_value_419900 | 128 | newgame_picker.c, newgame_picker.h, title_menu.h |
| 0x426110 | sub_426110 | 610 | cs_dispatch.c, cs_dispatch.h, cs_dispatch_win32.c |
| 0x426f50 | sub_426F50 | 26 | char_integrate.h, character.c |
| 0x426fd0 | sub_426FD0 | 781 | base_stat_table.c, base_stat_table.h, char_def.h (+2) |
| 0x4385e0 | sub_4385E0 | 45 | char_physics.c, char_physics.h |
| 0x43b980 | sub_43B980 | 783 | cutscene.c, dialogue.h, ui_run.h |
| 0x43bca0 | sub_43BCA0 | 1105 | cutscene.c, dialogue.h, newgame_drive.c (+2) |
| 0x43c110 | sub_43C110 | 84 | input.c, input.h, newgame_scene.h (+1) |
| 0x43c2e0 | sub_43C2E0 | 795 | main.c, ui_run.h |
| 0x43ca40 | menu_list_nav_43CA40 | 970 | cutscene.c, dialogue.c, dialogue.h (+3) |
| 0x43ce50 | menu_list_latch_43CE50 | 220 | cutscene.c, dialogue.h, menu_list.c (+2) |
| 0x43d1d0 | camera_follow_step_43D1D0 | 366 | camera_follow.c, camera_follow.h, main.c |
| 0x43d340 | camera_shake_apply_43D340 | 299 | camera_follow.c, camera_follow.h |
| 0x43e250 | sub_43E250 | 334 | main.c, render_draw.h |
| 0x440f40 | sub_440F40 | 670 | char_ai.c, char_ai.h |
| 0x4411e0 | sub_4411E0 | 131 | char_ai.c, char_ai.h |
| 0x4416c0 | sub_4416C0 | 52 | entity_move.c, entity_move.h |
| 0x44d160 | sub_44D160 | 379 | actor_render.c, actor_render.h, actor_spawn.c (+4) |
| 0x44e640 | sub_44E640 | 49 | hud.c, hud.h, npc_ai.h |
| 0x44e680 | sub_44E680 | 59 | hud.c, hud.h, npc_ai.h |
| 0x466930 | sub_466930 | 1315 | char_physics.c, char_physics.h |
| 0x467bb0 | sub_467BB0 | 3364 | char_physics.c, char_physics.h |
| 0x46a880 | sub_46A880 | 8890 | input_dik.c, input_dik.h, input_live.c (+1) |
| 0x4766a0 | hud_door_on_screen_4766A0 | 106 | camera_follow.h, hud.c, hud.h |
| 0x478ba0 | char_ai_478BA0 | 3514 | char_ai.c, char_ai.h |
| 0x479960 | sub_479960 | 165 | character.c, character.h, input.c (+1) |
| 0x479a10 | sub_479A10 | 644 | input.h |
| 0x479ca0 | sub_479CA0 | 134 | input.h |
| 0x479d30 | sub_479D30 | 99 | char_ai.h |
| 0x479da0 | sub_479DA0 | 62 | char_ai.h |
| 0x479de0 | sub_479DE0 | 34 | char_ai.h |
| 0x479e70 | sub_479E70 | 275 | char_ai.h, character.c, character.h (+2) |
| 0x48c820 | sub_48C820 | 873 | newgame_cursor.h, render_draw.h |
| 0x48cf80 | widget_box_frame_48CF80 | 1095 | main.c, newgame_box.c, newgame_box.h (+1) |
| 0x48d3d0 | sub_48D3D0 | 664 | newgame_box.c, render_draw.h |
| 0x48d670 | sub_48D670 | 154 | newgame_box.c, render_draw.h |
| 0x48d940 | sub_48D940 | 300 | main.c, newgame_cursor.c, newgame_cursor.h (+1) |
| 0x48e200 | sub_48E200 | 1221 | glyph_render.c, glyph_render.h, main.c (+1) |
| 0x48e6d0 | glyph_ruby_draw_48E6D0 | 389 | glyph_render.c, glyph_render.h, render_draw.h |
| 0x48e860 | sub_48E860 | 181 | glyph_render.c, glyph_render.h, main.c (+1) |
| 0x48e920 | scene_fade_grid_render_48E920 | 403 | main.c, render_draw.h, scene_fade.c (+1) |
| 0x48eac0 | sub_48EAC0 | 1131 | main.c, map_present.c, map_present.h (+3) |
| 0x490b90 | sub_490B90 | 307 | camera_follow.h, dialogue.c, map_present.c (+2) |
| 0x490cd0 | sub_490CD0 | 603 | color_grade.h, main.c, parallax.c (+4) |
| 0x490f30 | sub_490F30 | 2002 | color_grade.h, draw_pool.h, main.c (+7) |
| 0x491770 | sub_491770 | 52 | asset_register.c, asset_register.h, sprite_slot.h |
| 0x4917b0 | draw_pool_emit_layered_4917B0 | 106 | draw_pool.c, draw_pool.h, map_render.c (+1) |
| 0x492670 | draw_pool_emit_actor_492670 | 118 | actor_render.h, actor_spawn.h, draw_pool.c (+3) |
| 0x494e60 | sub_494E60 | 3904 | base_stat_table.h, hud.c, hud.h (+3) |
| 0x4962a0 | sub_4962A0 | 242 | hud.c, hud.h, main.c (+1) |
| 0x4969b0 | sub_4969B0 | 1272 | hud.c, hud.h, main.c (+1) |
| 0x497b40 | party_stat_hp_display_497B40 | 98 | party.c, render_draw.h |
| 0x497bb0 | party_stat_mp_display_497BB0 | 98 | party.c, render_draw.h |
| 0x498620 | sub_498620 | 82 | hud.h, main.c, render_draw.h |
| 0x498680 | hud_bar_row_geom_498680 | 401 | hud.c, hud.h, main.c (+1) |
| 0x498f10 | sub_498F10 | 344 | hud.h, main.c, render_draw.h |
| 0x499560 | parallax_strip_499560 | 271 | color_grade.h, parallax.c, parallax.h (+1) |
| 0x499ab0 | scene_cinematic_step_499AB0 | 1356 | camera_follow.h, screen_fx.h |
| 0x49a740 | sf_pattern_edges_in_49A740 | 336 | scene_fade.c, scene_fade.h, screen_fx.h |
| 0x49a890 | sf_pattern_center_out_49A890 | 361 | scene_fade.c, scene_fade.h, screen_fx.h |
| 0x49aa00 | sf_pattern_sweep_down_49AA00 | 216 | scene_fade.c, scene_fade.h, screen_fx.h |
| 0x49aae0 | sf_pattern_sweep_up_49AAE0 | 234 | scene_fade.c, scene_fade.h, screen_fx.h |
| 0x49c640 | sub_49C640 | 720 | dialogue.c, scene_box.h |
| 0x4c5350 | map_resolve_room_key_4C5350 | 1169 | game_map.c, game_map.h, party_member.h |
| 0x4cc820 | sub_4CC820 | 508 | party.c, party.h, party_member.h |
| 0x4e59a0 | sub_4E59A0 | 1676 | party.h, party_member.h |
| 0x54c970 | map_grid_clear_cell_54C970 | 84 | map_grid.c, map_grid.h |
| 0x54db10 | mover_horizontal_54DB10 | 946 | collision.c, collision.h, entity_move.c (+1) |
| 0x54ded0 | sub_54DED0 | 1555 | collision.c, collision.h |
| 0x54e5c0 | mover_vertical_54E5C0 | 945 | entity_move.c, entity_move.h |
| 0x54e990 | sub_54E990 | 861 | collision.c, collision.h |
| 0x560900 | sub_560900 | 251 | cs_dispatch.c, cs_dispatch.h, cs_dispatch_win32.c |
| 0x560e60 | actor_slot_reset_560E60 | 42 | game_map.c, game_map.h, scene_loop.h |
| 0x561c90 | sub_561C90 | 94 | game_map.h, game_world.h, scene_loop.h |
| 0x562210 | _WinMain@16 | 688 | app_pump.h, rng.h |
| 0x562a10 | sub_562A10 | 92 | asset_register.c, asset_register.h, title_menu.h |
| 0x562ea0 | build_title_drive_562EA0 | 4062 | app_flow.c, app_flow.h, asset_register.c (+11) |
| 0x563ef0 | sub_563EF0 | 445 | asset_register.c, asset_register.h |
| 0x564160 | sub_564160 | 376 | newgame_drive.c, newgame_drive.h, title_menu.h |
| 0x564780 | sub_564780 | 4067 | newgame_drive.c, newgame_drive.h, newgame_menu.c (+5) |
| 0x5657f0 | newgame_picker_dispatch_5657F0 | 31 | newgame_drive.c, newgame_picker.c, newgame_picker.h (+1) |
| 0x565d10 | sub_565D10 | 1228 | main.c, newgame_drive.c, newgame_drive.h (+1) |
| 0x566570 | newgame_option_label_566570 | 485 | newgame_menu.c, newgame_menu.h, newgame_picker.h (+1) |
| 0x566850 | sub_566850 | 337 | newgame_menu.c, newgame_menu.h, newgame_scene.c (+2) |
| 0x566a80 | newgame_option_value_566A80 | 2781 | newgame_menu.c, newgame_menu.h, newgame_picker.c (+3) |
| 0x567ba0 | sub_567BA0 | 1647 | main.c, newgame_drive.c, newgame_drive.h (+3) |
| 0x568320 | sub_568320 | 964 | newgame_drive.c, newgame_picker.c, newgame_picker.h (+1) |
| 0x56aea0 | sub_56AEA0 | 4294 | app_flow.h, input.h, main.c (+12) |
| 0x56c030 | sub_56C030 | 62 | title_menu.h, title_particles.c |
| 0x56c070 | drive_spawn_sparkle_56C070 | 265 | main.c, rng.h, title_menu.h (+2) |
| 0x56c180 | title_compositor_resolve_56C180 | 297 | main.c, title_menu.h, title_particles.c (+4) |
| 0x56c2b0 | sub_56C2B0 | 373 | main.c, title_menu.h, title_particles.c (+1) |
| 0x56c470 | sub_56C470 | 107 | title_menu.h, title_render.c, title_render.h (+1) |
| 0x56c4e0 | sub_56C4E0 | 155 | title_menu.h, title_render.c, title_render.h |
| 0x56c580 | title_render_sparkle_56C580 | 133 | title_menu.h, title_render.c, title_render.h (+1) |
| 0x56c610 | title_draw_sprite_56C610 | 35 | title_menu.h, title_render.c, title_render.h |
| 0x56c930 | menu_owner_transition_step_56C930 | 607 | menu_list.c, menu_list.h, title_menu.h |
| 0x56cd20 | sub_56CD20 | 2275 | main.c, prologue_drive.c, prologue_drive.h (+3) |
| 0x56dc20 | sub_56DC20 | 344 | main.c, title_menu.h |
| 0x56e190 | sub_56E190 | 26411 | asset_register.c, asset_register.h |
| 0x5748c0 | sub_5748C0 | 237 | asset_register.c, asset_register.h |
| 0x5749b0 | sub_5749B0 | 2342 | asset_register.c, asset_register.h, main.c |
| 0x5752e0 | sub_5752E0 | 17310 | main.c |
| 0x579a00 | sub_579A00 | 451 | asset_register.c, asset_register.h |
| 0x579bd0 | sub_579BD0 | 741 | asset_register.c, asset_register.h |
| 0x579ec0 | sub_579EC0 | 126 | asset_register.c, asset_register.h |
| 0x579f40 | sub_579F40 | 214 | asset_register.c, asset_register.h |
| 0x57a030 | sub_57A030 | 340 | asset_register.c, asset_register.h |
| 0x57a1a0 | sub_57A1A0 | 189 | asset_register.c, asset_register.h |
| 0x57a260 | sub_57A260 | 195 | asset_register.c, asset_register.h |
| 0x57a330 | ar_run_palette_ramp_57A330 | 3919 | asset_register.c, asset_register.h |
| 0x57b280 | sub_57B280 | 6073 | asset_register.c, asset_register.h |
| 0x57ca40 | sub_57CA40 | 24884 | asset_register.c, asset_register.h |
| 0x582b80 | sub_582B80 | 384 | asset_register.c, asset_register.h |
| 0x582d00 | sub_582D00 | 15 | asset_register.c, asset_register.h |
| 0x582d10 | sub_582D10 | 379 | asset_register.c, asset_register.h |
| 0x582e90 | sub_582E90 | 3560 | cs_dispatch.c, cs_dispatch.h, main.c (+2) |
| 0x585000 | gw_cross_reference_585000 | 413 | game_world.c, game_world.h, scene_loop.h (+2) |
| 0x586010 | load_room_586010 | 6133 | asset_register.c, asset_register.h, game_world.h (+4) |
| 0x587970 | sub_587970 | 866 | main.c, map_data.c, map_data.h (+5) |
| 0x587e00 | sub_587E00 | 18059 | asset_register.h, collision.h, game_world.h (+7) |
| 0x58c910 | game_bank_dims_58C910 | 347 | main.c, map_decode.c, map_grid.c (+1) |
| 0x58ca80 | sub_58CA80 | 167 | map_decode.c, map_grid.c, map_grid.h |
| 0x58cb30 | layer_hdr_u32_58CB30 | 1065 | map_decode.c, map_decode.h |
| 0x58ffa0 | sub_58FFA0 | 23 | wnd_proc.h, wnd_proc_win32.c |
| 0x59f2c0 | sub_59F2C0 | 3522 | game_map.c, game_map.h, game_world.c (+5) |
| 0x5a00c0 | sub_5A00C0 | 13690 | map_data.h |
| 0x5a4770 | sub_5A4770 | 45963 | app_pump.h, main.c |
| 0x5b1030 | sub_5B1030 | 156 | app_pump.c, app_pump.h, title_scene.c (+2) |
| 0x5b12e0 | sub_5B12E0 | 441 | wnd_proc.c, wnd_proc.h |
| 0x5b14c0 | sub_5B14C0 | 287 | wnd_proc.h, wnd_proc_win32.c |
| 0x5b5ac0 | zdd_busy_wait_ms_5B5AC0 | 39 | zdd.h, zdd_win32.c |
| 0x5b5d90 | sub_5B5D90 | 33 | asset_register.c, asset_register.h |
| 0x5b5dc0 | sub_5B5DC0 | 214 | main.c |
| 0x5b5f50 | sub_5B5F50 | 71 | asset_register.c, asset_register.h |
| 0x5b62a0 | sub_5B62A0 | 94 | map_data.h |
| 0x5b6340 | sub_5B6340 | 198 | map_data.c, map_data.h |
| 0x5b6e70 | unknown_libname_2 | 9 | bitmap_session.h |
| 0x5b6e80 | sub_5B6E80 | 5 | bitmap_session.h |
| 0x5b6e90 | sub_5B6E90 | 24 | bitmap_session.h |
| 0x5b6f00 | sub_5B6F00 | 7 | bitmap_session.c, bitmap_session.h |
| 0x5b6f10 | sub_5B6F10 | 26 | bitmap_session.h |
| 0x5b6f80 | sub_5B6F80 | 623 | bitmap_session.c, bitmap_session.h |
| 0x5b71f0 | sub_5B71F0 | 117 | bitmap_session.c, bitmap_session.h, bitmap_session_win32.c |
| 0x5b7270 | sub_5B7270 | 158 | bitmap_session.h |
| 0x5b7310 | sub_5B7310 | 469 | bitmap_session.c, bitmap_session.h |
| 0x5b74f0 | sub_5B74F0 | 293 | bitmap_session.h |
| 0x5b7800 | sub_5B7800 | 359 | asset_register.h, bitmap_session.c, bitmap_session.h |
| 0x5b7b90 | sub_5B7B90 | 53 | bitmap_session.h |
| 0x5b7bd0 | sub_5B7BD0 | 53 | asset_register.c, bitmap_session.c, bitmap_session.h |
| 0x5b7c10 | sub_5B7C10 | 186 | bitmap_session.c, bitmap_session.h |
| 0x5b7ee0 | sub_5B7EE0 | 153 | main.c, zdd.h |
| 0x5b7f80 | zdd_ctor_5B7F80 | 94 | zdd.c, zdd.h |
| 0x5b7fe0 | sub_5B7FE0 | 90 | zdd.h |
| 0x5b8040 | sub_5B8040 | 139 | zdd.h |
| 0x5b80d0 | sub_5B80D0 | 826 | zdd.c, zdd.h |
| 0x5b8480 | zdd_create_primary_surface_5B8480 | 1088 | zdd.c, zdd.h, zdd_win32.c |
| 0x5b88c0 | zdd_directdraw_create_ex_5B88C0 | 57 | zdd.h, zdd_win32.c |
| 0x5b8900 | zdd_set_display_mode_5B8900 | 74 | cs_dispatch.c, zdd.h, zdd_win32.c |
| 0x5b8950 | zdd_get_display_mode_5B8950 | 126 | zdd.h, zdd_win32.c |
| 0x5b89d0 | zdd_set_coop_level_5B89D0 | 71 | main.c, zdd.h, zdd_win32.c |
| 0x5b8a20 | sub_5B8A20 | 181 | zdd.c, zdd.h, zdd_win32.c |
| 0x5b8ae0 | pd_format_get_masks_5B8AE0 | 30 | pixel_drawer.c, pixel_drawer.h |
| 0x5b8b00 | zdd_color_convert_5B8B00 | 59 | render_platform.h, zdd.c, zdd.h |
| 0x5b8b40 | sub_5B8B40 | 184 | cs_dispatch.c, render_platform.h, zdd.c (+1) |
| 0x5b8c00 | zdd_create_surface_5B8C00 | 372 | render_platform.h, zdd.c, zdd.h (+1) |
| 0x5b8da0 | sub_5B8DA0 | 33 | zdd.h |
| 0x5b8dd0 | sub_5B8DD0 | 33 | cs_dispatch.c, zdd.h |
| 0x5b8e00 | sub_5B8E00 | 157 | zdd.h, zdd_win32.c |
| 0x5b8ea0 | zdd_object_upscale_16bpp_5B8EA0 | 285 | zdd.c, zdd.h |
| 0x5b8fc0 | flip_5B8FC0 | 335 | call_trace.h, game_drive.h, main.c (+8) |
| 0x5b9130 | sub_5B9130 | 151 | main.c, wnd_proc.h, wnd_proc_win32.c (+3) |
| 0x5b91d0 | sub_5B91D0 | 109 | zdd.h |
| 0x5b9240 | sub_5B9240 | 60 | zdd.h |
| 0x5b9280 | sub_5B9280 | 203 | zdd.c, zdd.h |
| 0x5b9350 | sub_5B9350 | 50 | render_platform.h, zdd.c, zdd.h |
| 0x5b9390 | title_frame_free_5B9390 | 75 | asset_register.c, asset_register.h, cs_dispatch.c (+5) |
| 0x5b93e0 | zdd_object_local_free_5B93E0 | 42 | render_platform.h, zdd.h, zdd_win32.c |
| 0x5b9410 | zdd_object_clear_5B9410 | 49 | main.c, osr_format.h, zdd.c (+1) |
| 0x5b9490 | sub_5B9490 | 63 | render_platform.h, zdd.c, zdd.h (+1) |
| 0x5b94d0 | sub_5B94D0 | 15 | render_platform.h, zdd.c, zdd.h (+1) |
| 0x5b94e0 | sub_5B94E0 | 29 | render_platform.h, wnd_proc.h, zdd.c (+2) |
| 0x5b9500 | sub_5B9500 | 17 | render_platform.h, wnd_proc.h, zdd.c (+2) |
| 0x5b9520 | sub_5B9520 | 157 | render_platform.h, zdd.c, zdd.h (+1) |
| 0x5b95c0 | sub_5B95C0 | 110 | render_platform.h, zdd.c, zdd.h |
| 0x5b9630 | sub_5B9630 | 268 | zdd.c, zdd.h |
| 0x5b9740 | zdd_get_attached_surface_5B9740 | 153 | zdd.h, zdd_win32.c |
| 0x5b97e0 | sub_5B97E0 | 66 | render_platform.h, zdd.c, zdd.h |
| 0x5b9830 | zdd_surface_set_color_key_5B9830 | 138 | render_platform.h, zdd.c, zdd.h (+1) |
| 0x5b98c0 | sub_5B98C0 | 73 | render_platform.h, zdd.c, zdd.h |
| 0x5b9910 | sub_5B9910 | 291 | zdd.c, zdd.h |
| 0x5b9a40 | blt_onto_5B9A40 | 112 | main.c, parallax.h, render_platform.h (+3) |
| 0x5b9ab0 | sub_5B9AB0 | 10 | zdd.h |
| 0x5b9ac0 | sub_5B9AC0 | 22 | zdd.c, zdd.h |
| 0x5b9ae0 | blt_rects_5B9AE0 | 140 | render_platform.h, zdd.c, zdd.h |
| 0x5b9b70 | blt_keyed_5B9B70 | 122 | hud.h, main.c, map_present.c (+5) |
| 0x5b9bf0 | blt_clipped_5B9BF0 | 256 | main.c, map_present.c, map_present.h (+4) |
| 0x5ba290 | sub_5BA290 | 30 | wnd_proc.h, wnd_proc_win32.c |
| 0x5bad20 | sub_5BAD20 | 26 | wnd_proc.h |
| 0x5bad40 | sub_5BAD40 | 26 | wnd_proc.h |
| 0x5bae30 | sub_5BAE30 | 60 | wnd_proc.h |
| 0x5bae70 | sub_5BAE70 | 30 | wnd_proc.h |
| 0x5bbd20 | sub_5BBD20 | 140 | wnd_proc.h, wnd_proc_win32.c |
| 0x5bcff0 | sub_5BCFF0 | 42 | pixel_drawer.c, pixel_drawer.h |
| 0x5bd020 | pd_channel_init_5BD020 | 26 | pixel_drawer.c, pixel_drawer.h |
| 0x5bd040 | build_lut_small_5BD040 | 801 | pixel_drawer.c, pixel_drawer.h |
| 0x5bd380 | pd_channel_mask_to_shift_5BD380 | 39 | pixel_drawer.c, pixel_drawer.h |
| 0x5bd3b0 | pd_blend_set_color_5BD3B0 | 30 | pixel_drawer.c, pixel_drawer.h |
| 0x5bd3d0 | pd_blend_commit_5BD3D0 | 241 | pixel_drawer.c, pixel_drawer.h |
| 0x5bd4d0 | pd_blend_init_5BD4D0 | 71 | pixel_drawer.c, pixel_drawer.h |
| 0x5bd550 | blt_alpha_5BD550 | 302 | hud.h, main.c, map_present.c (+8) |
| 0x5bd680 | sub_5BD680 | 1072 | render_platform.h, zdd.c, zdd.h |
| 0x5bef0e | ??3@YAXPAX@Z | 11 | asset_register.c, asset_register.h, cs_dispatch.c (+4) |
| 0x5bf4e8 | _fopen | 19 | cs_dispatch_win32.c, wnd_proc.h |
| 0x5bf505 | rand_draw_5BF505 | 30 | cutscene.c, main.c, rng.c (+1) |
| 0x5bf5db | _exit | 17 | app_pump.c, app_pump.h, app_pump_win32.c (+5) |
| 0x5bf5fd | _doexit | 153 | wnd_proc.h |
| 0x5c0907 | hex32_lower_5C0907 | 45 | zdd.c, zdd.h |
| 0x5c0934 | _xtoa | 92 | zdd.c |
| 0x5c123f | _free | 47 | asset_register.c |

## ported (825) — reimplemented, no host test yet

| VA | name | size | src |
|----|------|-----:|-----|
| 0x4017a0 | sub_4017A0 | 40 | scene_loop.h |
| 0x4017d0 | sub_4017D0 | 1175 | world_alloc.c, world_alloc.h |
| 0x401c70 | sub_401C70 | 54 | world_alloc.h |
| 0x401cb0 | sub_401CB0 | 140 | world_alloc.h |
| 0x401d40 | sub_401D40 | 49 | scene_cmd.h |
| 0x401d80 | sub_401D80 | 100 | script_helpers.h |
| 0x401e60 | sub_401E60 | 232 | scene_cmd.h |
| 0x401f50 | sub_401F50 | 222 | script_engine.h |
| 0x402030 | sub_402030 | 44 | script_helpers.h |
| 0x402060 | sub_402060 | 62 | script_engine.h |
| 0x4020a0 | sub_4020A0 | 503 | script_helpers.h |
| 0x4022a0 | sub_4022A0 | 47 | npc_ai.h |
| 0x4022d0 | sub_4022D0 | 88 | room_load.h |
| 0x402330 | sub_402330 | 431 | scene_cmd.h |
| 0x4024e0 | sub_4024E0 | 290 | script_helpers.h |
| 0x402610 | sub_402610 | 277 | script_helpers.h |
| 0x402730 | sub_402730 | 344 | scene_cmd.h |
| 0x402a60 | sub_402A60 | 435 | char_state.h |
| 0x402d40 | sub_402D40 | 470 | scene_cmd.h |
| 0x402f20 | sub_402F20 | 21 | script_engine.h |
| 0x402f40 | sub_402F40 | 21 | script_helpers.h |
| 0x402f60 | sub_402F60 | 412 | script_helpers.h |
| 0x403100 | sub_403100 | 181 | scene_cmd.h |
| 0x405e80 | sub_405E80 | 577 | scene_cmd.h |
| 0x4060d0 | sub_4060D0 | 241 | char_state.h |
| 0x4061d0 | sub_4061D0 | 55 | scene_cmd.h |
| 0x406210 | sub_406210 | 555 | scene_cmd.h |
| 0x406500 | sub_406500 | 454 | script_helpers.h |
| 0x4066d0 | sub_4066D0 | 989 | party_member.h |
| 0x406ab0 | sub_406AB0 | 442 | script_helpers.h |
| 0x406c70 | sub_406C70 | 587 | script_helpers.h |
| 0x406ec0 | sub_406EC0 | 813 | party_member.h |
| 0x4071f0 | sub_4071F0 | 196 | party_member.h |
| 0x4072c0 | sub_4072C0 | 85 | char_def.h |
| 0x407320 | sub_407320 | 190 | char_integrate.h, party_member.h |
| 0x407500 | sub_407500 | 886 | script_engine.h |
| 0x407880 | sub_407880 | 23 | ui_menu.h |
| 0x4078a0 | sub_4078A0 | 81 | party_member.h |
| 0x407900 | sub_407900 | 99 | ui_menu.h |
| 0x407970 | sub_407970 | 348 | script_engine.h |
| 0x407b80 | sub_407B80 | 32 | char_integrate.h |
| 0x407ba0 | sub_407BA0 | 907 | sound_fx.h |
| 0x408100 | sub_408100 | 205 | gimmick.h |
| 0x4081d0 | sub_4081D0 | 1366 | script_engine.h |
| 0x408d40 | sub_408D40 | 124 | party_member.h |
| 0x408dc0 | sub_408DC0 | 714 | party_member.h |
| 0x409090 | sub_409090 | 67 | party_member.h |
| 0x4090e0 | sub_4090E0 | 107 | char_def.h |
| 0x4094d0 | sub_4094D0 | 71 | party_member.h |
| 0x409970 | sub_409970 | 45 | sana_skills.h, sound_fx.h |
| 0x4099a0 | sub_4099A0 | 1251 | sound_fx.h |
| 0x409e90 | sub_409E90 | 375 | sound_fx.h |
| 0x40a010 | sub_40A010 | 645 | sound_fx.h |
| 0x40a810 | sub_40A810 | 691 | gimmick.h |
| 0x40ab80 | sub_40AB80 | 136 | actor_ops.h |
| 0x40ac10 | sub_40AC10 | 127 | gimmick.h |
| 0x40afe0 | sub_40AFE0 | 1640 | gimmick.c, gimmick.h |
| 0x40b710 | sub_40B710 | 480 | sound_fx.h |
| 0x40b8f0 | sub_40B8F0 | 626 | event.c, event.h |
| 0x40bb70 | sub_40BB70 | 30 | title_menu.h |
| 0x40bb90 | sub_40BB90 | 502 | sound_fx.h |
| 0x40bd90 | sub_40BD90 | 110 | entity_life.h |
| 0x40be00 | sub_40BE00 | 36 | gimmick.h |
| 0x40be30 | sub_40BE30 | 1032 | gimmick.c, gimmick.h |
| 0x40c300 | sub_40C300 | 120 | gimmick.h |
| 0x40c380 | sub_40C380 | 5077 | script_engine.c, script_engine.h |
| 0x40d960 | sub_40D960 | 69 | char_state.h |
| 0x40d9b0 | sub_40D9B0 | 437 | room_loop.h |
| 0x40dbc0 | sub_40DBC0 | 776 | ui_menu.h |
| 0x40df40 | sub_40DF40 | 374 | ui_core.h |
| 0x40e2f0 | sub_40E2F0 | 100 | ui_core.h |
| 0x40f1c0 | sub_40F1C0 | 530 | ui_core.h |
| 0x40f5a0 | sub_40F5A0 | 25 | ui_core.h |
| 0x40f800 | sub_40F800 | 511 | ui_core.h |
| 0x40fdb0 | sub_40FDB0 | 70 | ui_core.h |
| 0x410300 | sub_410300 | 196 | sprite_slot.h |
| 0x4103d0 | sub_4103D0 | 389 | ui_core.h |
| 0x410560 | sub_410560 | 164 | ui_text.h |
| 0x410610 | sub_410610 | 3103 | ui_text.h |
| 0x411230 | sub_411230 | 347 | ui_text.h |
| 0x411390 | sub_411390 | 413 | sound_fx.h |
| 0x411560 | sub_411560 | 117 | gimmick.h |
| 0x4115e0 | sub_4115E0 | 33 | room_loop.h |
| 0x411610 | sub_411610 | 657 | ui_menu.h |
| 0x4118b0 | sub_4118B0 | 134 | sprite_slot.h |
| 0x411c50 | sub_411C50 | 623 | ui_core.h |
| 0x411ec0 | sub_411EC0 | 118 | ui_core.h |
| 0x412100 | sub_412100 | 85 | ui_core.h |
| 0x412330 | sub_412330 | 98 | ui_core.h |
| 0x4123a0 | sub_4123A0 | 290 | ui_menu.h |
| 0x4124d0 | sub_4124D0 | 146 | ui_core.h |
| 0x412570 | sub_412570 | 19 | ui_core.h |
| 0x412590 | sub_412590 | 615 | ui_menu.h |
| 0x412800 | sub_412800 | 416 | ui_text.h |
| 0x4129a0 | sub_4129A0 | 16 | ui_menu.h |
| 0x4129b0 | sub_4129B0 | 602 | ui_menu.h |
| 0x412c40 | sub_412C40 | 227 | ui_core.h |
| 0x412d30 | sub_412D30 | 117 | ui_core.h |
| 0x412db0 | sub_412DB0 | 57 | room_loop.h |
| 0x412df0 | sub_412DF0 | 821 | ui_menu.h |
| 0x413130 | sub_413130 | 920 | ui_menu.h |
| 0x4134f0 | sub_4134F0 | 613 | ui_core.h |
| 0x413760 | sub_413760 | 173 | ui_core.h |
| 0x413810 | sub_413810 | 444 | party_member.h |
| 0x4139d0 | sub_4139D0 | 39 | ui_menu.h |
| 0x413a00 | sub_413A00 | 286 | party_member.h |
| 0x413b20 | sub_413B20 | 68 | party_member.h |
| 0x413b70 | sub_413B70 | 854 | ui_menu.h |
| 0x413ed0 | sub_413ED0 | 426 | ui_text.h |
| 0x4140c0 | sub_4140C0 | 836 | ui_menu.h |
| 0x414410 | sub_414410 | 119 | ui_menu.h |
| 0x414490 | sub_414490 | 1955 | ui_menu.h |
| 0x414c40 | sub_414C40 | 98 | ui_core.h |
| 0x414cb0 | sub_414CB0 | 906 | ui_menu.h |
| 0x415060 | sub_415060 | 751 | ui_menu.h |
| 0x415350 | sub_415350 | 195 | ui_menu.h |
| 0x415420 | sub_415420 | 58 | ui_core.h |
| 0x415460 | sub_415460 | 1018 | ui_menu.h |
| 0x415860 | sub_415860 | 198 | ui_core.h |
| 0x415930 | sub_415930 | 92 | ui_core.h |
| 0x415990 | sub_415990 | 119 | ui_core.h |
| 0x415a10 | sub_415A10 | 197 | ui_menu.h |
| 0x415e20 | sub_415E20 | 320 | party_member.h |
| 0x415f60 | sub_415F60 | 1205 | ui_menu.h |
| 0x416420 | sub_416420 | 115 | party_member.h |
| 0x416500 | sub_416500 | 107 | ui_core.h |
| 0x416570 | sub_416570 | 547 | party_member.h |
| 0x4167a0 | sub_4167A0 | 563 | ui_menu.h |
| 0x4169e0 | sub_4169E0 | 2529 | ui_menu.h |
| 0x4173d0 | sub_4173D0 | 29 | ui_core.h |
| 0x4173f0 | sub_4173F0 | 78 | ui_core.h |
| 0x417440 | sub_417440 | 51 | ui_core.h |
| 0x417480 | sub_417480 | 177 | ui_menu.h |
| 0x4175a0 | sub_4175A0 | 710 | ui_menu.h |
| 0x4182d0 | sub_4182D0 | 408 | room_load.h |
| 0x418c90 | sub_418C90 | 547 | ui_menu.h |
| 0x419c00 | sub_419C00 | 110 | scene_loop.h |
| 0x41a140 | sub_41A140 | 28 | room_loop.h |
| 0x41a2a0 | sub_41A2A0 | 254 | scene_cmd.h |
| 0x41a3a0 | sub_41A3A0 | 136 | script_helpers.h |
| 0x41a7f0 | sub_41A7F0 | 152 | scene_loop.h |
| 0x41bb80 | sub_41BB80 | 95 | title_menu.h |
| 0x41bbe0 | sub_41BBE0 | 312 | title_menu.h |
| 0x41c150 | sub_41C150 | 219 | title_menu.h |
| 0x41c240 | sub_41C240 | 58 | title_menu.h |
| 0x41c280 | sub_41C280 | 112 | ui_core.h |
| 0x41c2f0 | sub_41C2F0 | 42 | title_menu.h |
| 0x41c320 | sub_41C320 | 105 | title_menu.h |
| 0x41d190 | sub_41D190 | 779 | scene_cmd.h |
| 0x41d780 | sub_41D780 | 942 | title_menu.h |
| 0x41dc40 | sub_41DC40 | 75 | party_member.h |
| 0x41dc90 | sub_41DC90 | 890 | room_load.h |
| 0x41e020 | sub_41E020 | 70 | room_load.h |
| 0x41e180 | sub_41E180 | 244 | char_state.h |
| 0x41e280 | sub_41E280 | 47 | char_def.h |
| 0x41e2b0 | sub_41E2B0 | 63 | script_helpers.h |
| 0x41e2f0 | sub_41E2F0 | 66 | script_engine.h |
| 0x41e340 | sub_41E340 | 21 | script_helpers.h |
| 0x41e360 | sub_41E360 | 219 | entity_life.h |
| 0x41e440 | sub_41E440 | 125 | char_def.h |
| 0x41e600 | sub_41E600 | 907 | actor_ops.h |
| 0x41ea70 | sub_41EA70 | 187 | entity_life.h |
| 0x41eb30 | sub_41EB30 | 232 | script_helpers.h |
| 0x41ec20 | sub_41EC20 | 181 | scene_cmd.h |
| 0x41ece0 | sub_41ECE0 | 369 | script_helpers.h |
| 0x41ee60 | sub_41EE60 | 252 | char_state.h |
| 0x41ef80 | sub_41EF80 | 83 | char_state.h |
| 0x41efe0 | sub_41EFE0 | 27 | script_helpers.h |
| 0x41f000 | sub_41F000 | 72 | script_helpers.h |
| 0x41f050 | sub_41F050 | 100 | script_helpers.h |
| 0x41f0e0 | sub_41F0E0 | 280 | scene_cmd.h |
| 0x41f200 | sub_41F200 | 27403 | char_init.c, char_init.h |
| 0x426380 | sub_426380 | 157 | char_def.h |
| 0x426420 | sub_426420 | 24 | char_def.h |
| 0x426440 | sub_426440 | 47 | char_state.h |
| 0x426480 | sub_426480 | 4 | char_def.h |
| 0x426490 | sub_426490 | 322 | device_init.h |
| 0x4265e0 | sub_4265E0 | 53 | particle_init.h |
| 0x426620 | sub_426620 | 1437 | particle_init.c, particle_init.h |
| 0x426be0 | sub_426BE0 | 395 | char_integrate.h |
| 0x426d70 | sub_426D70 | 57 | particle_init.h |
| 0x426db0 | sub_426DB0 | 72 | char_state.h |
| 0x426e00 | sub_426E00 | 188 | char_def.h |
| 0x426ec0 | sub_426EC0 | 82 | particle_init.h |
| 0x426f20 | sub_426F20 | 47 | char_integrate.h |
| 0x426f70 | sub_426F70 | 37 | npc_ai.h |
| 0x426fa0 | sub_426FA0 | 45 | npc_ai.h |
| 0x4272e0 | sub_4272E0 | 79 | particle_init.h |
| 0x427330 | sub_427330 | 43 | device_init.h |
| 0x427360 | sub_427360 | 83 | char_def.h |
| 0x4273c0 | sub_4273C0 | 39 | char_def.h |
| 0x4273f0 | sub_4273F0 | 50 | char_def.h |
| 0x427430 | sub_427430 | 321 | char_def.h |
| 0x427580 | sub_427580 | 109 | char_def.h |
| 0x4275f0 | sub_4275F0 | 23 | char_def.h |
| 0x427610 | sub_427610 | 63 | char_def.h |
| 0x427650 | sub_427650 | 25 | char_def.h |
| 0x427670 | sub_427670 | 1242 | char_def.h |
| 0x427b70 | sub_427B70 | 132 | char_def.h |
| 0x427c00 | sub_427C00 | 43 | char_def.h |
| 0x427c30 | sub_427C30 | 83 | char_def.h |
| 0x427c90 | sub_427C90 | 43 | char_def.h |
| 0x427cc0 | sub_427CC0 | 23 | char_def.h |
| 0x427ce0 | sub_427CE0 | 73 | char_def.h |
| 0x427d30 | sub_427D30 | 141 | char_def.h |
| 0x427dc0 | sub_427DC0 | 33 | char_def.h |
| 0x427df0 | sub_427DF0 | 79 | char_def.h |
| 0x427e40 | sub_427E40 | 41 | char_def.h |
| 0x427e70 | sub_427E70 | 63 | char_def.h |
| 0x427eb0 | sub_427EB0 | 73 | char_def.h |
| 0x427f00 | sub_427F00 | 162 | char_def.h |
| 0x427fb0 | sub_427FB0 | 824 | char_def.h |
| 0x4282f0 | sub_4282F0 | 754 | char_def.h |
| 0x4285f0 | sub_4285F0 | 392 | char_def.h |
| 0x428780 | sub_428780 | 33 | char_def.h |
| 0x4287b0 | sub_4287B0 | 27 | char_def.h |
| 0x4287d0 | sub_4287D0 | 384 | char_def.h |
| 0x428950 | sub_428950 | 146 | char_def.h |
| 0x4289f0 | sub_4289F0 | 522 | char_def.h |
| 0x429d00 | sub_429D00 | 68 | party_member.h |
| 0x42c970 | sub_42C970 | 877 | char_monster.h |
| 0x42da20 | sub_42DA20 | 39 | char_monster.h |
| 0x42e290 | sub_42E290 | 1970 | char_monster.h |
| 0x42ea50 | sub_42EA50 | 195 | char_monster.h |
| 0x42eb20 | sub_42EB20 | 3097 | char_def.h |
| 0x42fbf0 | sub_42FBF0 | 772 | char_monster.h |
| 0x431930 | sub_431930 | 599 | char_def.h |
| 0x431b90 | sub_431B90 | 15 | char_def.h |
| 0x431ba0 | sub_431BA0 | 266 | char_def.h |
| 0x431cb0 | sub_431CB0 | 87 | char_def.h |
| 0x431d10 | sub_431D10 | 287 | scene_cmd.h |
| 0x431e30 | sub_431E30 | 25293 | device_init.c, device_init.h |
| 0x438520 | sub_438520 | 153 | device_init.h |
| 0x4385c0 | sub_4385C0 | 19 | device_init.h, particle_init.h |
| 0x438610 | sub_438610 | 19 | device_init.h |
| 0x438630 | sub_438630 | 60 | device_init.h |
| 0x438670 | sub_438670 | 42 | device_init.h |
| 0x4386a0 | sub_4386A0 | 80 | device_init.h |
| 0x4386f0 | sub_4386F0 | 64 | device_init.h |
| 0x438740 | sub_438740 | 555 | device_init.h |
| 0x438a60 | sub_438A60 | 2639 | particle_init.h |
| 0x439510 | sub_439510 | 67 | script_helpers.h |
| 0x439560 | sub_439560 | 283 | script_helpers.h |
| 0x439680 | sub_439680 | 9 | script_engine.h |
| 0x439690 | sub_439690 | 8866 | scene_run.h |
| 0x43c170 | sub_43C170 | 53 | ui_run.h |
| 0x43c610 | sub_43C610 | 52 | ui_run.h |
| 0x43c650 | sub_43C650 | 696 | ui_run.h |
| 0x43c920 | sub_43C920 | 143 | ui_run.h |
| 0x43c9b0 | sub_43C9B0 | 130 | room_load.h |
| 0x43cf30 | sub_43CF30 | 82 | ui_text.h |
| 0x43cf90 | sub_43CF90 | 564 | ui_text.h |
| 0x43d470 | sub_43D470 | 64 | ui_core.h |
| 0x43e080 | sub_43E080 | 89 | ui_core.h |
| 0x43e0e0 | sub_43E0E0 | 87 | ui_core.h |
| 0x43e140 | sub_43E140 | 266 | room_loop.h |
| 0x43e3a0 | sub_43E3A0 | 516 | event.c, event.h |
| 0x43e5b0 | sub_43E5B0 | 2120 | scene_cast.h |
| 0x43ee30 | sub_43EE30 | 228 | npc_ai.h |
| 0x43ef20 | sub_43EF20 | 1091 | scene_cast.h |
| 0x43f370 | sub_43F370 | 1284 | scene_cast.h |
| 0x43f880 | sub_43F880 | 5491 | npc_ai.c, npc_ai.h |
| 0x440e40 | sub_440E40 | 239 | npc_ai.h |
| 0x441270 | sub_441270 | 84 | npc_ai.h |
| 0x4412d0 | sub_4412D0 | 993 | npc_ai.c, npc_ai.h |
| 0x441700 | sub_441700 | 466 | npc_ai.h |
| 0x4418e0 | sub_4418E0 | 511 | npc_ai.h |
| 0x441ae0 | sub_441AE0 | 749 | npc_ai.h |
| 0x441f00 | sub_441F00 | 80 | scene_cast.h |
| 0x441f50 | sub_441F50 | 1284 | behave_helpers.h |
| 0x442460 | sub_442460 | 38 | actor_update.c, actor_update.h |
| 0x442490 | sub_442490 | 329 | behave_helpers.h |
| 0x442600 | sub_442600 | 53 | scene_cast.h |
| 0x442640 | sub_442640 | 200 | behave_helpers.h |
| 0x442710 | sub_442710 | 374 | behave_helpers.h |
| 0x442890 | sub_442890 | 54 | char_integrate.h |
| 0x442920 | sub_442920 | 149 | char_integrate.h |
| 0x4429c0 | sub_4429C0 | 86 | npc_ai.h |
| 0x442a20 | sub_442A20 | 76 | npc_ai.h |
| 0x442a70 | sub_442A70 | 12026 | char_handlers.c, char_integrate.c, char_integrate.h |
| 0x445ce0 | sub_445CE0 | 67 | shots.h |
| 0x445d30 | sub_445D30 | 124 | char_integrate.h |
| 0x445db0 | sub_445DB0 | 150 | actor_update.c, actor_update.h |
| 0x445f50 | sub_445F50 | 21 | char_integrate.h |
| 0x445f70 | sub_445F70 | 91 | char_integrate.h |
| 0x445fd0 | sub_445FD0 | 1015 | sound_fx.h |
| 0x4463d0 | sub_4463D0 | 48 | sound_fx.h |
| 0x446400 | sub_446400 | 225 | sound_fx.h |
| 0x4464f0 | sub_4464F0 | 138 | sound_fx.h |
| 0x446580 | sub_446580 | 185 | sound_fx.h |
| 0x446640 | sub_446640 | 112 | char_integrate.h |
| 0x4466b0 | sub_4466B0 | 276 | char_handlers.h |
| 0x4467d0 | sub_4467D0 | 289 | entity_life.h |
| 0x4478e0 | sub_4478E0 | 18 | char_attack.h |
| 0x447900 | sub_447900 | 772 | char_attack.h |
| 0x447c10 | sub_447C10 | 697 | char_attack.h |
| 0x447ed0 | sub_447ED0 | 89 | char_attack.h |
| 0x447f30 | sub_447F30 | 692 | char_attack.h |
| 0x4481f0 | sub_4481F0 | 424 | char_attack.h |
| 0x4483a0 | sub_4483A0 | 31 | char_attack.h |
| 0x4483c0 | sub_4483C0 | 227 | char_attack.h |
| 0x448c80 | sub_448C80 | 47 | particle_init.h |
| 0x448cb0 | sub_448CB0 | 264 | entity_fx.c, entity_fx.h |
| 0x449700 | sub_449700 | 165 | shots.h |
| 0x4497b0 | sub_4497B0 | 82 | shots.h |
| 0x44bbc0 | sub_44BBC0 | 66 | entity_fx.c, entity_fx.h |
| 0x44ce20 | sub_44CE20 | 505 | char_attack.h |
| 0x44d020 | sub_44D020 | 308 | char_status.h |
| 0x44d2e0 | sub_44D2E0 | 1528 | char_handlers.h |
| 0x44e6c0 | sub_44E6C0 | 108 | char_status.h |
| 0x4504c0 | sub_4504C0 | 197 | shots.h |
| 0x450590 | sub_450590 | 46 | char_attack.h |
| 0x4505c0 | sub_4505C0 | 577 | char_attack.h |
| 0x450e00 | sub_450E00 | 237 | entity_fx.h |
| 0x450ef0 | sub_450EF0 | 776 | sound_fx.h |
| 0x451200 | sub_451200 | 79 | entity_fx.c, entity_fx.h |
| 0x451250 | sub_451250 | 119 | char_integrate.h |
| 0x4512d0 | sub_4512D0 | 1012 | char_handlers.h |
| 0x4537d0 | sub_4537D0 | 283 | entity_fx.c, entity_fx.h |
| 0x4538f0 | sub_4538F0 | 112 | shots.h |
| 0x453960 | velocity_scatter_453960 | 70 | char_state.h |
| 0x455d80 | sub_455D80 | 3175 | char_handlers.h |
| 0x456a50 | sub_456A50 | 272 | skill_cast.h |
| 0x456b60 | sub_456B60 | 248 | skill_cast.h |
| 0x456c60 | sub_456C60 | 184 | char_handlers.h |
| 0x457ff0 | sub_457FF0 | 4675 | arche_pose.c, arche_pose.h |
| 0x459370 | sub_459370 | 71 | arche_pose.h |
| 0x4593c0 | sub_4593C0 | 32 | char_state.h |
| 0x4593e0 | sub_4593E0 | 1399 | arche_pose.h |
| 0x459990 | sub_459990 | 1313 | arche_pose.h |
| 0x459ed0 | sub_459ED0 | 72 | arche_pose.h |
| 0x459f20 | sub_459F20 | 888 | skill_cast.h |
| 0x45a2c0 | sub_45A2C0 | 58 | skill_cast.h |
| 0x45a300 | sub_45A300 | 14079 | skill_cast.c, skill_cast.h |
| 0x45db20 | sub_45DB20 | 1293 | skill_cast.h |
| 0x45e050 | sub_45E050 | 517 | skill_cast.h |
| 0x45e4c0 | sub_45E4C0 | 725 | skill_cast.h |
| 0x45e830 | sub_45E830 | 9388 | sana_skills.c, sana_skills.h |
| 0x460de0 | sub_460DE0 | 87 | char_attack.h |
| 0x460e40 | sub_460E40 | 143 | particle_init.h |
| 0x460ed0 | sub_460ED0 | 525 | char_attack.h |
| 0x4610e0 | sub_4610E0 | 209 | char_attack.h |
| 0x4611c0 | sub_4611C0 | 122 | char_attack.h |
| 0x461240 | sub_461240 | 434 | char_attack.h |
| 0x461410 | sub_461410 | 558 | char_attack.h |
| 0x461790 | sub_461790 | 19423 | arche_sword.c, arche_sword.h |
| 0x4665d0 | sub_4665D0 | 293 | char_attack.h |
| 0x466700 | sub_466700 | 73 | char_attack.h |
| 0x466750 | sub_466750 | 190 | arche_sword.h |
| 0x466e80 | sub_466E80 | 803 | char_attack.h |
| 0x4671c0 | sub_4671C0 | 158 | char_integrate.h |
| 0x467260 | sub_467260 | 208 | char_handlers.h |
| 0x467380 | ambient_event_timer_467380 | 376 | sound_fx.h |
| 0x467500 | sub_467500 | 185 | char_attack.h |
| 0x4678f0 | sub_4678F0 | 482 | arche_pose.h |
| 0x467ae0 | sub_467AE0 | 199 | arche_pose.h |
| 0x468a20 | sub_468A20 | 237 | room_loop.h |
| 0x46cd70 | actor_update_all_46CD70 | 1031 | world_tick.c, world_tick.h |
| 0x46d180 | sub_46D180 | 798 | entity_fx.c, entity_fx.h |
| 0x46d4a0 | sub_46D4A0 | 838 | entity_fx.c, entity_fx.h |
| 0x46d7f0 | sub_46D7F0 | 521 | entity_fx.c, entity_fx.h |
| 0x46da00 | sub_46DA00 | 1742 | shots.c, shots.h |
| 0x46e110 | sub_46E110 | 62 | shots.h |
| 0x46e150 | sub_46E150 | 49 | shots.h |
| 0x46e190 | sub_46E190 | 877 | shots.h |
| 0x46e510 | sub_46E510 | 10712 | particle_tick.c, particle_tick.h |
| 0x471080 | sub_471080 | 49 | particle_tick.c, particle_tick.h, shots.h |
| 0x4710c0 | sub_4710C0 | 20681 | shots.c, shots.h |
| 0x476500 | sub_476500 | 66 | shots.h |
| 0x476550 | sub_476550 | 326 | shots.h |
| 0x476710 | sub_476710 | 44 | particle_init.h |
| 0x476740 | sub_476740 | 307 | shots.h |
| 0x476880 | sub_476880 | 509 | sound_fx.h |
| 0x476a80 | sub_476A80 | 48 | shots.h |
| 0x476ab0 | sub_476AB0 | 229 | shots.h |
| 0x476ba0 | sub_476BA0 | 198 | shots.h |
| 0x476c70 | sub_476C70 | 144 | particle_init.h |
| 0x476d00 | sub_476D00 | 45 | particle_init.h |
| 0x476d30 | sub_476D30 | 230 | shots.h |
| 0x476e20 | sub_476E20 | 193 | shots.h |
| 0x476ef0 | sub_476EF0 | 369 | shots.h |
| 0x477070 | sub_477070 | 338 | shots.h |
| 0x4771d0 | sub_4771D0 | 316 | shots.h |
| 0x477310 | sub_477310 | 589 | shots.h |
| 0x477560 | sub_477560 | 232 | shots.h |
| 0x477650 | sub_477650 | 1294 | shots.h |
| 0x477b70 | sub_477B70 | 898 | shots.h |
| 0x477f10 | sub_477F10 | 727 | shots.h |
| 0x4781f0 | sub_4781F0 | 859 | shots.h |
| 0x478550 | sub_478550 | 77 | shots.h |
| 0x4785a0 | sub_4785A0 | 633 | shots.h |
| 0x478820 | sub_478820 | 551 | shots.h |
| 0x478a50 | sub_478A50 | 329 | char_attack.h |
| 0x479e10 | sub_479E10 | 38 | behave_helpers.h |
| 0x479e40 | sub_479E40 | 41 | behave_helpers.h |
| 0x47a970 | sub_47A970 | 1059 | behave_helpers.h |
| 0x47adb0 | sub_47ADB0 | 79 | behave_helpers.h |
| 0x47ae00 | sub_47AE00 | 2364 | behave_helpers.h |
| 0x47b770 | sub_47B770 | 77 | behave_ops.h |
| 0x47b7c0 | sub_47B7C0 | 163 | gimmick.h |
| 0x47b870 | sub_47B870 | 250 | behave_helpers.h |
| 0x47b990 | actor_ai_47B990 | 7461 | behave_helpers.c, behave_ops.h, char_behave.c (+1) |
| 0x47d740 | sub_47D740 | 26 | actor_update.c, actor_update.h |
| 0x47d760 | sub_47D760 | 47 | behave_ops.h |
| 0x47d790 | sub_47D790 | 710 | behave_helpers.h |
| 0x47da60 | sub_47DA60 | 329 | behave_helpers.h |
| 0x47dbb0 | sub_47DBB0 | 655 | behave_ops.h |
| 0x47de50 | sub_47DE50 | 446 | behave_helpers.h |
| 0x47e010 | sub_47E010 | 504 | behave_helpers.h |
| 0x47e330 | sub_47E330 | 1120 | behave_ops.h |
| 0x47e790 | sub_47E790 | 37 | npc_ai.h |
| 0x47e7c0 | sub_47E7C0 | 799 | behave_helpers.h |
| 0x47eae0 | sub_47EAE0 | 469 | behave_helpers.h |
| 0x47ecd0 | sub_47ECD0 | 2303 | behave_helpers.h |
| 0x47f610 | sub_47F610 | 90 | behave_helpers.h |
| 0x47f670 | sub_47F670 | 861 | behave_helpers.h |
| 0x47f9d0 | sub_47F9D0 | 874 | behave_helpers.h |
| 0x47fd40 | sub_47FD40 | 1242 | behave_helpers.h |
| 0x480260 | sub_480260 | 577 | behave_helpers.h |
| 0x4804b0 | sub_4804B0 | 207 | gimmick.h |
| 0x480580 | sub_480580 | 1430 | behave_helpers.h |
| 0x480b30 | sub_480B30 | 1932 | behave_helpers.h |
| 0x4812c0 | sub_4812C0 | 79 | gimmick.h |
| 0x481310 | sub_481310 | 1144 | behave_helpers.h |
| 0x481790 | sub_481790 | 645 | behave_helpers.h |
| 0x481a20 | sub_481A20 | 148 | behave_helpers.h |
| 0x481ac0 | sub_481AC0 | 1874 | behave_helpers.h |
| 0x482240 | sub_482240 | 310 | behave_helpers.h |
| 0x482380 | sub_482380 | 217 | behave_helpers.h |
| 0x482460 | sub_482460 | 536 | behave_helpers.h |
| 0x4826b0 | sub_4826B0 | 50 | behave_helpers.h |
| 0x482700 | sub_482700 | 96 | behave_helpers.h |
| 0x482760 | sub_482760 | 281 | behave_helpers.h |
| 0x482880 | sub_482880 | 532 | behave_helpers.h |
| 0x482ac0 | sub_482AC0 | 844 | behave_helpers.h |
| 0x482e10 | sub_482E10 | 501 | behave_helpers.h |
| 0x483040 | sub_483040 | 668 | behave_helpers.h |
| 0x4832e0 | sub_4832E0 | 519 | behave_helpers.h |
| 0x4834f0 | sub_4834F0 | 1062 | behave_helpers.h |
| 0x483920 | sub_483920 | 555 | behave_helpers.h |
| 0x483b50 | sub_483B50 | 600 | behave_helpers.h |
| 0x483de0 | sub_483DE0 | 446 | behave_helpers.h |
| 0x483fb0 | sub_483FB0 | 784 | behave_helpers.h |
| 0x4842c0 | sub_4842C0 | 244 | npc_ai.h |
| 0x4843c0 | sub_4843C0 | 360 | behave_helpers.h |
| 0x484530 | sub_484530 | 740 | behave_helpers.h |
| 0x484830 | sub_484830 | 90 | behave_helpers.h |
| 0x4848a0 | sub_4848A0 | 788 | behave_helpers.h |
| 0x484bc0 | sub_484BC0 | 48 | behave_helpers.h |
| 0x484bf0 | sub_484BF0 | 31 | behave_ops.h |
| 0x484c10 | sub_484C10 | 322 | behave_ops.h |
| 0x484d60 | sub_484D60 | 319 | behave_helpers.h |
| 0x484ea0 | sub_484EA0 | 1029 | behave_helpers.h |
| 0x4852c0 | sub_4852C0 | 37 | behave_ops.h |
| 0x4852f0 | sub_4852F0 | 55 | behave_ops.h |
| 0x485330 | sub_485330 | 22 | behave_helpers.h |
| 0x485350 | sub_485350 | 391 | behave_helpers.h |
| 0x4854e0 | sub_4854E0 | 388 | behave_helpers.h |
| 0x485670 | sub_485670 | 90 | behave_ops.h |
| 0x4856d0 | sub_4856D0 | 83 | behave_ops.h |
| 0x485740 | sub_485740 | 166 | behave_ops.h |
| 0x4857f0 | sub_4857F0 | 404 | behave_ops.h |
| 0x485990 | sub_485990 | 793 | behave_helpers.h |
| 0x485cb0 | sub_485CB0 | 576 | behave_helpers.h |
| 0x485f30 | sub_485F30 | 55 | behave_ops.h |
| 0x485f70 | sub_485F70 | 68 | char_state.h |
| 0x485fc0 | sub_485FC0 | 4709 | char_apply.c, char_apply.h |
| 0x487250 | sub_487250 | 114 | char_state.h |
| 0x4872d0 | sub_4872D0 | 474 | char_state.h |
| 0x4874b0 | sub_4874B0 | 115 | char_state.h |
| 0x487530 | sub_487530 | 54 | char_state.h |
| 0x487570 | sub_487570 | 437 | char_state.h |
| 0x487730 | sub_487730 | 366 | char_state.h |
| 0x4878b0 | sub_4878B0 | 1289 | char_status.c, char_status.h |
| 0x487dc0 | sub_487DC0 | 5082 | char_status.c, char_status.h |
| 0x4891c0 | sub_4891C0 | 181 | char_status.h |
| 0x489280 | sub_489280 | 818 | sound_fx.h |
| 0x4895c0 | sub_4895C0 | 8605 | char_hit.c, char_hit.h |
| 0x48b850 | sub_48B850 | 131 | char_hit.h |
| 0x48b8e0 | sub_48B8E0 | 60 | char_hit.h |
| 0x48b920 | sub_48B920 | 526 | behave_helpers.h |
| 0x48bb30 | sub_48BB30 | 632 | char_status.h |
| 0x48bdd0 | sub_48BDD0 | 132 | behave_helpers.h |
| 0x48be60 | sub_48BE60 | 715 | behave_helpers.h |
| 0x48c150 | sub_48C150 | 1374 | render_draw.h |
| 0x48c6b0 | sub_48C6B0 | 359 | render_draw.h |
| 0x48cb90 | sub_48CB90 | 1005 | render_draw.h |
| 0x48d710 | sub_48D710 | 550 | render_draw.h |
| 0x48da70 | sub_48DA70 | 1672 | render_draw.h |
| 0x48e100 | sub_48E100 | 246 | render_draw.h |
| 0x48ef40 | sub_48EF40 | 4389 | render_draw.h |
| 0x490090 | sub_490090 | 59 | render_draw.h |
| 0x4900d0 | sub_4900D0 | 129 | render_draw.h |
| 0x490160 | sub_490160 | 641 | render_draw.h |
| 0x4903f0 | sub_4903F0 | 1110 | render_draw.h |
| 0x490860 | sub_490860 | 802 | render_draw.h |
| 0x491740 | sub_491740 | 38 | sprite_slot.h |
| 0x491820 | sub_491820 | 696 | render_draw.h |
| 0x491ae0 | actor_render_emit_491AE0 | 2599 | render_draw.h |
| 0x4925e0 | sub_4925E0 | 129 | render_draw.h |
| 0x4926f0 | sub_4926F0 | 206 | render_draw.h |
| 0x4927c0 | sub_4927C0 | 289 | render_draw.h |
| 0x4928f0 | sub_4928F0 | 412 | render_draw.h |
| 0x492a90 | sub_492A90 | 429 | render_draw.h |
| 0x492c40 | sub_492C40 | 740 | render_draw.h |
| 0x492f30 | sub_492F30 | 139 | render_draw.h |
| 0x492fc0 | band_23e0_render_492FC0 | 623 | render_draw.h |
| 0x493230 | band_2560_render_493230 | 584 | render_draw.h |
| 0x493480 | band_13e0_render_493480 | 824 | render_draw.h |
| 0x4937c0 | band_1060_render_4937C0 | 647 | render_draw.h |
| 0x493a50 | sub_493A50 | 328 | render_draw.h |
| 0x493ba0 | band_1160_char_render_493BA0 | 3014 | render_draw.h |
| 0x494790 | sub_494790 | 706 | render_draw.h |
| 0x494a60 | sub_494A60 | 918 | render_draw.h |
| 0x494e10 | sub_494E10 | 66 | render_draw.h |
| 0x495dc0 | sub_495DC0 | 119 | render_draw.h |
| 0x495e40 | sub_495E40 | 411 | render_draw.h |
| 0x495fe0 | sub_495FE0 | 394 | render_draw.h |
| 0x496170 | sub_496170 | 42 | render_draw.h |
| 0x4961a0 | sub_4961A0 | 63 | render_draw.h |
| 0x4961e0 | sub_4961E0 | 95 | render_draw.h |
| 0x496240 | sub_496240 | 95 | render_draw.h |
| 0x4963a0 | sub_4963A0 | 446 | render_draw.h |
| 0x496560 | sub_496560 | 1028 | render_draw.h |
| 0x496970 | sub_496970 | 54 | hud_view.h |
| 0x496ec0 | sub_496EC0 | 1756 | render_draw.h |
| 0x4975a0 | sub_4975A0 | 64 | render_draw.h |
| 0x4975e0 | sub_4975E0 | 1054 | render_draw.h |
| 0x497a00 | sub_497A00 | 313 | render_draw.h |
| 0x497c20 | sub_497C20 | 788 | render_draw.h |
| 0x497f40 | sub_497F40 | 1750 | render_draw.h |
| 0x498820 | sub_498820 | 319 | render_draw.h |
| 0x498960 | sub_498960 | 1449 | render_draw.h |
| 0x499070 | sub_499070 | 142 | render_draw.h |
| 0x499100 | sub_499100 | 1118 | render_draw.h |
| 0x499670 | sub_499670 | 316 | render_draw.h |
| 0x4997b0 | sub_4997B0 | 150 | render_draw.h |
| 0x49a050 | sub_49A050 | 13 | sprite_slot.h |
| 0x49a060 | sub_49A060 | 651 | ui_run.h |
| 0x49a2f0 | sub_49A2F0 | 76 | ui_run.h |
| 0x49a340 | sub_49A340 | 291 | ui_run.h |
| 0x49a470 | sub_49A470 | 622 | ui_run.h |
| 0x49a6e0 | sub_49A6E0 | 82 | title_menu.h |
| 0x49abd0 | sub_49ABD0 | 168 | screen_fx.h |
| 0x49ac80 | sub_49AC80 | 664 | screen_fx.h |
| 0x49af40 | hud_party_anim_update_49AF40 | 3313 | hud_view.h |
| 0x49bc40 | sub_49BC40 | 261 | hud_view.h |
| 0x49bd50 | sub_49BD50 | 147 | hud_view.h |
| 0x49bdf0 | sub_49BDF0 | 97 | hud_view.h |
| 0x49be60 | sub_49BE60 | 290 | room_loop.h |
| 0x49c170 | sub_49C170 | 371 | render_draw.h |
| 0x49c2f0 | sub_49C2F0 | 808 | screen_fx.h |
| 0x49c620 | sub_49C620 | 31 | ui_core.h |
| 0x49c910 | sub_49C910 | 588 | scene_box.h |
| 0x49cb60 | sub_49CB60 | 527 | script_engine.h |
| 0x49cd70 | sub_49CD70 | 254 | script_engine.h |
| 0x49ce70 | sub_49CE70 | 231 | room_tick.h |
| 0x49cf60 | sub_49CF60 | 17 | script_helpers.h |
| 0x49cf80 | sub_49CF80 | 722 | script_helpers.h |
| 0x49d6e0 | sub_49D6E0 | 590 | scene_cmd.h |
| 0x49d930 | sub_49D930 | 2296 | chapter_scripts.h |
| 0x49e250 | sub_49E250 | 1123 | script_helpers.h |
| 0x49e6c0 | sub_49E6C0 | 2001 | script_helpers.h |
| 0x49f7c0 | sub_49F7C0 | 5270 | chapter_scripts.h |
| 0x4a0c90 | sub_4A0C90 | 20785 | chapter_scripts.h |
| 0x4a5e20 | sub_4A5E20 | 32 | scene_cmd.h |
| 0x4a5e40 | sub_4A5E40 | 37 | script_helpers.h |
| 0x4a5e70 | sub_4A5E70 | 42 | script_helpers.h |
| 0x4a5ea0 | sub_4A5EA0 | 29 | script_helpers.h |
| 0x4a5ec0 | sub_4A5EC0 | 21 | script_helpers.h |
| 0x4a5ee0 | sub_4A5EE0 | 72 | scene_cmd.h |
| 0x4a5f30 | sub_4A5F30 | 20 | scene_cmd.h |
| 0x4a5f50 | sub_4A5F50 | 45 | scene_cmd.h |
| 0x4a5f80 | sub_4A5F80 | 10 | script_helpers.h |
| 0x4a5f90 | sub_4A5F90 | 11 | scene_cmd.h |
| 0x4a5fa0 | sub_4A5FA0 | 1057 | chapter_scripts.h |
| 0x4a63d0 | sub_4A63D0 | 706 | script_helpers.h |
| 0x4a66a0 | sub_4A66A0 | 1244 | chapter_scripts.h |
| 0x4a6b80 | sub_4A6B80 | 23051 | chapter_scripts.h |
| 0x4ac610 | sub_4AC610 | 79 | script_helpers.h |
| 0x4ac660 | sub_4AC660 | 98 | script_helpers.h |
| 0x4ac6d0 | sub_4AC6D0 | 6756 | chapter_scripts.h |
| 0x4ae150 | sub_4AE150 | 712 | chapter_scripts.h |
| 0x4ae420 | sub_4AE420 | 6478 | chapter_scripts.h |
| 0x4afda0 | sub_4AFDA0 | 3 | script_engine.h |
| 0x4afdb0 | sub_4AFDB0 | 15577 | chapter_scripts.h |
| 0x4b3b10 | sub_4B3B10 | 6 | script_helpers.h |
| 0x4b3b20 | sub_4B3B20 | 330 | chapter_scripts.h |
| 0x4b3c70 | sub_4B3C70 | 697 | chapter_scripts.h |
| 0x4b3f30 | sub_4B3F30 | 1599 | script_helpers.h |
| 0x4b4570 | sub_4B4570 | 1996 | script_helpers.h |
| 0x4b4d40 | sub_4B4D40 | 28488 | chapter_scripts.h |
| 0x4bbe00 | sub_4BBE00 | 55 | script_helpers.h |
| 0x4bbe40 | sub_4BBE40 | 62 | script_helpers.h |
| 0x4bbe80 | sub_4BBE80 | 105 | script_helpers.h |
| 0x4bbef0 | sub_4BBEF0 | 5004 | chapter_scripts.h |
| 0x4bd2e0 | sub_4BD2E0 | 968 | chapter_scripts.h |
| 0x4bd6b0 | sub_4BD6B0 | 359 | chapter_scripts.h |
| 0x4bd820 | sub_4BD820 | 960 | chapter_scripts.h |
| 0x4bdbe0 | sub_4BDBE0 | 937 | chapter_scripts.h |
| 0x4bdf90 | sub_4BDF90 | 580 | chapter_scripts.h |
| 0x4be220 | sub_4BE220 | 14286 | chapter_scripts.h |
| 0x4c1a30 | sub_4C1A30 | 491 | chapter_scripts.h |
| 0x4c1c20 | sub_4C1C20 | 1774 | chapter_scripts.h |
| 0x4c2310 | sub_4C2310 | 1701 | chapter_scripts.h |
| 0x4c29c0 | sub_4C29C0 | 1020 | chapter_scripts.h |
| 0x4c2dc0 | sub_4C2DC0 | 4360 | chapter_scripts.h |
| 0x4c3ee0 | sub_4C3EE0 | 1727 | chapter_scripts.h |
| 0x4c45a0 | sub_4C45A0 | 3423 | chapter_scripts.h |
| 0x4c5300 | sub_4C5300 | 71 | script_helpers.h |
| 0x4c57f0 | sub_4C57F0 | 50 | scene_loop.h |
| 0x4c5830 | sub_4C5830 | 694 | scene_loop.h |
| 0x4c5e00 | sub_4C5E00 | 778 | party_member.h |
| 0x4c6110 | sub_4C6110 | 65 | char_def.h |
| 0x4c6160 | sub_4C6160 | 574 | party_member.h |
| 0x4c63a0 | sub_4C63A0 | 391 | party_member.h |
| 0x4c6530 | sub_4C6530 | 30 | party_member.h |
| 0x4c6550 | sub_4C6550 | 618 | party_member.h |
| 0x4c6830 | sub_4C6830 | 337 | party_member.h |
| 0x4c6990 | sub_4C6990 | 419 | party_member.h |
| 0x4c6b40 | sub_4C6B40 | 21496 | chapter_scripts.h |
| 0x4cbf80 | sub_4CBF80 | 47 | scene_cmd.h |
| 0x4cbfb0 | sub_4CBFB0 | 32 | script_helpers.h |
| 0x4cbfd0 | sub_4CBFD0 | 32 | script_helpers.h |
| 0x4cbff0 | sub_4CBFF0 | 24 | script_helpers.h |
| 0x4cc010 | sub_4CC010 | 31 | script_helpers.h |
| 0x4cc030 | sub_4CC030 | 45 | script_helpers.h |
| 0x4cc060 | sub_4CC060 | 31 | scene_cmd.h |
| 0x4cc080 | sub_4CC080 | 28 | scene_cmd.h |
| 0x4cc0a0 | sub_4CC0A0 | 28 | scene_cmd.h |
| 0x4cc0c0 | sub_4CC0C0 | 28 | scene_cmd.h |
| 0x4cc250 | sub_4CC250 | 536 | script_helpers.h |
| 0x4cc470 | sub_4CC470 | 304 | script_helpers.h |
| 0x4cc5a0 | sub_4CC5A0 | 628 | scene_loop.h |
| 0x4cca20 | sub_4CCA20 | 40 | script_helpers.h |
| 0x4cca50 | sub_4CCA50 | 25 | script_helpers.h |
| 0x4cca70 | sub_4CCA70 | 453 | scene_cmd.h |
| 0x4ccc90 | sub_4CCC90 | 2610 | chapter_scripts.h |
| 0x4cd6d0 | sub_4CD6D0 | 14951 | chapter_scripts.h |
| 0x4d13a0 | sub_4D13A0 | 20018 | chapter_scripts.h |
| 0x4d6490 | sub_4D6490 | 32 | script_helpers.h |
| 0x4d64b0 | sub_4D64B0 | 62 | scene_cmd.h |
| 0x4d64f0 | sub_4D64F0 | 6202 | chapter_scripts.h |
| 0x4d7d80 | sub_4D7D80 | 18166 | chapter_scripts.h |
| 0x4dc510 | sub_4DC510 | 21461 | chapter_scripts.h |
| 0x4e1950 | sub_4E1950 | 135 | scene_loop.h |
| 0x4e19e0 | sub_4E19E0 | 811 | chapter_scripts.h |
| 0x4e6030 | sub_4E6030 | 54 | party_member.h |
| 0x4e6070 | sub_4E6070 | 299 | party_member.h |
| 0x4e61a0 | sub_4E61A0 | 33 | scene_loop.h |
| 0x525ab0 | sub_525AB0 | 45 | char_hit.h |
| 0x54bf90 | sub_54BF90 | 30 | room_load.h |
| 0x54bfb0 | sub_54BFB0 | 801 | world_alloc.c, world_alloc.h |
| 0x54c2e0 | sub_54C2E0 | 217 | world_alloc.h |
| 0x54c3c0 | sub_54C3C0 | 358 | world_alloc.h |
| 0x54c640 | sub_54C640 | 812 | grid_refresh.c, grid_refresh.h |
| 0x54c9d0 | sub_54C9D0 | 214 | grid_refresh.c, grid_refresh.h |
| 0x54cab0 | sub_54CAB0 | 484 | shots.h |
| 0x54cca0 | sub_54CCA0 | 792 | shots.h |
| 0x54cfc0 | sub_54CFC0 | 147 | shots.h |
| 0x54d060 | sub_54D060 | 36 | arche_pose.h |
| 0x54d090 | sub_54D090 | 2636 | entity_fx.c, entity_fx.h |
| 0x54ecf0 | sub_54ECF0 | 137 | gimmick.h |
| 0x54efd0 | sub_54EFD0 | 358 | gimmick.h |
| 0x54f980 | actor_update_54F980 | 11597 | actor_update.c, actor_update.h |
| 0x5531b0 | ambient_sound_timer_5531B0 | 756 | sound_fx.h |
| 0x555790 | sub_555790 | 1082 | actor_ops.h |
| 0x555bd0 | sub_555BD0 | 694 | actor_ops.h |
| 0x555e90 | sub_555E90 | 109 | actor_ops.h |
| 0x555f00 | sub_555F00 | 177 | entity_life.h |
| 0x555fc0 | sub_555FC0 | 433 | entity_life.h |
| 0x556180 | sub_556180 | 509 | entity_life.h |
| 0x556380 | sub_556380 | 74 | entity_life.h |
| 0x5563d0 | sub_5563D0 | 266 | entity_life.h |
| 0x5564e0 | sub_5564E0 | 298 | actor_ops.h |
| 0x556610 | sub_556610 | 391 | gimmick.h |
| 0x5567a0 | sub_5567A0 | 309 | gimmick.h |
| 0x5568e0 | sub_5568E0 | 161 | gimmick.h |
| 0x556990 | sub_556990 | 1076 | char_hit.h |
| 0x556de0 | sub_556DE0 | 98 | gimmick.h |
| 0x556eb0 | sub_556EB0 | 62 | actor_update.c, actor_update.h |
| 0x556ef0 | sub_556EF0 | 349 | device_init.h |
| 0x557060 | sub_557060 | 778 | room_tick.h |
| 0x557370 | particle_spawn_557370 | 452 | particle_spawn.c, particle_spawn.h |
| 0x557550 | sub_557550 | 21735 | particle_init.c, particle_init.h |
| 0x55cbc0 | sub_55CBC0 | 394 | particle_init.h |
| 0x55cd50 | sub_55CD50 | 85 | particle_init.h |
| 0x55cdb0 | sub_55CDB0 | 145 | particle_init.h |
| 0x55ce50 | sub_55CE50 | 18 | particle_init.h |
| 0x55ce70 | sub_55CE70 | 311 | skill_cast.h |
| 0x55cfc0 | sub_55CFC0 | 358 | char_status.h |
| 0x55d140 | sub_55D140 | 8040 | shots.c, shots.h |
| 0x55f150 | sub_55F150 | 59 | shots.h |
| 0x55f190 | sub_55F190 | 936 | ui_run.h |
| 0x55f550 | sub_55F550 | 199 | ui_run.h |
| 0x55f790 | sub_55F790 | 1558 | scene_loop.h |
| 0x560a00 | sub_560A00 | 57 | title_menu.h |
| 0x560a40 | sub_560A40 | 1049 | title_menu.h |
| 0x560e90 | sub_560E90 | 700 | scene_loop.h |
| 0x561150 | sub_561150 | 126 | scene_loop.h |
| 0x5611d0 | sub_5611D0 | 173 | scene_loop.h |
| 0x561280 | sub_561280 | 37 | scene_loop.h |
| 0x5612b0 | sub_5612B0 | 56 | scene_loop.h |
| 0x5614e0 | sub_5614E0 | 113 | scene_loop.h |
| 0x562a70 | sub_562A70 | 127 | scene_loop.h |
| 0x562af0 | sub_562AF0 | 82 | title_menu.h |
| 0x562ba0 | sub_562BA0 | 432 | scene_loop.h |
| 0x562d50 | sub_562D50 | 239 | room_load.h |
| 0x562e40 | sub_562E40 | 30 | scene_loop.h |
| 0x562e60 | sub_562E60 | 58 | scene_loop.h |
| 0x5642e0 | sub_5642E0 | 440 | title_menu.h |
| 0x5644a0 | sub_5644A0 | 483 | title_menu.h |
| 0x564690 | sub_564690 | 47 | title_menu.h |
| 0x565810 | sub_565810 | 45 | title_menu.h |
| 0x565840 | sub_565840 | 115 | title_menu.h |
| 0x5658c0 | sub_5658C0 | 565 | title_menu.h |
| 0x566250 | sub_566250 | 169 | title_menu.h |
| 0x566350 | sub_566350 | 540 | title_menu.h |
| 0x568b40 | sub_568B40 | 659 | title_menu.h |
| 0x568de0 | sub_568DE0 | 998 | title_menu.h |
| 0x56a220 | sub_56A220 | 66 | title_menu.h |
| 0x56bfd0 | sub_56BFD0 | 88 | title_menu.h |
| 0x56cb90 | sub_56CB90 | 113 | title_menu.h |
| 0x56cc10 | sub_56CC10 | 267 | title_menu.h |
| 0x56d710 | sub_56D710 | 510 | title_menu.h |
| 0x56d910 | sub_56D910 | 272 | title_menu.h |
| 0x56da40 | sub_56DA40 | 96 | title_menu.h |
| 0x56daa0 | sub_56DAA0 | 267 | title_menu.h |
| 0x56de80 | sub_56DE80 | 67 | title_menu.h |
| 0x56ded0 | sub_56DED0 | 61 | title_menu.h |
| 0x56df10 | sub_56DF10 | 203 | sprite_slot.h |
| 0x56dfe0 | sub_56DFE0 | 417 | title_menu.h |
| 0x5851e0 | sub_5851E0 | 114 | scene_loop.h |
| 0x585f00 | sub_585F00 | 258 | scene_loop.h |
| 0x5878a0 | sub_5878A0 | 207 | room_load.h |
| 0x587ce0 | sub_587CE0 | 71 | room_load.h |
| 0x587d30 | sub_587D30 | 28 | room_load.h |
| 0x587d50 | sub_587D50 | 96 | room_load.h |
| 0x587db0 | sub_587DB0 | 76 | room_load.h |
| 0x58cf60 | sub_58CF60 | 83 | room_load.h |
| 0x58cfc0 | sub_58CFC0 | 27 | room_load.h |
| 0x58cfe0 | sub_58CFE0 | 63 | room_load.h |
| 0x58d020 | sub_58D020 | 51 | room_load.h |
| 0x58d060 | sub_58D060 | 45 | room_load.h |
| 0x58d090 | sub_58D090 | 18 | room_loop.h |
| 0x58d0b0 | sub_58D0B0 | 337 | room_load.h |
| 0x58d210 | sub_58D210 | 403 | room_load.h |
| 0x58d3b0 | sub_58D3B0 | 165 | room_load.h |
| 0x58d460 | sub_58D460 | 3341 | room_load.h |
| 0x58e170 | sub_58E170 | 441 | party_member.h |
| 0x58e330 | sub_58E330 | 873 | room_load.h |
| 0x58e6a0 | sub_58E6A0 | 218 | room_load.h |
| 0x58e780 | sub_58E780 | 1823 | room_loop.h |
| 0x58eeb0 | sub_58EEB0 | 479 | room_loop.h |
| 0x58f0b0 | sub_58F0B0 | 658 | room_loop.h |
| 0x58f360 | sub_58F360 | 3030 | room_loop.h |
| 0x58ff50 | sub_58FF50 | 66 | room_loop.h |
| 0x58ffc0 | sub_58FFC0 | 14 | room_tick.h |
| 0x58ffd0 | sub_58FFD0 | 595 | room_loop.h |
| 0x590230 | sub_590230 | 478 | shots.h |
| 0x590410 | sub_590410 | 112 | room_loop.h |
| 0x590660 | sub_590660 | 566 | room_tick.h |
| 0x5908a0 | sub_5908A0 | 1011 | room_tick.h |
| 0x59a1f0 | sub_59A1F0 | 1485 | gimmick.h |
| 0x59a7c0 | sub_59A7C0 | 181 | gimmick.h |
| 0x59a880 | sub_59A880 | 138 | gimmick.h |
| 0x59a910 | sub_59A910 | 302 | room_tick.h |
| 0x59aa40 | sub_59AA40 | 630 | room_tick.c, room_tick.h |
| 0x59acc0 | sub_59ACC0 | 90 | room_tick.h |
| 0x59ad20 | sub_59AD20 | 60 | room_tick.h |
| 0x59afd0 | sub_59AFD0 | 374 | gimmick.c, gimmick.h |
| 0x59b170 | sub_59B170 | 2621 | gimmick.c, gimmick.h |
| 0x59bdd0 | sub_59BDD0 | 1129 | gimmick.h |
| 0x59c240 | sub_59C240 | 929 | gimmick.h |
| 0x59c750 | sub_59C750 | 702 | room_tick.c, room_tick.h |
| 0x59ca10 | sub_59CA10 | 114 | room_loop.h |
| 0x59ca90 | sub_59CA90 | 975 | room_tick.c, room_tick.h |
| 0x59ce60 | sub_59CE60 | 350 | room_tick.h |
| 0x59cfc0 | sub_59CFC0 | 405 | scene_loop.h |
| 0x59d3f0 | sub_59D3F0 | 323 | room_loop.h |
| 0x59db70 | sub_59DB70 | 227 | room_load.h |
| 0x59e1a0 | sub_59E1A0 | 34 | scene_loop.h |
| 0x59ec30 | sub_59EC30 | 531 | title_menu.h |
| 0x59ee50 | sub_59EE50 | 1132 | title_menu.h |
| 0x5a3bb0 | sub_5A3BB0 | 62 | scene_loop.h |
| 0x5a3bf0 | sub_5A3BF0 | 27 | scene_loop.h |
| 0x5a3c10 | sub_5A3C10 | 37 | scene_loop.h |
| 0x5a3c40 | sub_5A3C40 | 404 | scene_loop.h |
| 0x5a3de0 | sub_5A3DE0 | 49 | scene_loop.h |
| 0x5a3e20 | sub_5A3E20 | 455 | scene_loop.h |
| 0x5a3ff0 | sub_5A3FF0 | 516 | scene_loop.h |
| 0x5a4200 | sub_5A4200 | 276 | scene_loop.h |
| 0x5a4460 | sub_5A4460 | 30 | scene_loop.h |
| 0x5a4480 | sub_5A4480 | 312 | scene_loop.h |
| 0x5b15e0 | sub_5B15E0 | 1124 | char_attack.h |
| 0x5b1aa0 | sub_5B1AA0 | 405 | arche_pose.h |
| 0x5b1c50 | sub_5B1C50 | 699 | char_attack.h |
| 0x5b28a0 | sub_5B28A0 | 3020 | char_handlers.h |
| 0x5b34d0 | sub_5B34D0 | 277 | char_hit.h |
| 0x5b3890 | sub_5B3890 | 615 | char_attack.h |
| 0x5b3b00 | sub_5B3B00 | 1002 | char_handlers.h |
| 0x5b3f00 | sub_5B3F00 | 545 | char_hit.h |
| 0x5b4130 | sub_5B4130 | 60 | char_hit.h |
| 0x5b4170 | sub_5B4170 | 133 | char_hit.h |
| 0x5b4200 | sub_5B4200 | 295 | behave_helpers.h |
| 0x5b4330 | sub_5B4330 | 469 | script_helpers.h |
| 0x5b4510 | sub_5B4510 | 26 | arche_pose.h |
| 0x5b4530 | sub_5B4530 | 1209 | party_member.h |
| 0x5b4a10 | sub_5B4A10 | 741 | party_member.h |
| 0x5b4d00 | sub_5B4D00 | 792 | char_hit.h |
| 0x5b5020 | sub_5B5020 | 286 | char_hit.h |
| 0x5b5140 | sub_5B5140 | 833 | char_hit.h |
| 0x5b54d0 | sub_5B54D0 | 166 | char_state.h |
| 0x5b5580 | sub_5B5580 | 121 | char_state.h |
| 0x5b5600 | sub_5B5600 | 701 | hud_view.h |
| 0x5b58c0 | sub_5B58C0 | 204 | hud_view.h |
| 0x5b5990 | sub_5B5990 | 202 | char_state.h |
| 0x5b5eb0 | sub_5B5EB0 | 59 | render_platform.h |
| 0x5b6300 | sub_5B6300 | 60 | scene_loop.h |
| 0x5b9450 | sub_5B9450 | 52 | render_platform.h |
| 0x5bf3ee | emit_last_error_line_5BF3EE | 82 | cs_dispatch_win32.c |
| 0x5bf440 | _fclose | 86 | cs_dispatch_win32.c |
| 0x5bf496 | _fprintf | 50 | cs_dispatch_win32.c |
| 0x5bf4fb | sub_5BF4FB | 10 | rng.c, rng.h |
| 0x5bf6df | _time | 220 | rng.h |

