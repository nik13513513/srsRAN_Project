/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ru_controller_generic_impl.h"
#include "srsran/phy/lower/lower_phy_controller.h"
#include "srsran/radio/radio_session.h"
#include "srsran/support/math/math_utils.h"

using namespace srsran;

ru_controller_generic_impl::ru_controller_generic_impl(std::vector<lower_phy_controller*> low_phy_crtl_,
                                                       std::vector<phy_metrics_adapter*>  low_phy_metrics_,
                                                       radio_session&                     radio_,
                                                       double                             srate_MHz_) :
  low_phy_crtl(std::move(low_phy_crtl_)),
  low_phy_metrics(std::move(low_phy_metrics_)),
  radio(radio_),
  srate_MHz(srate_MHz_)
{
}

void ru_controller_generic_impl::start()
{
  // Calculate starting time from the radio current time plus one hundred milliseconds.
  double                     delay_s      = 0.1;
  baseband_gateway_timestamp current_time = radio.read_current_time();
  baseband_gateway_timestamp start_time   = current_time + static_cast<uint64_t>(delay_s * srate_MHz * 1e6);

  // Round start time to the next subframe.
  uint64_t sf_duration = static_cast<uint64_t>(srate_MHz * 1e3);
  start_time           = divide_ceil(start_time, sf_duration) * sf_duration;

  radio.start(start_time);

  for (auto& low_phy : low_phy_crtl) {
    low_phy->start(start_time);
  }
}

void ru_controller_generic_impl::stop()
{
  radio.stop();

  for (auto& low_phy : low_phy_crtl) {
    low_phy->stop();
  }
}

bool ru_controller_generic_impl::set_tx_gain(unsigned port_id, double gain_dB)
{
  return radio.get_management_plane().set_tx_gain(port_id, gain_dB);
}

bool ru_controller_generic_impl::set_rx_gain(unsigned port_id, double gain_dB)
{
  return radio.get_management_plane().set_rx_gain(port_id, gain_dB);
}

void ru_controller_generic_impl::print_metrics()
{
  phy_metrics_adapter::print_header();
  for (auto metrics : low_phy_metrics) {
    metrics->print_metrics();
  }
}
