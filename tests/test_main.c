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
    X(ar_layout_matches_retail)                                \
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
