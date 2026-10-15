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
- touched: **595** (38.0% of engine-proper) — tested 249, ported 346
- code bytes touched: **29.8%** (530,187 / 1,776,526 B of engine-proper)
- unported: **1229**
- orphan refs in src/ not in this table: 0

## tested (249) — ported + host unit suite

| VA | name | size | src |
|----|------|-----:|-----|
| 0x4031c0 | sub_4031C0 | 815 | glyph_wrap.h |
| 0x406440 | cs_fail_406440 | 184 | cs_dispatch.c, cs_dispatch.h, cs_dispatch_win32.c |
| 0x408b90 | sub_408B90 | 419 | wnd_proc.h |
| 0x40a540 | sub_40A540 | 133 | char_integrate.h, character.h |
| 0x40dee0 | sub_40DEE0 | 85 | main.c |
| 0x40e0c0 | menu_ctrl_clear_40E0C0 | 555 | menu_list.c, menu_list.h, room_load.h |
| 0x40e360 | sub_40E360 | 636 | main.c |
| 0x40e5e0 | wrap_class_40E5E0 | 2553 | glyph_wrap.c, glyph_wrap.h, main.c |
| 0x40f040 | wrap_parse_40F040 | 379 | glyph_wrap.c, glyph_wrap.h |
| 0x40f3e0 | sub_40F3E0 | 434 | menu_list.c, menu_list.h, newgame_menu.c |
| 0x40f5c0 | menu_ctrl_build_40F5C0 | 563 | menu_list.c, menu_list.h |
| 0x40fa00 | sub_40FA00 | 800 | glyph_text.c, glyph_text.h, newgame_menu.c (+3) |
| 0x40fd20 | glyph_token_search_40FD20 | 143 | glyph_text.c, glyph_text.h |
| 0x411530 | sub_411530 | 38 | char_ai.h |
| 0x411940 | sub_411940 | 770 | main.c, newgame_picker.c, newgame_picker.h |
| 0x411f40 | sub_411F40 | 444 | menu_list.c, menu_list.h, newgame_menu.c (+1) |
| 0x412160 | sub_412160 | 459 | newgame_menu.c, newgame_menu.h, newgame_picker.c (+1) |
| 0x412c10 | sub_412C10 | 46 | obj_container.c, obj_container.h |
| 0x414080 | sub_414080 | 63 | obj_container.c, obj_container.h |
| 0x417870 | sub_417870 | 106 | map_decode.h, sprite_slot.h |
| 0x4178e0 | sub_4178E0 | 194 | asset_register.c, asset_register.h, bitmap_session.c (+1) |
| 0x4179b0 | sub_4179B0 | 415 | asset_register.c, asset_register.h, sprite_slot.h (+1) |
| 0x417b50 | sub_417B50 | 109 | asset_register.c, asset_register.h, sprite_slot.h |
| 0x417bc0 | ar_pal_swap_table_for_va_417BC0 | 123 | asset_register.c, asset_register.h, sprite_slot.h |
| 0x417c40 | game_arm_parallax_grade_417C40 | 1625 | actor_render.h, actor_spawn.c, color_grade.c (+2) |
| 0x418470 | sub_418470 | 40 | asset_register.c, asset_register.h, color_grade.h (+4) |
| 0x4184a0 | sub_4184A0 | 1035 | asset_register.c, asset_register.h, bitmap_session.h (+1) |
| 0x4188b0 | sub_4188B0 | 891 | asset_register.c, asset_register.h |
| 0x4192b0 | sub_4192B0 | 52 | menu_list.c, menu_list.h, newgame_picker.c |
| 0x419900 | picker_seek_value_419900 | 128 | newgame_picker.c, newgame_picker.h |
| 0x426110 | sub_426110 | 610 | cs_dispatch.c, cs_dispatch.h, cs_dispatch_win32.c |
| 0x426f50 | sub_426F50 | 26 | char_integrate.h, character.c |
| 0x426fd0 | sub_426FD0 | 781 | base_stat_table.c, base_stat_table.h, char_def.h (+2) |
| 0x4385e0 | sub_4385E0 | 45 | char_physics.c, char_physics.h |
| 0x43b980 | sub_43B980 | 783 | cutscene.c, dialogue.h |
| 0x43bca0 | sub_43BCA0 | 1105 | cutscene.c, dialogue.h, newgame_drive.c (+1) |
| 0x43c110 | sub_43C110 | 84 | input.c, input.h, newgame_scene.h (+1) |
| 0x43c2e0 | sub_43C2E0 | 795 | main.c |
| 0x43ca40 | menu_list_nav_43CA40 | 970 | cutscene.c, dialogue.c, dialogue.h (+2) |
| 0x43ce50 | menu_list_latch_43CE50 | 220 | cutscene.c, dialogue.h, menu_list.c (+1) |
| 0x43d1d0 | camera_follow_step_43D1D0 | 366 | camera_follow.c, camera_follow.h, main.c |
| 0x43d340 | camera_shake_apply_43D340 | 299 | camera_follow.c, camera_follow.h |
| 0x43e250 | sub_43E250 | 334 | main.c |
| 0x440f40 | sub_440F40 | 670 | char_ai.c, char_ai.h |
| 0x4411e0 | sub_4411E0 | 131 | char_ai.c, char_ai.h |
| 0x4416c0 | sub_4416C0 | 52 | entity_move.c, entity_move.h |
| 0x44d160 | sub_44D160 | 379 | actor_render.c, actor_render.h, actor_spawn.c (+3) |
| 0x44e640 | sub_44E640 | 49 | hud.c, hud.h, npc_ai.h |
| 0x44e680 | sub_44E680 | 59 | hud.c, hud.h, npc_ai.h |
| 0x466930 | sub_466930 | 1315 | char_physics.c, char_physics.h |
| 0x467bb0 | sub_467BB0 | 3364 | char_physics.c, char_physics.h |
| 0x46a880 | sub_46A880 | 8890 | input_dik.c, input_dik.h, input_live.c (+1) |
| 0x4766a0 | hud_door_on_screen_4766A0 | 106 | hud.c, hud.h |
| 0x478ba0 | char_ai_478BA0 | 3514 | char_ai.c, char_ai.h |
| 0x479960 | sub_479960 | 165 | character.c, character.h, input.c (+1) |
| 0x479a10 | sub_479A10 | 644 | input.h |
| 0x479ca0 | sub_479CA0 | 134 | input.h |
| 0x479d30 | sub_479D30 | 99 | char_ai.h |
| 0x479da0 | sub_479DA0 | 62 | char_ai.h |
| 0x479de0 | sub_479DE0 | 34 | char_ai.h |
| 0x479e70 | sub_479E70 | 275 | char_ai.h, character.c, character.h (+2) |
| 0x48c820 | sub_48C820 | 873 | newgame_cursor.h |
| 0x48cf80 | widget_box_frame_48CF80 | 1095 | main.c, newgame_box.c, newgame_box.h |
| 0x48d3d0 | sub_48D3D0 | 664 | newgame_box.c |
| 0x48d670 | sub_48D670 | 154 | newgame_box.c |
| 0x48d940 | sub_48D940 | 300 | main.c, newgame_cursor.c, newgame_cursor.h |
| 0x48e200 | sub_48E200 | 1221 | glyph_render.c, glyph_render.h, main.c |
| 0x48e6d0 | glyph_ruby_draw_48E6D0 | 389 | glyph_render.c, glyph_render.h |
| 0x48e860 | sub_48E860 | 181 | glyph_render.c, glyph_render.h, main.c |
| 0x48e920 | scene_fade_grid_render_48E920 | 403 | main.c, scene_fade.c, scene_fade.h |
| 0x48eac0 | sub_48EAC0 | 1131 | main.c, map_present.c, map_present.h (+2) |
| 0x490b90 | sub_490B90 | 307 | dialogue.c, map_present.c, map_present.h (+1) |
| 0x490cd0 | sub_490CD0 | 603 | color_grade.h, main.c, parallax.c (+3) |
| 0x490f30 | sub_490F30 | 2002 | color_grade.h, draw_pool.h, main.c (+6) |
| 0x491770 | sub_491770 | 52 | asset_register.c, asset_register.h, sprite_slot.h |
| 0x4917b0 | draw_pool_emit_layered_4917B0 | 106 | draw_pool.c, draw_pool.h, map_render.c |
| 0x492670 | draw_pool_emit_actor_492670 | 118 | actor_render.h, actor_spawn.h, draw_pool.c (+2) |
| 0x494e60 | sub_494E60 | 3904 | base_stat_table.h, hud.c, hud.h (+2) |
| 0x4962a0 | sub_4962A0 | 242 | hud.c, hud.h, main.c |
| 0x4969b0 | sub_4969B0 | 1272 | hud.c, hud.h, main.c |
| 0x497b40 | party_stat_hp_display_497B40 | 98 | party.c |
| 0x497bb0 | party_stat_mp_display_497BB0 | 98 | party.c |
| 0x498620 | sub_498620 | 82 | hud.h, main.c |
| 0x498680 | hud_bar_row_geom_498680 | 401 | hud.c, hud.h, main.c |
| 0x498f10 | sub_498F10 | 344 | hud.h, main.c |
| 0x499560 | parallax_strip_499560 | 271 | color_grade.h, parallax.c, parallax.h |
| 0x499ab0 | scene_cinematic_step_499AB0 | 1356 | camera_follow.h |
| 0x49a740 | sf_pattern_edges_in_49A740 | 336 | scene_fade.c, scene_fade.h |
| 0x49a890 | sf_pattern_center_out_49A890 | 361 | scene_fade.c, scene_fade.h |
| 0x49aa00 | sf_pattern_sweep_down_49AA00 | 216 | scene_fade.c, scene_fade.h |
| 0x49aae0 | sf_pattern_sweep_up_49AAE0 | 234 | scene_fade.c, scene_fade.h |
| 0x49c640 | sub_49C640 | 720 | dialogue.c |
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
| 0x562a10 | sub_562A10 | 92 | asset_register.c, asset_register.h |
| 0x562ea0 | build_title_drive_562EA0 | 4062 | app_flow.c, app_flow.h, asset_register.c (+10) |
| 0x563ef0 | sub_563EF0 | 445 | asset_register.c, asset_register.h |
| 0x564160 | sub_564160 | 376 | newgame_drive.c, newgame_drive.h |
| 0x564780 | sub_564780 | 4067 | newgame_drive.c, newgame_drive.h, newgame_menu.c (+4) |
| 0x5657f0 | newgame_picker_dispatch_5657F0 | 31 | newgame_drive.c, newgame_picker.c, newgame_picker.h |
| 0x565d10 | sub_565D10 | 1228 | main.c, newgame_drive.c, newgame_drive.h |
| 0x566570 | newgame_option_label_566570 | 485 | newgame_menu.c, newgame_menu.h, newgame_picker.h |
| 0x566850 | sub_566850 | 337 | newgame_menu.c, newgame_menu.h, newgame_scene.c (+1) |
| 0x566a80 | newgame_option_value_566A80 | 2781 | newgame_menu.c, newgame_menu.h, newgame_picker.c (+2) |
| 0x567ba0 | sub_567BA0 | 1647 | main.c, newgame_drive.c, newgame_drive.h (+2) |
| 0x568320 | sub_568320 | 964 | newgame_drive.c, newgame_picker.c, newgame_picker.h |
| 0x56aea0 | sub_56AEA0 | 4294 | app_flow.h, input.h, main.c (+11) |
| 0x56c030 | sub_56C030 | 62 | title_particles.c |
| 0x56c070 | drive_spawn_sparkle_56C070 | 265 | main.c, rng.h, title_particles.c (+1) |
| 0x56c180 | title_compositor_resolve_56C180 | 297 | main.c, title_particles.c, title_particles.h (+3) |
| 0x56c2b0 | sub_56C2B0 | 373 | main.c, title_particles.c, title_particles.h |
| 0x56c470 | sub_56C470 | 107 | title_render.c, title_render.h, title_sink.c |
| 0x56c4e0 | sub_56C4E0 | 155 | title_render.c, title_render.h |
| 0x56c580 | title_render_sparkle_56C580 | 133 | title_render.c, title_render.h, title_scene.c |
| 0x56c610 | title_draw_sprite_56C610 | 35 | title_render.c, title_render.h |
| 0x56c930 | menu_owner_transition_step_56C930 | 607 | menu_list.c, menu_list.h |
| 0x56cd20 | sub_56CD20 | 2275 | main.c, prologue_drive.c, prologue_drive.h (+2) |
| 0x56dc20 | sub_56DC20 | 344 | main.c |
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
| 0x5b8b00 | zdd_color_convert_5B8B00 | 59 | zdd.c, zdd.h |
| 0x5b8b40 | sub_5B8B40 | 184 | cs_dispatch.c, zdd.c, zdd.h |
| 0x5b8c00 | zdd_create_surface_5B8C00 | 372 | zdd.c, zdd.h, zdd_win32.c |
| 0x5b8da0 | sub_5B8DA0 | 33 | zdd.h |
| 0x5b8dd0 | sub_5B8DD0 | 33 | cs_dispatch.c, zdd.h |
| 0x5b8e00 | sub_5B8E00 | 157 | zdd.h, zdd_win32.c |
| 0x5b8ea0 | zdd_object_upscale_16bpp_5B8EA0 | 285 | zdd.c, zdd.h |
| 0x5b8fc0 | flip_5B8FC0 | 335 | call_trace.h, game_drive.h, main.c (+8) |
| 0x5b9130 | sub_5B9130 | 151 | main.c, wnd_proc.h, wnd_proc_win32.c (+3) |
| 0x5b91d0 | sub_5B91D0 | 109 | zdd.h |
| 0x5b9240 | sub_5B9240 | 60 | zdd.h |
| 0x5b9280 | sub_5B9280 | 203 | zdd.c, zdd.h |
| 0x5b9350 | sub_5B9350 | 50 | zdd.c, zdd.h |
| 0x5b9390 | title_frame_free_5B9390 | 75 | asset_register.c, asset_register.h, cs_dispatch.c (+4) |
| 0x5b93e0 | zdd_object_local_free_5B93E0 | 42 | zdd.h, zdd_win32.c |
| 0x5b9410 | zdd_object_clear_5B9410 | 49 | main.c, osr_format.h, zdd.c (+1) |
| 0x5b9490 | sub_5B9490 | 63 | zdd.c, zdd.h, zdd_win32.c |
| 0x5b94d0 | sub_5B94D0 | 15 | zdd.c, zdd.h, zdd_win32.c |
| 0x5b94e0 | sub_5B94E0 | 29 | wnd_proc.h, zdd.c, zdd.h (+1) |
| 0x5b9500 | sub_5B9500 | 17 | wnd_proc.h, zdd.c, zdd.h (+1) |
| 0x5b9520 | sub_5B9520 | 157 | zdd.c, zdd.h, zdd_win32.c |
| 0x5b95c0 | sub_5B95C0 | 110 | zdd.c, zdd.h |
| 0x5b9630 | sub_5B9630 | 268 | zdd.c, zdd.h |
| 0x5b9740 | zdd_get_attached_surface_5B9740 | 153 | zdd.h, zdd_win32.c |
| 0x5b97e0 | sub_5B97E0 | 66 | zdd.c, zdd.h |
| 0x5b9830 | zdd_surface_set_color_key_5B9830 | 138 | zdd.c, zdd.h, zdd_win32.c |
| 0x5b98c0 | sub_5B98C0 | 73 | zdd.c, zdd.h |
| 0x5b9910 | sub_5B9910 | 291 | zdd.c, zdd.h |
| 0x5b9a40 | blt_onto_5B9A40 | 112 | main.c, parallax.h, town_render.h (+2) |
| 0x5b9ab0 | sub_5B9AB0 | 10 | zdd.h |
| 0x5b9ac0 | sub_5B9AC0 | 22 | zdd.c, zdd.h |
| 0x5b9ae0 | blt_rects_5B9AE0 | 140 | zdd.c, zdd.h |
| 0x5b9b70 | blt_keyed_5B9B70 | 122 | hud.h, main.c, map_present.c (+4) |
| 0x5b9bf0 | blt_clipped_5B9BF0 | 256 | main.c, map_present.c, map_present.h (+3) |
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
| 0x5bd550 | blt_alpha_5BD550 | 302 | hud.h, main.c, map_present.c (+7) |
| 0x5bd680 | sub_5BD680 | 1072 | zdd.c, zdd.h |
| 0x5bef0e | ??3@YAXPAX@Z | 11 | asset_register.c, asset_register.h, cs_dispatch.c (+4) |
| 0x5bf4e8 | _fopen | 19 | cs_dispatch_win32.c, wnd_proc.h |
| 0x5bf505 | rand_draw_5BF505 | 30 | cutscene.c, main.c, rng.c (+1) |
| 0x5bf5db | _exit | 17 | app_pump.c, app_pump.h, app_pump_win32.c (+5) |
| 0x5bf5fd | _doexit | 153 | wnd_proc.h |
| 0x5c0907 | hex32_lower_5C0907 | 45 | zdd.c, zdd.h |
| 0x5c0934 | _xtoa | 92 | zdd.c |
| 0x5c123f | _free | 47 | asset_register.c |

