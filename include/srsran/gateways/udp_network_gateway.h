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

#include "srsran/gateways/network_gateway.h"
#include "srsran/support/io/io_broker.h"
#include <netdb.h>
#include <sys/types.h>

struct sockaddr_storage;

namespace srsran {

struct udp_network_gateway_config : common_network_gateway_config {
  unsigned               rx_max_mmsg              = 256;
  float                  pool_occupancy_threshold = 0.9;
  std::optional<uint8_t> dscp;
  std::string            ext_bind_addr = "auto";
};

/// Interface to inject PDUs into gateway entity.
class udp_network_gateway_data_handler
{
public:
  virtual ~udp_network_gateway_data_handler() = default;

  /// \brief handle_pdu Transmit a new PDU.
  /// \param pdu The PDU to be transmitted.
  /// \param dest_addr The destination address of that PDU.
  virtual void handle_pdu(byte_buffer pdu, const sockaddr_storage& dest_addr) = 0;
};

/// Interface to trigger bind/listen/connect operations on gateway socket.
class udp_network_gateway_controller
{
public:
  virtual ~udp_network_gateway_controller() = default;

  /// \brief Create and bind the configured address and port.
  virtual bool create_and_bind() = 0;

  /// \brief Trigger receive call on socket.
  virtual void receive() = 0;

  /// \brief Return socket file descriptor.
  virtual int get_socket_fd() = 0;

  /// \brief Return the port to which the socket is bound.
  ///
  /// In case the gateway was configured to bind to port 0, i.e. the operating system shall pick a random free port,
  /// this function can be used to get the actual port number.
  virtual std::optional<uint16_t> get_bind_port() const = 0;

  /// \brief Return the address to which the socket is bound.
  ///
  /// In case the gateway was configured to use a hostname,
  /// this function can be used to get the actual IP address in string form.
  virtual bool get_bind_address(std::string& ip_address) const = 0;

  /// \brief Register the UDP gateway in the IO broker for automatic handling of notifications.
  virtual bool subscribe_to(io_broker& broker) = 0;
};

class udp_network_gateway : public udp_network_gateway_data_handler, public udp_network_gateway_controller
{};

} // namespace srsran
