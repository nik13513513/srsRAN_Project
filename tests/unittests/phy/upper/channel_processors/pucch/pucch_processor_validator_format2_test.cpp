/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "../../../support/resource_grid_test_doubles.h"
#include "pucch_processor_test_fixture.h"
#include "srsran/phy/upper/channel_processors/channel_processor_formatters.h"
#include "srsran/phy/upper/channel_processors/pucch/factories.h"
#include "srsran/phy/upper/channel_processors/pucch/formatters.h"
#include "srsran/ran/pucch/pucch_constants.h"
#include <fmt/ostream.h>
#include <gtest/gtest.h>
#include <regex>

using namespace srsran;

namespace {

// Maximum number of UCI payload bits supported by the current PUCCH Format 2 implementation.
constexpr unsigned PUCCH_F2_IMPL_MAX_NBITS = 1706;

// Valid PUCCH Format 2 configuration.
const pucch_processor::format2_configuration base_format_2_config = {
    // Context.
    std::nullopt,
    // Slot.
    {0, 9},
    // CP.
    cyclic_prefix::NORMAL,
    // Rx Ports.
    {0},
    // BWP size.
    50,
    // BWP start.
    10,
    // Starting PRB.
    1,
    // Second hop PRB.
    {},
    // Number of PRB.
    10,
    // Start symbol index.
    12,
    // Number of OFDM symbols.
    1,
    // RNTI.
    65535,
    // N_ID.
    0,
    // N_ID_0.
    0,
    // Number of HARQ-ACK bits.
    pucch_constants::FORMAT2_MIN_UCI_NBITS,
    // Number of SR bits.
    0,
    // Number of CSI Part 1 bits.
    0,
    // Number of CSI Part 2 bits.
    0};

// Test case parameters structure.
struct test_params {
  pucch_processor::format2_configuration config;
  std::string                            assert_message;
};

struct test_case_t {
  std::function<test_params()> get_test_params;
};

std::ostream& operator<<(std::ostream& os, const test_case_t& test_case)
{
  fmt::print(os, "{}", test_case.get_test_params().config);
  return os;
}

} // namespace

using PucchProcessorFormat2Param   = test_case_t;
using PucchProcessorFormat2Fixture = PucchProcessorTestFixture<PucchProcessorFormat2Param, MAX_NSYMB_PER_SLOT - 1, 1>;
template <>
std::unique_ptr<pucch_processor> PucchProcessorFormat2Fixture::processor = nullptr;
template <>
std::unique_ptr<pucch_pdu_validator> PucchProcessorFormat2Fixture::validator = nullptr;

// Maximum channel dimensions used to construct the PUCCH processor.
static constexpr channel_estimate::channel_estimate_dimensions max_dimensions =
    PucchProcessorFormat2Fixture::channel_estimate_dimensions;

