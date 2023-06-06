/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "rrc_ue_test_helpers.h"
#include <gtest/gtest.h>

using namespace srsran;
using namespace srs_cu_cp;

/// Fixture class RRC Reestablishment tests preparation
class rrc_ue_reest : public rrc_ue_test_helper, public ::testing::Test
{
protected:
  static void SetUpTestSuite() { srslog::init(); }

  void SetUp() override { init(); }

  void TearDown() override
  {
    // flush logger after each test
    srslog::flush();
  }
};

/// Test the RRC Reestablishment
TEST_F(rrc_ue_reest, when_invalid_reestablishment_request_received_then_rrc_setup_sent)
{
  connect_amf();
  receive_invalid_reestablishment_request(0, to_rnti(0x4601));

  // check if the RRC Setup Request was generated
  ASSERT_EQ(get_srb0_pdu_type(), asn1::rrc_nr::dl_ccch_msg_type_c::c1_c_::types::rrc_setup);

  // check if SRB1 was created
  check_srb1_exists();

  receive_setup_complete();

  check_initial_ue_message_sent();
}

/// Test the RRC Reestablishment
TEST_F(rrc_ue_reest, when_valid_reestablishment_request_received_but_security_context_not_found_then_rrc_setup_sent)
{
  connect_amf();
  receive_valid_reestablishment_request(1, to_rnti(0x4601));

  // check if the RRC Setup Request was generated
  ASSERT_EQ(get_srb0_pdu_type(), asn1::rrc_nr::dl_ccch_msg_type_c::c1_c_::types::rrc_setup);

  // check if SRB1 was created
  check_srb1_exists();

  receive_setup_complete();

  check_initial_ue_message_sent();
}

// TODO Starting the RRC Re-establishment procedure is temporally disabled. Remember to activate unittest when
// enabling it.
// /// Test the RRC Reestablishment
// TEST_F(rrc_ue_reest, when_valid_reestablishment_request_received_then_rrc_reestablishment_sent)
// {
//   connect_amf();
//   add_ue_reestablishment_context(uint_to_ue_index(0));
//   receive_valid_reestablishment_request(1, to_rnti(0x4601));

//   // check if SRB1 was created
//   check_srb1_exists();

//   // check if the RRC Reestablishment was generated
//   ASSERT_EQ(get_srb1_pdu_type(), asn1::rrc_nr::dl_dcch_msg_type_c::c1_c_::types::rrc_reest);

//   receive_reestablishment_complete();
// }
