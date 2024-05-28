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

#include "srsran/ran/rrm.h"
#include "srsran/ran/s_nssai.h"

namespace srsran {

/// Cell-specific Default RAN slice configuration.
struct cell_rrm_policy_config {
  /// RRM Policy identifier.
  rrm_policy_member rrc_member;
  /// Sets the minimum percentage of PRBs to be allocated to this group.
  unsigned min_prb_ratio = 0;
  /// Sets the maximum percentage of PRBs to be allocated to this group.
  unsigned max_prb_ratio = MAX_NOF_PRBS;
};

} // namespace srsran