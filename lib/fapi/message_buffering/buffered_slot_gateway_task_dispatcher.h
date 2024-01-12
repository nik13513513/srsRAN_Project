/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "buffered_slot_gateway_impl.h"
#include "srsran/fapi/slot_time_message_notifier.h"

namespace srsran {

class task_executor;

namespace fapi {

/// Buffered slot gateway task dispatcher.
class buffered_slot_gateway_task_dispatcher : public slot_message_gateway, public slot_time_message_notifier
{
public:
  buffered_slot_gateway_task_dispatcher(unsigned              l2_nof_slots_ahead,
                                        subcarrier_spacing    scs_,
                                        slot_message_gateway& gateway,
                                        task_executor&        executor_);

  // See interface for documentation.
  void on_slot_indication(const slot_indication_message& msg) override;

  // See interface for documentation.
  void dl_tti_request(const dl_tti_request_message& msg) override;

  // See interface for documentation.
  void ul_tti_request(const ul_tti_request_message& msg) override;

  // See interface for documentation.
  void ul_dci_request(const ul_dci_request_message& msg) override;

  // See interface for documentation.
  void tx_data_request(const tx_data_request_message& msg) override;

private:
  const subcarrier_spacing   scs;
  srslog::basic_logger&      logger;
  task_executor&             executor;
  buffered_slot_gateway_impl buffered_gateway;
};

} // namespace fapi
} // namespace srsran
