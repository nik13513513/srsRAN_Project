/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "srsran/ran/band_helper.h"
#include "srsran/ran/bs_channel_bandwidth.h"
#include "srsran/ran/pci.h"
#include "srsran/ran/ssb_properties.h"

namespace srsran {

/// \brief Main cell parameters from which other cell parameters (e.g. coreset0, BWP RBs) will be derived.
/// \remark Only fields that may affect many different fields in du_cell_config (e.g. number of PRBs) should be added
/// in this struct.
struct cell_config_builder_params {
  /// Physical Cell Identity.
  pci_t pci = 1;
  /// subCarrierSpacingCommon, as per \c MIB, TS 38.331.
  subcarrier_spacing scs_common = subcarrier_spacing::kHz15;
  /// BS Channel Bandwidth, as per TS 38.104, Section 5.3.1.
  bs_channel_bandwidth_fr1 channel_bw_mhz = bs_channel_bandwidth_fr1::MHz10;
  /// This ARFCN represents "f_ref" for DL, as per TS 38.211, Section 5.4.2.1.
  unsigned dl_arfcn = 365000;
  /// <em>NR operating band<\em>, as per Table 5.2-1 and 5.2-2, TS 38.104. If not specified, a valid band for the
  /// provided DL ARFCN is automatically derived.
  optional<nr_band> band;
  /// offsetToPointA, as per TS 38.211, Section 4.4.4.2; \ref ssb_offset_to_pointA.
  ssb_offset_to_pointA offset_to_point_a{18};
  /// This is \c controlResourceSetZero, as per TS38.213, Section 13.
  unsigned coreset0_index = 9;
  /// This is \c searchSpaceZero, as per TS38.213, Section 13.
  unsigned search_space0_index = 0;
  /// k_ssb or SSB SubcarrierOffest, as per TS38.211 Section 7.4.3.1. Possible values: {0, ..., 23}.
  ssb_subcarrier_offset k_ssb = 6;
  /// Whether to enable CSI-RS in the cell.
  bool csi_rs_enabled = true;
  /// Number of DL ports for the cell.
  unsigned nof_dl_ports = 1;
  /// Whether fallback DCI format is being used in SearchSpace#2.
  bool fallback_dci_format_in_ss2 = false;
};

} // namespace srsran