// Test cases are implemented as lambda functions that generate and return an invalid PUCCH Format 2 configuration,
// along with the expected assert message.
const std::vector<test_case_t> processoressor_validator_test_data = {
    {
        [] {
          test_params entry         = {};
          entry.config              = base_format_2_config;
          entry.config.bwp_start_rb = 10;
          entry.config.bwp_size_rb  = MAX_RB - entry.config.bwp_start_rb + 1;
          entry.assert_message      = fmt::format(
              R"(BWP allocation goes up to PRB {}\, exceeding the configured maximum grid RB size\, i\.e\.\, {}\.)",
              entry.config.bwp_start_rb + entry.config.bwp_size_rb,
              MAX_RB);
          return entry;
        },
    },
    {
        [] {
          test_params entry         = {};
          entry.config              = base_format_2_config;
          entry.config.starting_prb = entry.config.bwp_size_rb - entry.config.nof_prb + 1;
          entry.assert_message =
              fmt::format(R"(PRB allocation within the BWP goes up to PRB {}\, exceeding BWP size\, i\.e\.\, {}\.)",
                          entry.config.starting_prb + entry.config.nof_prb,
                          entry.config.bwp_size_rb);
          return entry;
        },
    },
    {
        [] {
          test_params entry               = {};
          entry.config                    = base_format_2_config;
          entry.config.start_symbol_index = get_nsymb_per_slot(entry.config.cp) - entry.config.nof_symbols + 1;
          entry.assert_message            = fmt::format(
              R"(OFDM symbol allocation goes up to symbol {}\, exceeding the number of symbols in the given slot with {} CP\, i\.e\.\, {}\.)",
              entry.config.start_symbol_index + entry.config.nof_symbols,
              entry.config.cp.to_string(),
              get_nsymb_per_slot(entry.config.cp));
          return entry;
        },
    },
    {
        [] {
          test_params entry               = {};
          entry.config                    = base_format_2_config;
          entry.config.cp                 = cyclic_prefix::NORMAL;
          entry.config.nof_symbols        = 1;
          entry.config.start_symbol_index = max_dimensions.nof_symbols;
          entry.assert_message            = fmt::format(
              R"(OFDM symbol allocation goes up to symbol {}\, exceeding the configured maximum number of slot symbols\, i\.e\.\, {}\.)",
              entry.config.start_symbol_index + entry.config.nof_symbols,
              max_dimensions.nof_symbols);
          return entry;
        },
    },
    {
        [] {
          test_params entry    = {};
          entry.config         = base_format_2_config;
          entry.config.ports   = {};
          entry.assert_message = R"(The number of receive ports cannot be zero\.)";
          return entry;
        },
    },
    {
        [] {
          test_params entry    = {};
          entry.config         = base_format_2_config;
          entry.config.ports   = {0, 1};
          entry.assert_message = fmt::format(
              R"(The number of receive ports\, i\.e\. {}\, exceeds the configured maximum number of receive ports\, i\.e\.\, {}\.)",
              entry.config.ports.size(),
              max_dimensions.nof_rx_ports);
          return entry;
        },
    },
    {
        [] {
          test_params entry          = {};
          entry.config               = base_format_2_config;
          entry.config.nof_csi_part2 = 1;
          entry.assert_message       = R"(CSI Part 2 is not currently supported\.)";
          return entry;
        },
    },
    {
        [] {
          test_params entry          = {};
          entry.config               = base_format_2_config;
          entry.config.nof_harq_ack  = pucch_constants::FORMAT2_MIN_UCI_NBITS - 1;
          entry.config.nof_sr        = 0;
          entry.config.nof_csi_part1 = 0;
          entry.config.nof_csi_part2 = 0;
          entry.assert_message       = fmt::format(
              R"(UCI Payload length\, i\.e\.\, {} is not supported\. Payload length must be {} to {} bits\.)",
              entry.config.nof_harq_ack + entry.config.nof_sr + entry.config.nof_csi_part1 + entry.config.nof_csi_part2,
              pucch_constants::FORMAT2_MIN_UCI_NBITS,
              PUCCH_F2_IMPL_MAX_NBITS);
          return entry;
        },
    },
    {
        [] {
          test_params entry         = {};
          entry.config              = base_format_2_config;
          entry.config.nof_harq_ack = uci_constants::MAX_NOF_HARQ_BITS;
          entry.assert_message =
              R"(The effective code rate \(i\.e\., [0-9]*\.[0-9]*\) exceeds the maximum allowed 0\.8\.)";
          return entry;
        },
    },
    {
        [] {
          test_params entry               = {};
          entry.config                    = base_format_2_config;
          entry.config.nof_harq_ack       = PUCCH_F2_IMPL_MAX_NBITS;
          entry.config.nof_sr             = 1;
          entry.config.nof_csi_part1      = 0;
          entry.config.nof_csi_part2      = 0;
          entry.config.start_symbol_index = 0;
          entry.config.nof_symbols        = max_dimensions.nof_symbols;
          entry.config.starting_prb       = 0;
          entry.config.nof_prb            = entry.config.bwp_size_rb;
          entry.assert_message            = fmt::format(
              R"(UCI Payload length\, i\.e\.\, {} is not supported\. Payload length must be {} to {} bits\.)",
              entry.config.nof_harq_ack + entry.config.nof_sr + entry.config.nof_csi_part1 + entry.config.nof_csi_part2,
              pucch_constants::FORMAT2_MIN_UCI_NBITS,
              PUCCH_F2_IMPL_MAX_NBITS);
          return entry;
        },
    }};

TEST_P(PucchProcessorFormat2Fixture, PucchProcessorValidatortest)
{
  ASSERT_NE(processor, nullptr) << "PUCCH processor not created.";
  ASSERT_NE(validator, nullptr) << "PUCCH validator not created.";

  const test_case_t& param = GetParam();

  // Make sure the configuration is invalid.
  error_type<std::string> validator_out = validator->is_valid(param.get_test_params().config);
  ASSERT_FALSE(validator_out.has_value()) << "Validation should fail.";
  ASSERT_TRUE(std::regex_match(validator_out.error(), std::regex(param.get_test_params().assert_message)))
      << "The assertion message doesn't match the expected pattern.";

  // Prepare resource grid.
  resource_grid_reader_spy grid;

  // Process PUCCH PDU.
#ifdef ASSERTS_ENABLED
  ASSERT_DEATH({ processor->process(grid, param.get_test_params().config); }, param.get_test_params().assert_message);
#endif // ASSERTS_ENABLED
}

// Creates test suite that combines all possible parameters.
INSTANTIATE_TEST_SUITE_P(PucchProcessorValidatortest,
                         PucchProcessorFormat2Fixture,
                         ::testing::ValuesIn(processoressor_validator_test_data));
