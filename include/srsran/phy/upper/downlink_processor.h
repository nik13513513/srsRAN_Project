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

/// \file
/// \brief Downlink processor classes.
///
/// The downlink processor handles all the given downlink PDUs and sends the configured resource grid through a gateway
/// when every PDU for the given slot has finished processing.
///
/// The slot context processing starts with the downlink processor controller interface. The downlink processor slot
/// context is configured once using \ref downlink_processor_controller::configure_resource_grid() which returns a
/// unique downlink processor.
///
/// The unique downlink processor will accept downlink transmissions as the object is available in a scope. When the
/// unique downlink processor is released or deleted, the downlink processor controller will not accept configuring a
/// new slot context until all the transmissions are completed and the resource grid is sent over the gateway.
///

#include "srsran/phy/upper/channel_processors/pdcch/pdcch_processor.h"
#include "srsran/phy/upper/channel_processors/pdsch/pdsch_processor.h"
#include "srsran/phy/upper/channel_processors/ssb_processor.h"
#include "srsran/phy/upper/signal_processors/nzp_csi_rs_generator.h"

namespace srsran {

struct resource_grid_context;
class shared_resource_grid;

/// Downlink processor interface that groups and processes all the downlink channels within a slot.
class downlink_processor
{
public:
  virtual ~downlink_processor() = default;

  /// \brief Process the given PDCCH PDU.
  ///
  /// \param[in] pdu PDCCH PDU to process.
  virtual void process_pdcch(const pdcch_processor::pdu_t& pdu) = 0;

  /// \brief Process the given PDSCH PDU and its related data, which it is given in data parameter.
  ///
  /// \param[in]     data      Contains the PDSCH transport blocks.
  /// \param[in]     pdu       PDSCH PDU to process.
  virtual void process_pdsch(static_vector<shared_transport_block, pdsch_processor::MAX_NOF_TRANSPORT_BLOCKS> data,
                             const pdsch_processor::pdu_t&                                                    pdu) = 0;

  /// \brief Process the given SSB PDU.
  ///
  /// \param[in] pdu SS/PBCH block PDU to process.
  virtual void process_ssb(const ssb_processor::pdu_t& pdu) = 0;

  /// \brief Process the given NZP-CSI-RS configuration.
  ///
  /// \param[in] config NZP-CSI-RS configuration to process.
  virtual void process_nzp_csi_rs(const nzp_csi_rs_generator::config_t& config) = 0;
};

/// \brief Unique downlink processor.
///
/// Keeps the downlink processor interface available for processing downlink transmissions as long as it is available in
/// a scope.
///
/// The downlink processor closes the window for accepting new transmissions when either the unique downlink processor
/// is destroyed or released.
class unique_downlink_processor
{
public:
  /// Downlink processor underlying interface.
  class downlink_processor_callback : public downlink_processor
  {
  public:
    /// Default destructor.
    virtual ~downlink_processor_callback() = default;

    /// \brief Stops accepting PDUs.
    ///
    /// When this method is called, the interface will not expect to process more PDUs, so once it finishes to process
    /// all the enqueued PDUs, the resource grid will be sent to the lower bound gateway using the \c context from
    /// configure_resource_grid() to provide the processing context of the resource grid in the lower physical layer.
    virtual void finish_processing_pdus() = 0;
  };

  /// Default constructor - creates an invalid downlink processor.
  unique_downlink_processor() = default;

  /// Builds a unique downlink processor from an underlying instance.
  explicit unique_downlink_processor(downlink_processor_callback& processor_) : processor(&processor_) {}

  /// Disabled copy constructor.
  unique_downlink_processor(const unique_downlink_processor&) = delete;

  /// Disabled copy assignment operator.
  unique_downlink_processor& operator=(const unique_downlink_processor& other) = delete;

  /// Move constructor.
  unique_downlink_processor(unique_downlink_processor&& other) noexcept
  {
    release();
    processor       = other.processor;
    other.processor = nullptr;
  }

  /// Move assignment operator.
  unique_downlink_processor& operator=(unique_downlink_processor&& other) noexcept
  {
    release();
    processor       = other.processor;
    other.processor = nullptr;
    return *this;
  }

  /// Returns True if the unique processor is valid, false otherwise.
  bool is_valid() const { return processor != nullptr; }

  /// Default destructor - notifies the end of processing PDUs.
  ~unique_downlink_processor() { release(); }

  /// Releases the unique processor.
  void release()
  {
    if (is_valid()) {
      processor->finish_processing_pdus();
    }
    processor = nullptr;
  }

  /// Gets the underlying downlink processor.
  downlink_processor& get()
  {
    report_fatal_error_if_not(is_valid(), "Invalid processor.");
    return *processor;
  }

  /// Gets the underlying downlink processor.
  downlink_processor* operator->()
  {
    report_fatal_error_if_not(is_valid(), "Invalid processor.");
    return processor;
  }

private:
  /// Reference to the underlying downlink processor. Set to \c nullptr for an invalid processor.
  downlink_processor_callback* processor = nullptr;
};

/// Downlink processor controller.
class downlink_processor_controller
{
public:
  /// Default destructor.
  virtual ~downlink_processor_controller() = default;

  /// \brief Configures the downlink processor with a slot context and resource grid.
  ///
  /// \param[in] context Slot processing context information.
  /// \param[in] grid    Resource grid associated to the slot.
  /// \return A valid unique downlink processor if the downlink processor controller. Otherwise, an invalid unique
  /// downlink processor.
  virtual unique_downlink_processor configure_resource_grid(const resource_grid_context& context,
                                                            shared_resource_grid         grid) = 0;
};

/// Downlink processor validation interface.
class downlink_pdu_validator
{
public:
  /// Default destructor.
  virtual ~downlink_pdu_validator() = default;

  /// \brief Validates the SS/PBCH block processor configuration parameters.
  /// \return True if the parameters contained in \c pdu are supported, false otherwise.
  virtual bool is_valid(const ssb_processor::pdu_t& pdu) const = 0;

  /// \brief Validates PDCCH processor configuration parameters.
  /// \return True if the parameters contained in \c pdu are supported, false otherwise.
  virtual bool is_valid(const pdcch_processor::pdu_t& pdu) const = 0;

  /// \brief Validates PDSCH processor configuration parameters.
  /// \return A success if the parameters contained in \c pdu are supported, an error message otherwise.
  virtual error_type<std::string> is_valid(const pdsch_processor::pdu_t& pdu) const = 0;

  /// \brief Validates NZP-CSI-RS generator configuration parameters.
  /// \return True if the parameters contained in \c config are supported, false otherwise.
  virtual bool is_valid(const nzp_csi_rs_generator::config_t& config) const = 0;
};

/// Pool to access a downlink processor.
class downlink_processor_pool
{
public:
  virtual ~downlink_processor_pool() = default;

  /// \brief Returns a downlink processor controller with the given slot and sector.
  ///
  /// \param slot Slot point.
  /// \param sector_id Sector ID.
  /// \return A downlink processor.
  virtual downlink_processor_controller& get_processor_controller(slot_point slot, unsigned sector_id) = 0;
};

} // namespace srsran
