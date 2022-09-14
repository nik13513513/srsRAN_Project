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

#include "ngap_types.h"
#include "srsgnb/adt/optional.h"
#include "srsgnb/asn1/ngap.h"
#include "srsgnb/support/async/async_task.h"

namespace srsgnb {

namespace srs_cu_cp {

struct ngap_message {
  asn1::ngap::ngap_pdu_c pdu;
};

/// This interface is used to push NGAP messages to the NG interface.
class ng_message_handler
{
public:
  virtual ~ng_message_handler() = default;

  /// Handle the incoming NGAP message.
  virtual void handle_message(const ngap_message& msg) = 0;
};

/// Interface used by NG to inform about events.
class ng_event_handler
{
public:
  virtual ~ng_event_handler()           = default;
  virtual void handle_connection_loss() = 0;
};

/// This interface notifies the reception of new NGAP messages over the NG interface.
class ng_message_notifier
{
public:
  virtual ~ng_message_notifier() = default;

  /// This callback is invoked on each received NGAP message.
  virtual void on_new_message(const ngap_message& msg) = 0;
};

struct ng_setup_request_message {
  asn1::ngap::ng_setup_request_s msg;
  unsigned                       max_setup_retries = 5;
};

struct ng_setup_response_message {
  asn1::ngap::ng_setup_resp_s msg;
  bool                        success;
};

/// Handle NGAP interface management procedures as defined in TS 38.413 section 8.7
class ngap_connection_manager
{
public:
  virtual ~ngap_connection_manager() = default;

  /// \brief Initiates the NG Setup procedure.
  /// \param[in] request The NGSetupRequest message to transmit.
  /// \return Returns a ng_setup_response_message struct with the success member set to 'true' in case of a
  /// successful outcome, 'false' otherwise.
  /// \remark The CU transmits the NGSetupRequest as per TS 38.413 section 8.7.1
  /// and awaits the response. If a NGSetupFailure is received the NGAP will handle the failure.
  virtual async_task<ng_setup_response_message> handle_ngap_setup_request(const ng_setup_request_message& request) = 0;
};

struct ngap_initial_ue_message {
  ue_ngap_id_t                            ue_ngap_id;
  byte_buffer                             nas_pdu;
  asn1::ngap::rrcestablishment_cause_opts establishment_cause;
  asn1::ngap::nr_cgi_s                    nr_cgi;
};

/// Handle NGAP NAS Message procedures as defined in TS 38.413 section 8.6.
class ngap_nas_message_handler
{
public:
  virtual ~ngap_nas_message_handler() = default;

  /// \brief Initiates Initial UE message procedure as per TS 38.413 section 8.6.1.
  /// \param[in] msg The ngap initial UE message to transmit.
  virtual void handle_initial_ue_message(const ngap_initial_ue_message& msg) = 0;
};

/// Combined entry point for the NGAP object.
class ngap_interface : public ng_message_handler,
                       public ng_event_handler,
                       public ngap_connection_manager,
                       public ngap_nas_message_handler
{
public:
  virtual ~ngap_interface() = default;
};

} // namespace srs_cu_cp

} // namespace srsgnb