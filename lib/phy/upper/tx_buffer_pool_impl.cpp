/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "tx_buffer_pool_impl.h"
#include "tx_buffer_impl.h"
#include "srsran/phy/upper/trx_buffer_identifier.h"
#include "srsran/phy/upper/tx_buffer_pool.h"
#include "srsran/phy/upper/unique_tx_buffer.h"
#include "srsran/ran/slot_point.h"
#include <algorithm>
#include <chrono>
#include <memory>
#include <thread>

using namespace srsran;

unique_tx_buffer
tx_buffer_pool_impl::reserve(const slot_point& slot, trx_buffer_identifier id, unsigned nof_codeblocks, bool new_data)
{
  // No more reservations are allowed if the pool is stopped.
  if (stopped.load(std::memory_order_acquire)) {
    return unique_tx_buffer();
  }

  // Try to find the HARQ identifier.
  auto id_found = std::find(identifiers.begin(), identifiers.end(), id);

  // Find an available buffer if no buffer was found with the same identifier if new data is true, otherwise return an
  // invalid buffer.
  if (id_found == identifiers.end()) {
    if (new_data) {
      id_found = std::find(identifiers.begin(), identifiers.end(), trx_buffer_identifier::invalid());
    } else {
      logger.warning(slot.sfn(),
                     slot.slot_index(),
                     "DL HARQ {}: failed to reserve, identifier for retransmissions not found.",
                     id);
      return unique_tx_buffer();
    }
  }

  // Report warning and return invalid buffer if no available buffer has been found.
  if (id_found == identifiers.end()) {
    logger.warning(
        slot.sfn(), slot.slot_index(), "DL HARQ {}: failed to reserve, insufficient buffers in the pool.", id);
    return unique_tx_buffer();
  }

  // Get buffer index within the pool.
  unsigned i_buffer = id_found - identifiers.begin();

  // Get reference to the buffer.
  tx_buffer_impl& buffer = buffers[i_buffer];

  // Make sure that the codeblocks do not change for retransmissions.
  if (!new_data && nof_codeblocks != buffer.get_nof_codeblocks()) {
    logger.warning(slot.sfn(),
                   slot.slot_index(),
                   "DL HARQ {}: failed to reserve, number of codeblocks for retransmissions do not match.",
                   id);
    return unique_tx_buffer();
  }

  // Reserve codeblocks.
  tx_buffer_status status = buffer.reserve(nof_codeblocks);

  // Report warning and return invalid buffer if the reservation is not successful.
  if (status != tx_buffer_status::successful) {
    logger.warning(slot.sfn(),
                   slot.slot_index(),
                   "DL HARQ {}: failed to reserve, {}.",
                   id,
                   (status == tx_buffer_status::already_in_use) ? "HARQ already in use" : "insufficient CBs");
    return unique_tx_buffer();
  }

  // Update identifier and expiration.
  identifiers[i_buffer] = id;
  expirations[i_buffer] = slot + expire_timeout_slots;

  // Create buffer.
  return unique_tx_buffer(buffer);
}

unique_tx_buffer tx_buffer_pool_impl::reserve(const slot_point& slot, unsigned nof_codeblocks)
{
  // No more reservations are allowed if the pool is stopped.
  if (stopped) {
    return unique_tx_buffer();
  }

  // Find an available buffer if no buffer was found with the same identifier.
  auto id_found = std::find(identifiers.begin(), identifiers.end(), trx_buffer_identifier::invalid());

  // Report warning and return invalid buffer if no available buffer has been found.
  if (id_found == identifiers.end()) {
    logger.warning(
        slot.sfn(), slot.slot_index(), "DL HARQ invalid: failed to reserve, insufficient buffers in the pool.");
    return unique_tx_buffer();
  }

  // Get buffer index within the pool.
  unsigned i_buffer = id_found - identifiers.begin();

  // Get reference to the buffer.
  tx_buffer_impl& buffer = buffers[i_buffer];

  // Reserve codeblocks.
  tx_buffer_status status = buffer.reserve(nof_codeblocks);

  // Report warning and return invalid buffer if the reservation is not successful.
  if (status != tx_buffer_status::successful) {
    logger.warning(slot.sfn(),
                   slot.slot_index(),
                   "DL HARQ invalid: failed to reserve, {}.",
                   (status == tx_buffer_status::already_in_use) ? "HARQ already in use" : "insufficient CBs");
    return unique_tx_buffer();
  }

  // Update identifier and expiration.
  identifiers[i_buffer] = trx_buffer_identifier::unknown();
  expirations[i_buffer] = slot + expire_timeout_slots;

  // Create buffer.
  return unique_tx_buffer(buffer);
}

void tx_buffer_pool_impl::run_slot(const slot_point& slot)
{
  // Predicate for finding available buffers.
  auto pred = [](trx_buffer_identifier id) { return id != trx_buffer_identifier::invalid(); };

  // Iterate over all the buffers that are currently reserved.
  for (auto it = std::find_if(identifiers.begin(), identifiers.end(), pred); it != identifiers.end();
       it      = std::find_if(++it, identifiers.end(), pred)) {
    // Calculate buffer index.
    unsigned i_buffer = it - identifiers.begin();

    // Get reference to the buffer.
    tx_buffer_impl& buffer = buffers[i_buffer];

    // Determines whether the buffer is free.
    bool is_free = buffer.is_free();

    // A buffer is expired if the expiration slot is lower than or equal to the current slot.
    if ((expirations[i_buffer] != null_expiration) && (expirations[i_buffer] <= slot)) {
      // Try to expire the buffer.
      is_free = buffer.expire();

      // If the buffer is not available, increase the expiration time and continue to the next buffer.
      if (!is_free) {
        expirations[i_buffer] = slot + expire_timeout_slots;
      }
    }

    // Clear identifier and expiration.
    if (is_free) {
      identifiers[i_buffer] = trx_buffer_identifier::invalid();
      expirations[i_buffer] = null_expiration;
    }
  }
}
tx_buffer_pool& tx_buffer_pool_impl::get_pool()
{
  return *this;
}

void tx_buffer_pool_impl::stop()
{
  // Signals the stop of the pool. No more reservation are allowed after this point.
  stopped.store(true, std::memory_order_release);

  // Makes sure all buffers are unlocked.
  for (const auto& buffer : buffers) {
    while (buffer.is_locked()) {
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
  }
}

std::unique_ptr<tx_buffer_pool_controller> srsran::create_tx_buffer_pool(const tx_buffer_pool_config& config)
{
  return std::make_unique<tx_buffer_pool_impl>(config);
}
