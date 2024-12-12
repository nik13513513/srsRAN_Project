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

#include "srsran/fapi/slot_time_message_notifier.h"

namespace srsran {
namespace fapi {

/// Dummy FAPI slot time message notifier that will close the application if its methods are called.
class slot_time_message_notifier_dummy : public slot_time_message_notifier
{
public:
  // See interface for documentation.
  void on_slot_indication(const slot_indication_message& msg) override;
};

} // namespace fapi
} // namespace srsran