## ported (346) — reimplemented, no host test yet

| VA | name | size | src |
|----|------|-----:|-----|
| 0x4017a0 | sub_4017A0 | 40 | scene_loop.h |
| 0x4017d0 | sub_4017D0 | 1175 | world_alloc.c, world_alloc.h |
| 0x401c70 | sub_401C70 | 54 | world_alloc.h |
| 0x401cb0 | sub_401CB0 | 140 | world_alloc.h |
| 0x401f50 | sub_401F50 | 222 | script_engine.h |
| 0x402060 | sub_402060 | 62 | script_engine.h |
| 0x4022a0 | sub_4022A0 | 47 | npc_ai.h |
| 0x4022d0 | sub_4022D0 | 88 | room_load.h |
| 0x402a60 | sub_402A60 | 435 | char_state.h |
| 0x402f20 | sub_402F20 | 21 | script_engine.h |
| 0x4060d0 | sub_4060D0 | 241 | char_state.h |
| 0x4066d0 | sub_4066D0 | 989 | party_member.h |
| 0x406ec0 | sub_406EC0 | 813 | party_member.h |
| 0x4071f0 | sub_4071F0 | 196 | party_member.h |
| 0x4072c0 | sub_4072C0 | 85 | char_def.h |
| 0x407320 | sub_407320 | 190 | char_integrate.h, party_member.h |
| 0x407b80 | sub_407B80 | 32 | char_integrate.h |
| 0x407ba0 | sub_407BA0 | 907 | sound_fx.h |
| 0x408100 | sub_408100 | 205 | gimmick.h |
| 0x408d40 | sub_408D40 | 124 | party_member.h |
| 0x408dc0 | sub_408DC0 | 714 | party_member.h |
| 0x409090 | sub_409090 | 67 | party_member.h |
| 0x4090e0 | sub_4090E0 | 107 | char_def.h |
| 0x4094d0 | sub_4094D0 | 71 | party_member.h |
| 0x4099a0 | sub_4099A0 | 1251 | sound_fx.h |
| 0x409e90 | sub_409E90 | 375 | sound_fx.h |
| 0x40a010 | sub_40A010 | 645 | sound_fx.h |
| 0x40ab80 | sub_40AB80 | 136 | actor_ops.h |
| 0x40ac10 | sub_40AC10 | 127 | gimmick.h |
| 0x40afe0 | sub_40AFE0 | 1640 | gimmick.c, gimmick.h |
| 0x40b710 | sub_40B710 | 480 | sound_fx.h |
| 0x40b8f0 | sub_40B8F0 | 626 | event.c, event.h |
| 0x40bb90 | sub_40BB90 | 502 | sound_fx.h |
| 0x40bd90 | sub_40BD90 | 110 | entity_life.h |
| 0x40be00 | sub_40BE00 | 36 | gimmick.h |
| 0x40be30 | sub_40BE30 | 1032 | gimmick.c, gimmick.h |
| 0x40c300 | sub_40C300 | 120 | gimmick.h |
| 0x40c380 | sub_40C380 | 5077 | script_engine.c, script_engine.h |
| 0x40d960 | sub_40D960 | 69 | char_state.h |
| 0x40d9b0 | sub_40D9B0 | 437 | room_loop.h |
| 0x411390 | sub_411390 | 413 | sound_fx.h |
| 0x411560 | sub_411560 | 117 | gimmick.h |
| 0x4115e0 | sub_4115E0 | 33 | room_loop.h |
| 0x4118b0 | sub_4118B0 | 134 | sprite_slot.h |
| 0x412db0 | sub_412DB0 | 57 | room_loop.h |
| 0x413810 | sub_413810 | 444 | party_member.h |
| 0x413a00 | sub_413A00 | 286 | party_member.h |
| 0x413b20 | sub_413B20 | 68 | party_member.h |
| 0x415e20 | sub_415E20 | 320 | party_member.h |
| 0x416420 | sub_416420 | 115 | party_member.h |
| 0x416570 | sub_416570 | 547 | party_member.h |
| 0x4182d0 | sub_4182D0 | 408 | room_load.h |
| 0x419c00 | sub_419C00 | 110 | scene_loop.h |
| 0x41a7f0 | sub_41A7F0 | 152 | scene_loop.h |
| 0x41dc40 | sub_41DC40 | 75 | party_member.h |
| 0x41dc90 | sub_41DC90 | 890 | room_load.h |
| 0x41e020 | sub_41E020 | 70 | room_load.h |
| 0x41e180 | sub_41E180 | 244 | char_state.h |
| 0x41e280 | sub_41E280 | 47 | char_def.h |
| 0x41e2f0 | sub_41E2F0 | 66 | script_engine.h |
| 0x41e440 | sub_41E440 | 125 | char_def.h |
| 0x41e600 | sub_41E600 | 907 | actor_ops.h |
| 0x41ea70 | sub_41EA70 | 187 | entity_life.h |
| 0x41ee60 | sub_41EE60 | 252 | char_state.h |
| 0x41ef80 | sub_41EF80 | 83 | char_state.h |
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
| 0x431930 | sub_431930 | 599 | char_def.h |
| 0x431b90 | sub_431B90 | 15 | char_def.h |
| 0x431ba0 | sub_431BA0 | 266 | char_def.h |
| 0x431cb0 | sub_431CB0 | 87 | char_def.h |
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
| 0x439680 | sub_439680 | 9 | script_engine.h |
| 0x43c9b0 | sub_43C9B0 | 130 | room_load.h |
| 0x43e140 | sub_43E140 | 266 | room_loop.h |
| 0x43e3a0 | sub_43E3A0 | 516 | event.c, event.h |
| 0x43ee30 | sub_43EE30 | 228 | npc_ai.h |
| 0x43f880 | sub_43F880 | 5491 | npc_ai.c, npc_ai.h |
| 0x440e40 | sub_440E40 | 239 | npc_ai.h |
| 0x441270 | sub_441270 | 84 | npc_ai.h |
| 0x4412d0 | sub_4412D0 | 993 | npc_ai.c, npc_ai.h |
| 0x441700 | sub_441700 | 466 | npc_ai.h |
| 0x4418e0 | sub_4418E0 | 511 | npc_ai.h |
| 0x441ae0 | sub_441AE0 | 749 | npc_ai.h |
| 0x442460 | sub_442460 | 38 | actor_update.c, actor_update.h |
| 0x442890 | sub_442890 | 54 | char_integrate.h |
| 0x442920 | sub_442920 | 149 | char_integrate.h |
| 0x4429c0 | sub_4429C0 | 86 | npc_ai.h |
| 0x442a20 | sub_442A20 | 76 | npc_ai.h |
| 0x442a70 | sub_442A70 | 12026 | char_integrate.c, char_integrate.h |
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
| 0x4467d0 | sub_4467D0 | 289 | entity_life.h |
| 0x448c80 | sub_448C80 | 47 | particle_init.h |
| 0x448cb0 | sub_448CB0 | 264 | entity_fx.c, entity_fx.h |
| 0x44bbc0 | sub_44BBC0 | 66 | entity_fx.c, entity_fx.h |
| 0x450e00 | sub_450E00 | 237 | entity_fx.h |
| 0x450ef0 | sub_450EF0 | 776 | sound_fx.h |
| 0x451200 | sub_451200 | 79 | entity_fx.c, entity_fx.h |
| 0x451250 | sub_451250 | 119 | char_integrate.h |
| 0x4537d0 | sub_4537D0 | 283 | entity_fx.c, entity_fx.h |
| 0x453960 | velocity_scatter_453960 | 70 | char_state.h |
| 0x457ff0 | sub_457FF0 | 4675 | arche_pose.c, arche_pose.h |
| 0x459370 | sub_459370 | 71 | arche_pose.h |
| 0x4593c0 | sub_4593C0 | 32 | char_state.h |
| 0x4593e0 | sub_4593E0 | 1399 | arche_pose.h |
| 0x459990 | sub_459990 | 1313 | arche_pose.h |
| 0x459ed0 | sub_459ED0 | 72 | arche_pose.h |
| 0x460e40 | sub_460E40 | 143 | particle_init.h |
| 0x4671c0 | sub_4671C0 | 158 | char_integrate.h |
| 0x467380 | ambient_event_timer_467380 | 376 | sound_fx.h |
| 0x4678f0 | sub_4678F0 | 482 | arche_pose.h |
| 0x467ae0 | sub_467AE0 | 199 | arche_pose.h |
| 0x468a20 | sub_468A20 | 237 | room_loop.h |
| 0x46cd70 | actor_update_all_46CD70 | 1031 | world_tick.c, world_tick.h |
| 0x46d180 | sub_46D180 | 798 | entity_fx.c, entity_fx.h |
| 0x46d4a0 | sub_46D4A0 | 838 | entity_fx.c, entity_fx.h |
| 0x46d7f0 | sub_46D7F0 | 521 | entity_fx.c, entity_fx.h |
| 0x46e510 | sub_46E510 | 10712 | particle_tick.c, particle_tick.h |
| 0x471080 | sub_471080 | 49 | particle_tick.c, particle_tick.h |
| 0x476710 | sub_476710 | 44 | particle_init.h |
| 0x476880 | sub_476880 | 509 | sound_fx.h |
| 0x476c70 | sub_476C70 | 144 | particle_init.h |
| 0x476d00 | sub_476D00 | 45 | particle_init.h |
| 0x47b770 | sub_47B770 | 77 | behave_ops.h |
| 0x47b7c0 | sub_47B7C0 | 163 | gimmick.h |
| 0x47b990 | actor_ai_47B990 | 7461 | behave_ops.h, char_behave.c, char_behave.h |
| 0x47d740 | sub_47D740 | 26 | actor_update.c, actor_update.h |
| 0x47d760 | sub_47D760 | 47 | behave_ops.h |
| 0x47dbb0 | sub_47DBB0 | 655 | behave_ops.h |
| 0x47e330 | sub_47E330 | 1120 | behave_ops.h |
| 0x47e790 | sub_47E790 | 37 | npc_ai.h |
| 0x4804b0 | sub_4804B0 | 207 | gimmick.h |
| 0x4812c0 | sub_4812C0 | 79 | gimmick.h |
| 0x4842c0 | sub_4842C0 | 244 | npc_ai.h |
| 0x484bf0 | sub_484BF0 | 31 | behave_ops.h |
| 0x484c10 | sub_484C10 | 322 | behave_ops.h |
| 0x4852c0 | sub_4852C0 | 37 | behave_ops.h |
| 0x4852f0 | sub_4852F0 | 55 | behave_ops.h |
| 0x485670 | sub_485670 | 90 | behave_ops.h |
| 0x4856d0 | sub_4856D0 | 83 | behave_ops.h |
| 0x485740 | sub_485740 | 166 | behave_ops.h |
| 0x4857f0 | sub_4857F0 | 404 | behave_ops.h |
| 0x485f30 | sub_485F30 | 55 | behave_ops.h |
| 0x485f70 | sub_485F70 | 68 | char_state.h |
| 0x485fc0 | sub_485FC0 | 4709 | char_apply.c, char_apply.h |
| 0x487250 | sub_487250 | 114 | char_state.h |
| 0x4872d0 | sub_4872D0 | 474 | char_state.h |
| 0x4874b0 | sub_4874B0 | 115 | char_state.h |
| 0x487530 | sub_487530 | 54 | char_state.h |
| 0x487570 | sub_487570 | 437 | char_state.h |
| 0x487730 | sub_487730 | 366 | char_state.h |
| 0x489280 | sub_489280 | 818 | sound_fx.h |
| 0x491740 | sub_491740 | 38 | sprite_slot.h |
| 0x49a050 | sub_49A050 | 13 | sprite_slot.h |
| 0x49be60 | sub_49BE60 | 290 | room_loop.h |
| 0x49cb60 | sub_49CB60 | 527 | script_engine.h |
| 0x49cd70 | sub_49CD70 | 254 | script_engine.h |
| 0x4afda0 | sub_4AFDA0 | 3 | script_engine.h |
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
| 0x4cc5a0 | sub_4CC5A0 | 628 | scene_loop.h |
| 0x4e1950 | sub_4E1950 | 135 | scene_loop.h |
| 0x4e6030 | sub_4E6030 | 54 | party_member.h |
| 0x4e6070 | sub_4E6070 | 299 | party_member.h |
| 0x4e61a0 | sub_4E61A0 | 33 | scene_loop.h |
| 0x54bf90 | sub_54BF90 | 30 | room_load.h |
| 0x54bfb0 | sub_54BFB0 | 801 | world_alloc.c, world_alloc.h |
| 0x54c2e0 | sub_54C2E0 | 217 | world_alloc.h |
| 0x54c3c0 | sub_54C3C0 | 358 | world_alloc.h |
| 0x54c640 | sub_54C640 | 812 | grid_refresh.c, grid_refresh.h |
| 0x54c9d0 | sub_54C9D0 | 214 | grid_refresh.c, grid_refresh.h |
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
| 0x556de0 | sub_556DE0 | 98 | gimmick.h |
| 0x556eb0 | sub_556EB0 | 62 | actor_update.c, actor_update.h, script_engine.h |
| 0x556ef0 | sub_556EF0 | 349 | device_init.h |
| 0x557370 | particle_spawn_557370 | 452 | particle_spawn.c, particle_spawn.h |
| 0x557550 | sub_557550 | 21735 | particle_init.c, particle_init.h |
| 0x55cbc0 | sub_55CBC0 | 394 | particle_init.h |
| 0x55cd50 | sub_55CD50 | 85 | particle_init.h |
| 0x55cdb0 | sub_55CDB0 | 145 | particle_init.h |
| 0x55ce50 | sub_55CE50 | 18 | particle_init.h |
| 0x55f790 | sub_55F790 | 1558 | scene_loop.h |
| 0x560e90 | sub_560E90 | 700 | scene_loop.h |
| 0x561150 | sub_561150 | 126 | scene_loop.h |
| 0x5611d0 | sub_5611D0 | 173 | scene_loop.h |
| 0x561280 | sub_561280 | 37 | scene_loop.h |
| 0x5612b0 | sub_5612B0 | 56 | scene_loop.h |
| 0x5614e0 | sub_5614E0 | 113 | scene_loop.h |
| 0x562a70 | sub_562A70 | 127 | scene_loop.h |
| 0x562ba0 | sub_562BA0 | 432 | scene_loop.h |
| 0x562d50 | sub_562D50 | 239 | room_load.h |
| 0x562e40 | sub_562E40 | 30 | scene_loop.h |
| 0x562e60 | sub_562E60 | 58 | scene_loop.h |
| 0x56df10 | sub_56DF10 | 203 | sprite_slot.h |
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
| 0x58ffd0 | sub_58FFD0 | 595 | room_loop.h |
| 0x590410 | sub_590410 | 112 | room_loop.h |
| 0x59a1f0 | sub_59A1F0 | 1485 | gimmick.h |
| 0x59a7c0 | sub_59A7C0 | 181 | gimmick.h |
| 0x59a880 | sub_59A880 | 138 | gimmick.h |
| 0x59afd0 | sub_59AFD0 | 374 | gimmick.c, gimmick.h |
| 0x59b170 | sub_59B170 | 2621 | gimmick.c, gimmick.h |
| 0x59bdd0 | sub_59BDD0 | 1129 | gimmick.h |
| 0x59c240 | sub_59C240 | 929 | gimmick.h |
| 0x59ca10 | sub_59CA10 | 114 | room_loop.h |
| 0x59cfc0 | sub_59CFC0 | 405 | scene_loop.h |
| 0x59d3f0 | sub_59D3F0 | 323 | room_loop.h |
| 0x59db70 | sub_59DB70 | 227 | room_load.h |
| 0x59e1a0 | sub_59E1A0 | 34 | scene_loop.h |
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
| 0x5b1aa0 | sub_5B1AA0 | 405 | arche_pose.h |
| 0x5b4510 | sub_5B4510 | 26 | arche_pose.h |
| 0x5b4530 | sub_5B4530 | 1209 | party_member.h |
| 0x5b4a10 | sub_5B4A10 | 741 | party_member.h |
| 0x5b54d0 | sub_5B54D0 | 166 | char_state.h |
| 0x5b5580 | sub_5B5580 | 121 | char_state.h |
| 0x5b5990 | sub_5B5990 | 202 | char_state.h |
| 0x5b6300 | sub_5B6300 | 60 | scene_loop.h |
| 0x5bf3ee | emit_last_error_line_5BF3EE | 82 | cs_dispatch_win32.c |
| 0x5bf440 | _fclose | 86 | cs_dispatch_win32.c |
| 0x5bf496 | _fprintf | 50 | cs_dispatch_win32.c |
| 0x5bf4fb | sub_5BF4FB | 10 | rng.c, rng.h |
| 0x5bf6df | _time | 220 | rng.h |

