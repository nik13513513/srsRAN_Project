/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "../cell/resource_grid.h"
#include "../ue_scheduling/ue.h"
#include "../ue_scheduling/ue_configuration.h"
#include "srsgnb/scheduler/scheduler_slot_handler.h"

namespace srsgnb {

/// Contains the output of the PUCCH allocator for HARQ-ACK grant.
struct pucch_harq_ack_grant {
  /// \c pucch_res_indicator, or \f$\Delta_{PRI}\f$, is the <em>PUCCH resource indicator<\em> field for DCI 1_0 and 1_1
  /// as per TS 38.213, Section 9.2.1. It indicates the UE which PUCCH resource should be used for HACK-(N)ACK
  /// reporting.
  /// \remark This is valid only if \c pucch is NOT nullptr.
  unsigned pucch_res_indicator{0};
  /// Pointer of the allocated PUCCH PDU; if \c nullptr, the allocation wasn't successful.
  pucch_info* pucch_pdu{nullptr};
};

/// Contains the number of UCI HARQ-ACK and CSI information bits of a removed PUCCH grant.
struct pucch_uci_bits {
  /// Number of HARQ-ACK info bits that should have been reported in the removed PUCCH grant.
  unsigned harq_ack_nof_bits{0};
  /// Number of CSI-part1 info bits that should have been reported in the removed PUCCH grant.
  unsigned csi_part1_bits{0};
  // TODO: add extra bits for CSI part 2.
};

/// PUCCH scheduling interface.
class pucch_allocator
{
public:
  virtual ~pucch_allocator() = default;

  /// Allocate the common PUCCH resource for HARQ-ACK for a given UE.
  /// \param[out,in] slot_alloc struct with scheduling results.
  /// \param[in] tcrnti temporary RNTI of the UE.
  /// \param[in] pdsch_time_domain_resource k0 value, or delay (in slots) of PDSCH slot vs the corresponding PDCCH slot.
  /// \param[in] k1 delay in slots of the UE's PUCCH HARQ-ACK report with respect to the PDSCH.
  /// \param[in] dci_info information with DL DCI, needed for HARQ-(N)-ACK scheduling info.
  /// \return The grant for the UE's PUCCH HARQ-(N)-ACK report and the PUCCH resource indicator.
  virtual pucch_harq_ack_grant alloc_common_pucch_harq_ack_ue(cell_resource_allocator&    res_alloc,
                                                              rnti_t                      tcrnti,
                                                              unsigned                    pdsch_time_domain_resource,
                                                              unsigned                    k1,
                                                              const pdcch_dl_information& dci_info) = 0;

  /// Allocate the PUCCH resource for a UE's SR opportunity.
  /// \param[out,in] slot_alloc struct with scheduling results.
  /// \param[in] crnti C-RNTI of the UE.
  /// \param[in] ue_cell_cfg user configuration.
  virtual void pucch_allocate_sr_opportunity(cell_slot_resource_allocator& pucch_slot_alloc,
                                             rnti_t                        crnti,
                                             const ue_cell_configuration&  ue_cell_cfg) = 0;

  /// Allocate a PUCCH HARQ-ACK grant for a given UE using dedicated resources.
  ///
  /// \remark This function does not check whether there are PUSCH grants allocated for the same UE. The check needs to
  /// be performed by the caller.
  ///
  /// \param[out,in] res_alloc struct with scheduling results.
  /// \param[in] crnti RNTI of the UE.
  /// \param[in] ue_cell_cfg user configuration.
  /// \param[in] pdsch_time_domain_resource k0 value, or delay (in slots) of PDSCH slot vs the corresponding PDCCH slot.
  /// \param[in] k1 delay in slots of the UE's PUCCH HARQ-ACK report with respect to the PDSCH.
  /// \return The grant for the UE's PUCCH HARQ-(N)-ACK report and the PUCCH resource indicator.
  virtual pucch_harq_ack_grant alloc_ded_pucch_harq_ack_ue(cell_resource_allocator&     res_alloc,
                                                           rnti_t                       crnti,
                                                           const ue_cell_configuration& ue_cell_cfg,
                                                           unsigned                     pdsch_time_domain_resource,
                                                           unsigned                     k1) = 0;

  /// Remove UCI allocations on PUCCH for a given UE.
  /// \param[out,in] slot_alloc struct with scheduling results.
  /// \param[in] crnti RNTI of the UE.
  /// \return struct with the number of HARQ-ACK and CSI info bits from the removed PUCCH grants. If there was no PUCCH
  /// to be removed, return 0 for both HARQ-ACK and CSI info bits.
  virtual pucch_uci_bits remove_ue_uci_from_pucch(cell_slot_resource_allocator& slot_alloc, rnti_t crnti) = 0;
};

} // namespace srsgnb
