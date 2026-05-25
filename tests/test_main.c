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
    X(pd_channel_mask_to_shift_rgb565_R)                       \
    X(pd_channel_mask_to_shift_rgb565_G)                       \
    X(pd_channel_mask_to_shift_rgb565_B)                       \
    X(pd_channel_mask_to_shift_rgb555)                         \
    X(pd_channel_mask_to_shift_zero_mask)                      \
    X(pd_channel_mask_to_shift_single_bit_at_0)                \
    X(pd_channel_init_zeroes_and_seeds_weight)                 \
    X(pd_channel_free_lut_when_allocated)                      \
    X(pd_channel_free_lut_when_not_allocated)                  \
    X(pd_blend_init_zeroes_state_and_inits_channels)           \
    X(pd_blend_set_color_writes_per_channel_weights)           \
    X(pd_blend_set_color_does_not_touch_other_fields)          \
    X(pd_blend_layout_matches_retail_offsets)                  \
    X(pd_lut_shared_short_circuit)                             \
    X(pd_lut_different_weight_allocates_fresh)                 \
    X(pd_lut_invalid_state_is_noop)                            \
    X(pd_lut_small_identity_weight_1000)                       \
    X(pd_lut_small_half_weight_500)                            \
    X(pd_lut_small_zero_weight)                                \
    X(pd_lut_small_invert)                                     \
    X(pd_lut_large_mode1_add)                                  \
    X(pd_lut_large_mode2_sub)                                  \
    X(pd_lut_large_default_mode)                               \
    X(pd_format_get_masks_reads_struct)                        \
    X(pd_commit_null_fmt_uses_rgb565_defaults)                 \
    X(pd_commit_custom_format)                                 \
    X(pd_commit_flag_forces_state_2)                           \
    X(pd_commit_nondefault_weight_state_1)                     \
    X(pd_commit_nondefault_mode_state_1)                       \
    X(pd_commit_idempotent_frees_old_luts)                     \
    X(pd_commit_b_uses_r_as_prev_not_g)                        \
    X(pd_boot_group_a_weight_ramp_and_mode)                    \
    X(pd_boot_group_b_weight_ramp_div_22)                      \
    X(pd_boot_group_c_grey_ramp)                               \
    X(pd_boot_group_d_special_colours)                         \
    X(pd_boot_group_e_weight_ramp_and_mode)                    \
    X(pd_boot_all_slots_committed_against_fmt)                 \
    X(pd_boot_custom_fmt_propagates)                           \
    X(pd_boot_init_is_idempotent)                              \
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
    X(group3_clones_apply_is_idempotent)                       \
    X(group3_clones_dst_pool_range)                            \
    X(group3_clones_first_entry_propagates_resource_id)        \
    X(group3_clones_fires_from_register_group3_sprites)        \
    X(inline_clones_targets_populated_after_register)          \
    X(inline_clones_propagate_source_metadata)                 \
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
    X(bs_release_no_free_nulls_pixels)                         \
    X(bs_release_is_idempotent_on_null)                        \
    X(bs_release_frees_pixels)                                 \
    X(bs_get_set_bit_count_recomputes_stride)                  \
    X(bs_init_bitmap_stamps_BIH)                               \
    X(bs_init_bitmap_24bpp_size)                               \
    X(bs_emit_palette_bgra_swaps_RGB_to_BGR)                   \
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
    X(bitmap_session_layout_matches_retail)                    \
    X(wp_harmless_messages_consumed)                           \
    X(wp_close_calls_exit_zero)                                \
    X(wp_default_forwards_to_defwindowproc)                    \
    X(wp_paint_no_ctx_falls_through)                           \
    X(wp_paint_ctx_but_no_paint_this_falls_through)            \
    X(wp_paint_helper_returns_zero_falls_through)              \
    X(wp_paint_consumed_no_defwindowproc)                      \
    X(wp_activateapp_writes_flag_without_ctx)                  \
    X(wp_activateapp_loaded_zero_skips_body)                   \
    X(wp_activateapp_deactivate_calls_pause)                   \
    X(wp_activateapp_activate_minimal_emits_cp3_and_post)      \
    X(wp_activateapp_full_chain_call_order)                    \
    X(wp_activateapp_quiet_suppresses_logs_only)               \
    X(wp_activateapp_extra_null_sparse_loop)                   \
    X(wp_activateapp_device_chain_sets_zdm_active)             \
    X(wp_activateapp_device_chain_partial_zdm_inactive)        \
    X(wp_activateapp_null_zdm_skips_zdm_call)                  \
    X(wp_timer_clears_ctx_timer_field)                         \
    X(wp_timer_no_ctx_consumed)                                \
    X(wp_state_init_clears_all_globals)                        \
    X(wp_app_ctx_layout_matches_retail)                        \
    X(zdd_layout_matches_retail_offsets)                       \
    X(zdd_ctor_zeros_struct)                                   \
    X(zdd_dderr_name_known_entries)                            \
    X(zdd_dderr_name_unknown_returns_null)                     \
    X(zdd_log_dderr_format_known_hresult)                      \
    X(zdd_log_dderr_format_empty_prefix1)                      \
    X(zdd_log_dderr_format_unknown_hresult)                    \
    X(zdd_log_dderr_format_null_prefixes_tolerated)            \
    X(zdd_restore_cursor_noop_when_shown)                      \
    X(zdd_restore_cursor_shows_when_hidden)                    \
    X(zdd_release_children_in_order_and_zeros_fields)          \
    X(zdd_release_children_skips_nulls)                        \
    X(zdd_dtor_releases_everything_and_flushes_log)            \
    X(zdd_dtor_quiet_when_clean)                               \
    X(zdd_dtor_open_objects_warns_only)                        \
    X(zdd_dtor_flushes_log_buffer_if_nonempty)                 \
    X(zdd_create_success_path)                                 \
    X(zdd_create_failure_path_tears_down)                      \
    X(zdd_object_layout_matches_retail_offsets)                \
    X(zdd_object_ctor_sets_parent_and_bumps_open_objects)      \
    X(zdd_object_release_pixel_buf_frees_and_clears)           \
    X(zdd_object_release_pixel_buf_noop_when_null)             \
    X(zdd_object_dtor_releases_in_retail_order)                \
    X(zdd_object_dtor_clean_object_noop)                       \
    X(zdd_obj_destroy_walks_dtor_and_frees)                    \
    X(zdd_obj_destroy_idempotent_on_null)                      \
    X(zdd_build_desc_minimal_no_pixelformat)                   \
    X(zdd_build_desc_caps_base_preserved)                      \
    X(zdd_build_desc_force_videomem_adds_caps)                 \
    X(zdd_build_desc_self_videomem_flag_adds_caps)             \
    X(zdd_build_desc_pixelformat_mode_2_8bpp)                  \
    X(zdd_build_desc_pixelformat_mode_2_16bpp_rgb565)          \
    X(zdd_build_desc_pixelformat_mode_2_24bpp)                 \
    X(zdd_build_desc_pixelformat_mode_2_32bpp)                 \
    X(zdd_build_desc_pixelformat_mode_2_unknown_bpp)           \
    X(zdd_build_desc_mode_other_no_pixelformat_even_with_bpp)  \

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
