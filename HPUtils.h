#pragma once
#include "esphome/core/hal.h"
namespace esphome {
namespace hayward_pool_heater {

/**
 * @brief Clears the bit at a specified position in a byte.
 *
 * @param byte Pointer to the byte in which the bit is to be cleared.
 * @param position The position of the bit to clear (0-based index).
 */
static inline void clear_bit(uint8_t* byte, uint8_t position) {
  if (byte == nullptr) return;
  *byte &= ~(1 << position);
}

/**
 * @brief Sets the bit at a specified position in a byte.
 *
 * @param byte Pointer to the byte in which the bit is to be set.
 * @param position The position of the bit to set (0-based index).
 */
static inline void set_bit(uint8_t* byte, uint8_t position) {
  if (byte == nullptr) return;
  *byte |= (1 << position);
}
/**
 * @brief Gets the bit at a specified position in a byte.
 *
 * @param byte Byte in which the bit is to be read
 * @param position The position of the bit to get (0-based index).
 */
static inline bool get_bit(uint8_t byte, uint8_t position) { return byte & (1 << position); }
}  // namespace hayward_pool_heater
}  // namespace esphome