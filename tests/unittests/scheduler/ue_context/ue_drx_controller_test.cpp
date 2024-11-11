/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "lib/scheduler/ue_context/ue_drx_controller.h"
#include "lib/scheduler/ue_context/ul_logical_channel_manager.h"
#include "srsran/support/test_utils.h"
#include <gtest/gtest.h>

using namespace srsran;

using msec = std::chrono::milliseconds;

class base_ue_drx_controller_test
{
protected:
  base_ue_drx_controller_test(const std::optional<drx_config>& drx_cfg_) : drx_cfg(drx_cfg_) {}

  void tick()
  {
    drx.slot_indication(next_slot);
    cur_slot = next_slot;
    ++next_slot;
  }

  const subcarrier_spacing        scs          = subcarrier_spacing::kHz30;
  const std::chrono::milliseconds conres_timer = msec{64};
  std::optional<drx_config>       drx_cfg;
  ul_logical_channel_manager      ul_lc_ch_mng;
  slot_point                      ul_ccch_slot{to_numerology_value(scs), 0};
  srslog::basic_logger&           logger = srslog::fetch_basic_logger("SCHED");
  ue_drx_controller               drx{scs, conres_timer, drx_cfg, ul_lc_ch_mng, ul_ccch_slot, logger};

  slot_point next_slot{to_numerology_value(scs),
                       test_rgen::uniform_int<unsigned>(0, 10240) * get_nof_slots_per_subframe(scs) - 1};
  slot_point cur_slot;
};

class drx_disabled_ue_drx_controller_test : public base_ue_drx_controller_test, public testing::Test
{
protected:
  drx_disabled_ue_drx_controller_test() : base_ue_drx_controller_test(std::nullopt) {}
};

TEST_F(drx_disabled_ue_drx_controller_test, when_no_drx_config_provided_pdcch_is_always_available)
{
  const unsigned NOF_TESTS = 16;
  for (unsigned i = 0; i != NOF_TESTS; ++i) {
    tick();
    ASSERT_TRUE(drx.is_pdcch_enabled(cur_slot));
  }
}

class ue_drx_controller_test : public base_ue_drx_controller_test, public testing::Test
{
protected:
  ue_drx_controller_test() : base_ue_drx_controller_test(drx_config{msec{80}, msec{10}, msec{20}, msec{10}}) {}

  const unsigned period_slots     = drx_cfg->long_cycle.count() * get_nof_slots_per_subframe(scs);
  const unsigned offset_slot      = drx_cfg->long_start_offset.count() * get_nof_slots_per_subframe(scs);
  const unsigned on_dur_slots     = drx_cfg->on_duration_timer.count() * get_nof_slots_per_subframe(scs);
  const unsigned inactivity_slots = drx_cfg->inactivity_timer.count() * get_nof_slots_per_subframe(scs);
};

TEST_F(ue_drx_controller_test, when_drx_config_provided_slot_offset_and_on_duration_are_respected)
{
  for (unsigned i = 0; i != period_slots; ++i) {
    tick();

    const unsigned slot_mod         = cur_slot.to_uint() % period_slots;
    bool           enabled          = drx.is_pdcch_enabled(cur_slot);
    bool           in_active_window = slot_mod >= offset_slot and slot_mod < (offset_slot + on_dur_slots);

    ASSERT_EQ(enabled, in_active_window);
  }
}

TEST_F(ue_drx_controller_test, when_pdcch_sent_then_on_duration_extended_by_inactivity_timer)
{
  next_slot = slot_point{to_numerology_value(scs), 0};

  const unsigned pdcch_idx  = test_rgen::uniform_int<unsigned>(offset_slot + 1, offset_slot + on_dur_slots - 1);
  const unsigned active_end = std::max(pdcch_idx + inactivity_slots, offset_slot + on_dur_slots);

  for (unsigned i = 0; i != period_slots; ++i) {
    tick();

    if (i == pdcch_idx) {
      drx.on_new_pdcch_alloc(cur_slot);
    }

    bool enabled = drx.is_pdcch_enabled(cur_slot);
    if (i < offset_slot) {
      ASSERT_FALSE(enabled);
    } else if (i < active_end) {
      ASSERT_TRUE(enabled) << fmt::format(
          "Failed active window detection at slot_mod={}, window=[{},{})", i, offset_slot, active_end);
    } else {
      ASSERT_FALSE(enabled) << fmt::format(
          "Failed inactive window detection at slot_mod={}, window=[{},{})", i, offset_slot, active_end);
    }
  }
}

TEST_F(ue_drx_controller_test, when_sr_is_pending_then_drx_is_in_active_time)
{
  ul_lc_ch_mng.handle_sr_indication();

  for (unsigned i = 0; i != period_slots; ++i) {
    tick();

    ASSERT_TRUE(drx.is_pdcch_enabled(cur_slot));
  }

  ul_lc_ch_mng.reset_sr_indication();

  for (unsigned i = 0; i != period_slots; ++i) {
    tick();

    const unsigned slot_mod         = cur_slot.to_uint() % period_slots;
    bool           enabled          = drx.is_pdcch_enabled(cur_slot);
    bool           in_active_window = slot_mod >= offset_slot and slot_mod < (offset_slot + on_dur_slots);

    ASSERT_EQ(enabled, in_active_window);
  }
}

TEST_F(ue_drx_controller_test, when_conres_timer_is_running_then_drx_is_active)
{
  next_slot = slot_point{to_numerology_value(scs), 0};

  const unsigned con_res_timer_slots = conres_timer.count() * get_nof_slots_per_subframe(scs);

  drx.on_con_res_start();

  for (unsigned i = 0; i != con_res_timer_slots; ++i) {
    tick();
    ASSERT_TRUE(drx.is_pdcch_enabled(cur_slot)) << "DRX should be active while ra-ConResTimer is running";
  }

  for (unsigned i = 0; i != period_slots; ++i) {
    tick();

    const unsigned slot_mod         = cur_slot.to_uint() % period_slots;
    bool           enabled          = drx.is_pdcch_enabled(cur_slot);
    bool           in_active_window = slot_mod >= offset_slot and slot_mod < (offset_slot + on_dur_slots);

    ASSERT_EQ(enabled, in_active_window);
  }
}
