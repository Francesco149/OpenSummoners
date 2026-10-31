/*
 * tests/test_main.c — unit-test driver for OpenSummoners ports.
 *
 *   make -C tests run             # all tests
 *   ./tests/build/run_tests       # invoke directly (after first build)
 *
 * Outcomes: 0 pass, 1 fail (with message), 2 skip.  Process exit is 0
 * iff failure count is 0.  Skips don't count as failures.
 *
 * Registration: add one X(name) line to TESTS for every test.  The
 * X-macro expands twice — once for the extern decl, once for the
 * dispatch table — so the test name appears exactly once and can't
 * drift from the declaration.
 */
#include "t.h"

typedef int (*test_fn)(void);

struct test_case {
    const char *name;
    test_fn     fn;
};

/* ─── test registry ───────────────────────────────────────────────────
 * One X(n) per test.  Grouped by ported module for readability.
 */
#define TESTS \
    X(ar_xfree_null_safe)                                      \
    X(ar_color_lerp_endpoints)                                 \
    X(ar_color_lerp_midpoint)                                  \
    X(ar_color_lerp_descending)                                \
    X(ar_color_lerp_ignores_alpha)                             \
    X(palette_pack_entry_basic)                                \
    X(palette_pack_entry_ignores_top_byte)                     \
    X(palette_pack_entry_overwrites_existing)                  \
    X(palette_install_lazy_allocates_first_time)               \
    X(palette_install_reuses_existing_buffer)                  \
    X(palette_install_destroy_frees_buffer)                    \
    X(sprite_destroy_frees_aux_and_entries)                    \
    X(ar_sprite_slot_frame_f38_bound)                          \
    X(sprite_destroy_safe_on_zero_slot)                        \
    X(sprite_register_writes_all_named_fields)                 \
    X(sprite_register_frees_existing_aux_and_entries)          \
    X(sprite_register_truncates_id_and_group_to_uint16)        \
    X(sprite_register_matches_FUN_005748c0_arg_shape)          \
    X(sprite_clone_copies_all_metadata)                        \
    X(sprite_clone_frees_dst_existing_aux_and_entries)         \
    X(sprite_clone_deep_copies_aux_buf)                        \
    X(sprite_clone_no_aux_when_src_aux_null)                   \
    X(sprite_clone_resets_dst_id_and_group_to_uint16)          \
    X(info_entry_clear_zeroes_marker_flag_data_palette)        \
    X(info_entry_clear_leaves_pad_untouched)                   \
    X(info_entry_pool_wired_by_state_init)                     \
    X(gdi_destroy_deletes_each_handle)                         \
    X(gdi_reset_clears_then_allocates)                         \
    X(make_font_family_courier)                                \
    X(make_font_family_times)                                  \
    X(make_font_family_arial)                                  \
    X(make_font_family_courier_italic)                         \
    X(make_font_unknown_family_skips_face)                     \
    X(set_font_writes_slot_and_handle)                         \
    X(set_font_destroys_existing)                              \
    X(set_pen_bumps_count)                                     \
    X(set_brush_bumps_count)                                   \
    X(set_pen_capacity_zero_no_op)                             \
    X(pen_gradient_endpoints_and_lerp)                         \
    X(pen_gradient_capacity_two)                               \
    X(pen_gradient_capacity_one)                               \
    X(register_fonts_sprite_slots)                             \
    X(register_fonts_gdi_indices)                              \
    X(register_fonts_dimensions_in_call_order)                 \
    X(sound_slot_init_writes_fields)                           \
    X(sound_slot_init_clears_state)                            \
    X(register_sounds_all_ids_and_kinds)                       \
    X(register_sounds_buffer_pointer_preserved)                \
    X(main_sprites_inline_slots_field_map)                     \
    X(main_sprites_transient_idx0_uses_sotesp_module)          \
    X(main_sprites_trailing_ids_in_index_order)                \
    X(main_sprites_untouched_indices_stay_zero)                \
    X(main_sprites_total_slot_count)                           \
    X(main_sprites_coexists_with_register_fonts)               \
    X(game_sprites_inline_block_field_map)                     \
    X(game_sprites_trailing_call_shapes)                       \
    X(game_sprites_total_slot_count)                           \
    X(game_sprites_resource_ids_unique)                        \
    X(game_sprites_untouched_indices_stay_zero)                \
    X(game_sprites_coexists_with_main_sprites)                 \
    X(game_sounds_total_entry_count)                           \
    X(game_sounds_index_range_and_gaps)                        \
    X(game_sounds_field_writes_spot_check)                     \
    X(game_sounds_resource_ids_unique)                         \
    X(game_sounds_coexists_with_main_sounds)                   \
    X(game_sounds_buffer_pointer_preserved)                    \
    X(aux_sounds_writes_four_entries)                          \
    X(aux_sounds_fills_game_sounds_gap)                        \
    X(aux_sounds_buffer_pointer_preserved)                     \
    X(locale_sounds_no_locale_uses_fallback_or_settings)       \
    X(locale_sounds_skip_when_primary_id_zero)                 \
    X(locale_sounds_launcher_flag_forces_fallback)             \
    X(locale_sounds_override_path_uses_current_locale)         \
    X(locale_sounds_override_7fff_skips_under_locale)          \
    X(locale_sounds_coexists_with_game_sounds)                 \
    X(locale_sounds_buffer_pointer_preserved)                  \
    X(group3_sprites_writes_233_distinct_slots)                \
    X(group3_sprites_group_tag_stamped)                        \
    X(group3_sprites_zdd_and_settings_uniform)                 \
    X(group3_sprites_spotcheck_first_entry)                    \
    X(group3_sprites_spotcheck_max_idx)                        \
    X(group3_sprites_spotcheck_colorkey_entry)                 \
    X(group3_sprites_no_overlap_with_main_sprite_indices)      \
    X(group3_info_events_first_event_flag_set)                 \
    X(group3_info_events_marker_and_flag_pair)                 \
    X(group3_info_events_data_ptr_set)                         \
    X(group3_info_events_marker_set_with_high_value)           \
    X(group3_info_events_copy_marker_and_flag_chain)           \
    X(group3_info_events_struct_copy_from_zero_init)           \
    X(group3_info_events_dst_indices_in_92_to_437_range)       \
    X(group3_info_events_fires_from_register_group3_sprites)   \
    X(pool_get_slot_sentinel_returns_null)                     \
    X(pool_get_slot_ramp_range)                                \
    X(pool_get_slot_main_range)                                \
    X(ss_mgr_clone_copies_slot_metadata)                       \
    X(ss_mgr_clone_copies_info_marker_and_flag)                \
    X(ss_mgr_clone_clears_dst_info_data_and_palette)           \
    X(ss_mgr_clone_destroys_old_dst_entries)                   \
    X(npc_palette_remap_lookup)                                \
    X(reapply_data_events_restores_after_clone)                \
    X(cross_slot_palette_swap_tables)                          \
    X(apply_slot_palette_swap_noop_paths)                      \
    X(group3_clones_apply_is_idempotent)                       \
    X(group3_clones_dst_pool_range)                            \
    X(group3_clones_first_entry_propagates_resource_id)        \
    X(group3_clones_fires_from_register_group3_sprites)        \
    X(inline_clones_targets_populated_after_register)          \
    X(inline_clones_propagate_source_metadata)                 \
    X(inline_clones_errands_floor_tilesets)                    \
    X(inline_clones_late_cluster_shares_one_source)            \
    X(inline_clones_apply_is_idempotent)                       \
    X(inline_clones_src_dst_sets_disjoint)                     \
    X(boot_register_all_group_tags_per_batch)                  \
    X(boot_register_all_zdd_vs_zds_routing)                    \
    X(boot_register_all_sotesp_module_for_special_slots)       \
    X(boot_register_all_locale_state_routed)                   \
    X(boot_register_all_null_locale_skips_tail)                \
    X(boot_register_all_touches_every_batch_signature_slot)    \
    X(ar_layout_matches_retail)                                \
    X(ar_sprite_frame_null_slot_and_entries_return_null)       \
    X(ar_sprite_frame_loaded_indexes_without_hook)             \
    X(ar_sprite_frame_lazy_decode_fires_once_then_indexes)     \
    X(ar_sprite_frame_unloaded_no_hook_returns_null)           \
    X(sheet_decode_pixels_brightness_and_key)                  \
    X(sheet_decode_pixels_truncates_toward_zero)               \
    X(sheet_decode_pixels_lut_then_scale)                      \
    X(sheet_decode_pixels_non24_is_noop)                       \
    X(sheet_decode_pixels_null_pixels_is_noop)                 \
    X(sprite_slice_geometry_and_order)                         \
    X(sprite_slice_default_cell_is_whole_sheet)                \
    X(sprite_slice_count_zero_when_cell_exceeds_sheet)         \
    X(sprite_slice_colorkey_sentinel_passthrough)              \
    X(sprite_slice_null_surface_marks_failure)                 \
    X(sprite_decode_cleanup_then_bs_failure)                   \
    X(sprite_decode_end_to_end_24bpp)                          \
    X(bs_release_no_free_nulls_pixels)                         \
    X(bs_release_is_idempotent_on_null)                        \
    X(bs_release_frees_pixels)                                 \
    X(bs_get_set_bit_count_recomputes_stride)                  \
    X(bs_init_bitmap_stamps_BIH)                               \
    X(bs_init_bitmap_24bpp_size)                               \
    X(bs_emit_palette_bgra_swaps_RGB_to_BGR)                   \
    X(bs_install_palette_swaps_RGB_to_BGR)                     \
    X(bs_install_palette_roundtrips_emit)                      \
    X(decode_returns_zero_on_missing_resource)                 \
    X(decode_raw_8bpp_copies_palette_and_pixels)               \
    X(decode_raw_24bpp_skips_palette_copy)                     \
    X(decode_raw_unsupported_depth_releases_pixels)            \
    X(compressed_header_signature_mismatch_returns_zero)       \
    X(compressed_header_pixel_offset_too_large)                \
    X(compressed_header_happy_path)                            \
    X(decode_compressed_path_8bpp)                             \
    X(decode_compressed_signature_mismatch_returns_zero)       \
    X(palette_session_begin_emits_BGRA_on_8bpp)                \
    X(palette_session_begin_returns_false_on_missing_resource) \
    X(palette_session_begin_returns_false_on_24bpp)            \
    X(main_sprites_installs_palette_when_resource_8bpp)        \
    X(main_sprites_skips_palette_when_resource_missing)        \
    X(palette_ramps_installs_palettes_on_all_12_slots)         \
    X(palette_ramps_ramp0_three_color_overrides)               \
    X(palette_ramps_ramp4_uses_blue_bg)                        \
    X(palette_ramps_skip_install_when_resource_missing)        \
    X(palette_ramps_ramp_slot_field_writes)                    \
    X(palette_ramps_extras_use_caller_settings)                \
    X(palette_ramps_portrait_flags_written_per_register)       \
    X(palette_ramps_portrait_slot_field_writes)                \
    X(palette_ramps_coexist_with_main_pool_unwritten)          \
    X(trim_24bpp_loose_ybottom_quirk)                          \
    X(trim_8bpp_tight_ybottom)                                 \
    X(trim_all_key_no_opaque)                                  \
    X(trim_all_opaque_no_key)                                  \
    X(trim_other_depth_full_rect)                              \
    X(trim_subwindow_8bpp)                                     \
    X(trim_8bpp_nonsquare_quirk69)                             \
    X(convert_24bpp_to_16bpp_rgb565)                           \
    X(convert_8bpp_to_16bpp_palette_and_key)                   \
    X(convert_to_16bpp_wrong_depth_noop)                       \
    X(convert_8bpp_to_24bpp_expands)                           \
    X(convert_24bpp_to_32bpp_widens)                           \
    X(convert_24bpp_to_32bpp_wrong_depth_noop)                 \
    X(load_palette_from_reverses_channels)                     \
    X(bitmap_session_layout_matches_retail)                    \
    X(input_mgr_ring_offsets)                                  \
    X(input_poll_hit_consumes)                                 \
    X(input_poll_wrong_id_misses)                              \
    X(input_poll_flag_must_be_one)                             \
    X(input_poll_age_boundary)                                 \
    X(input_poll_future_ts_rejected)                           \
    X(input_poll_rollover_within_window)                       \
    X(input_poll_highest_index_wins)                           \
    X(input_poll_endpoints_in_range)                           \
    X(input_poll_empty_ring_misses)                            \
    X(input_any_fresh_press_matches_any_id)                    \
    X(input_any_fresh_press_empty_misses)                      \
    X(input_any_fresh_press_flag_and_age)                      \
    X(input_mgr_reset_flushes)                                 \
    X(input_dtap_two_presses_hit)                              \
    X(input_dtap_single_press_misses)                          \
    X(input_dtap_out_of_window_misses)                         \
    X(input_dtap_window_boundary_inclusive)                    \
    X(input_dtap_flag_must_be_pressed)                         \
    X(input_dtap_other_dir_ignored)                            \
    X(input_ring_find_recent_hit)                              \
    X(input_ring_find_recent_window)                           \
    X(input_ring_find_recent_rejects)                          \
    X(render_id_register_and_lookup)                           \
    X(render_id_overwrite)                                     \
    X(render_id_forget_and_recycle)                            \
    X(render_id_forget_probes_past_tombstone)                  \
    X(render_id_sheet_hash_table)                              \
    X(render_id_fnv1a_known_vector)                            \
    X(render_id_fnv1a_sensitive_and_seeded)                    \
    X(glyph_token_search_found)                                \
    X(glyph_token_search_absent)                               \
    X(glyph_token_search_empty_needle)                         \
    X(glyph_token_search_start_offset)                         \
    X(glyph_token_search_skips_sjis_trail)                     \
    X(map_data_header)                                         \
    X(map_data_cells_copied)                                   \
    X(map_data_layers)                                         \
    X(map_data_truncated)                                      \
    X(map_data_cell_fields)                                    \
    X(map_data_cell_bounds)                                    \
    X(map_grid_set_dims)                                       \
    X(map_grid_clear_cell)                                     \
    X(map_grid_emit_obj_block)                                 \
    X(map_grid_emit_obj_clamps_dims)                           \
    X(map_grid_emit_tile_explicit_span)                        \
    X(map_grid_emit_tile_bank_span)                            \
    X(char_physics_walk_ramp)                                  \
    X(char_physics_jump_windup)                                \
    X(char_ai_ring_find_window_flag_consume)                   \
    X(char_ai_ring_double_needs_two_records)                   \
    X(char_ai_combo_chain)                                     \
    X(char_ai_cmd_and_slots)                                   \
    X(entity_move_h_blocked_by_solid)                          \
    X(entity_move_h_platform_passes)                           \
    X(entity_move_h_probe_writes_nothing)                      \
    X(entity_move_v_lands_on_platform)                         \
    X(entity_move_v_pass_skips_platform)                       \
    X(entity_move_v_head_bonk)                                 \
    X(entity_move_faction_relation)                            \
    X(collision_drop_clamps_on_floor)                          \
    X(collision_open_air_clears)                               \
    X(collision_wall_off_axis_no_block)                        \
    X(collision_ceiling_blocks_upward)                         \
    X(collision_zero_delta_noop)                               \
    X(collision_slope_callback_invoked)                        \
    X(collision_hwalk_wall_stops_flush)                        \
    X(collision_hwalk_open_clears)                             \
    X(collision_hwalk_zero_delta_noop)                         \
    X(collision_hwalk_climbs_ramp)                             \
    X(collision_hwalk_descends_ramp)                           \
    X(collision_hwalk_margin_skips_head_row)                   \
    X(map_decode_1b58b_shape0)                                 \
    X(map_decode_1b58b_shape2)                                 \
    X(map_decode_1b58d_shape2_blend)                           \
    X(map_decode_1b58f_shape0_base_only)                       \
    X(map_decode_1b58f_shape12_fg)                             \
    X(map_decode_29ff4_shape14_z1)                             \
    X(map_decode_single_tile_arms)                             \
    X(map_decode_1b5ab)                                        \
    X(map_decode_cfg_init)                                     \
    X(map_decode_house_arms)                                   \
    X(map_decode_block_arms)                                   \
    X(map_decode_errands_arms)                                 \
    X(map_decode_full_small)                                   \
    X(map_decode_empty_cell_noop)                              \
    X(map_decode_simple_arm_2af8)                              \
    X(map_decode_2b2a_shape_flag)                              \
    X(map_decode_1d8aa_blend_pair)                             \
    X(map_decode_22b16_shape3_span)                            \
    X(map_decode_1bd82_stair)                                  \
    X(map_decode_999xxx_markers)                               \
    X(map_decode_placeholder_pass)                             \
    X(map_render_camera_init)                                  \
    X(map_render_camera_town_first_frame)                      \
    X(map_render_window_basic)                                 \
    X(map_render_window_clamps_origin)                         \
    X(map_render_window_no_cap_when_near_edge)                 \
    X(map_render_window_row_sum_components)                    \
    X(map_render_grid_index)                                   \
    X(map_render_tile_basic)                                   \
    X(map_render_tile_empty_cell)                              \
    X(map_render_tile_footprint_suboffset)                     \
    X(map_render_walk_basic)                                   \
    X(map_render_walk_resolver_gate)                           \
    X(map_render_walk_window_clip)                             \
    X(draw_pool_node_size)                                     \
    X(draw_pool_caps_table)                                    \
    X(draw_pool_emit_fields)                                   \
    X(draw_pool_emit_layer_mask)                               \
    X(draw_pool_emit_overflow)                                 \
    X(draw_pool_layer0_always_full)                            \
    X(draw_pool_reset)                                         \
    X(draw_pool_emit_actor_opaque)                             \
    X(draw_pool_emit_actor_alpha)                              \
    X(draw_pool_emit_actor_null_cel)                           \
    X(camera_follow_pan_trajectory)                            \
    X(camera_follow_vel_caps_at_speed)                         \
    X(camera_follow_lands_exact_no_overshoot)                  \
    X(camera_follow_far_boost_only_when_flagged)               \
    X(camera_shake_inactive_is_zero)                           \
    X(camera_follow_step_both_axes)                            \
    X(camera_apply_snap_jumps_and_clamps)                      \
    X(camera_apply_pan_sets_target_keeps_cur)                  \
    X(anim_clip_loop_trajectory)                               \
    X(anim_clip_oneshot_holds_last)                            \
    X(anim_clip_duration_gate)                                 \
    X(anim_clip_null_is_noop)                                  \
    X(anim_clip_set_is_change_gated)                           \
    X(anim_clip_sprite_id)                                     \
    X(actor_describe_static)                                   \
    X(actor_describe_skips)                                    \
    X(actor_describe_mirrored)                                 \
    X(actor_describe_animated)                                 \
    X(actor_describe_clip_terminator)                          \
    X(actor_describe_clip_link_overrides_dir)                  \
    X(actor_describe_flag_abs)                                 \
    X(actor_describe_angle)                                    \
    X(actor_static_emits_node)                                 \
    X(actor_static_skip_flag)                                  \
    X(actor_static_layer_override)                             \
    X(actor_static_describe_fail)                              \
    X(actor_protagonist_three_cels)                            \
    X(actor_protagonist_body_is_describe)                      \
    X(actor_protagonist_skip_and_gate)                         \
    X(actor_anim_advance_matches_stepper)                      \
    X(actor_anim_advance_null_is_noop)                         \
    X(actor_spawn_census)                                      \
    X(actor_spawn_sprite_lookup)                               \
    X(actor_spawn_invisible_self_skips)                        \
    X(actor_spawn_protagonist)                                 \
    X(actor_spawn_prop_renders)                                \
    X(actor_spawn_guards)                                      \
    X(actor_pool_update_trots)                                 \
    X(actor_spawn_struct)                                      \
    X(actor_spawn_effect)                                      \
    X(actor_spawn_cutscene_iris)                               \
    X(arche_runoff)                                            \
    X(arche_house_turn_clip)                                   \
    X(arche_pose_clip)                                         \
    X(arche_sword_clip)                                        \
    X(errands_cast_zorder)                                     \
    X(errands_fire)                                            \
    X(errands_clock_pot_mapdriven)                             \
    X(butterfly_pertick)                                       \
    X(butterfly_motion)                                        \
    X(butterfly_flutter)                                       \
    X(party_dramatist_find)                                    \
    X(party_archetype_default_bank)                            \
    X(party_resolve_arrival_cast)                              \
    X(party_stat_maxes)                                        \
    X(party_stat_ratio_and_display)                            \
    X(party_stat_level)                                        \
    X(party_leader_gate)                                       \
    X(party_base_stat_find)                                    \
    X(party_stats_init_arche)                                  \
    X(party_init_errands)                                      \
    X(crt_itoa_radix_and_sign)                                \
    X(crt_gmtime_epoch_leap_and_year_end)                     \
    X(crt_localtime_zone_and_usa_dst)                         \
    X(crt_loctotime_inverts_localtime)                        \
    X(crt_strftime_save_stamp_and_names)                      \
    X(crt_sprintf_formats)                                    \

#define X(n) extern int test_##n(void);
TESTS
#undef X

static const struct test_case TESTS_TABLE[] = {
#define X(n) { #n, test_##n },
    TESTS
#undef X
};
static const size_t N_TESTS = sizeof(TESTS_TABLE) / sizeof(TESTS_TABLE[0]);

int main(int argc, char **argv)
{
    const char *filter = (argc >= 2) ? argv[1] : NULL;
    int pass = 0, fail = 0, skip = 0;

    for (size_t i = 0; i < N_TESTS; i++) {
        const struct test_case *t = &TESTS_TABLE[i];
        if (filter && !strstr(t->name, filter)) continue;
        fprintf(stderr, "[%zu/%zu] %s\n", i + 1, N_TESTS, t->name);
        int rc = t->fn();
        if      (rc == 0) pass++;
        else if (rc == 2) skip++;
        else              fail++;
    }

    fprintf(stderr, "\nresult: %d pass, %d fail, %d skip (of %zu)\n",
            pass, fail, skip, N_TESTS);
    return fail == 0 ? 0 : 1;
}
