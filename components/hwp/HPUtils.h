#pragma once
// #include "esphome/core/hal.h"
#include "CS.h"
#include <cstdint>
#include <type_traits>

// Utility to perform a generic memory comparison using std::memcmp.
#include "esphome/components/watchdog/watchdog.h"
#include "esphome/core/optional.h"
namespace esphome {
namespace hwp {

/**
 * @brief Clears the bit at a specified position in a byte.
 *
 * @param byte Pointer to the byte in which the bit is to be cleared.
 * @param position The position of the bit to clear (0-based index).
 */
inline void clear_bit(uint8_t* byte, uint8_t position) {
    if (byte == nullptr) return;
    *byte &= ~(1 << position);
}

/**
 * @brief Sets the bit at a specified position in a byte.
 *
 * @param byte Pointer to the byte in which the bit is to be set.
 * @param position The position of the bit to set (0-based index).
 */
inline void set_bit(uint8_t* byte, uint8_t position) {
    if (byte == nullptr) return;
    *byte |= (1 << position);
}
/**
 * @brief Gets the bit at a specified position in a byte.
 *
 * @param byte Byte in which the bit is to be read
 * @param position The position of the bit to get (0-based index).
 */
inline bool get_bit(uint8_t byte, uint8_t position) { return byte & (1 << position); }

template <typename T> bool memcmp_equal(const T& lhs, const T& rhs);
template <typename T> bool update_if_changed(esphome::optional<T>& original, const T& new_value);
std::string format_bool_diff(bool current, bool reference);
template <typename T>
std::string format_diff(const T& current, const T& reference, const char* sep = " ") {
    CS cs;
    bool changed = (current != reference);
    cs.set_changed_base_color(changed);

    auto cs_inv = changed ? CS::invert : "";
    auto cs_inv_rst = changed ? CS::invert_rst : "";
    cs << cs_inv << current << cs_inv_rst << sep;
    return cs.str();
}

/**
 * @brief Inverts a byte array (i.e., sets each bit to its opposite state).
 *
 * @param buffer The array to be inverted.
 * @param length The number of bytes in the array to be inverted.
 */
template <size_t N> void inverse(uint8_t (&buffer)[N], const size_t length) {
    for (unsigned long i = 0; i < length && i < N; i++) {
        buffer[i] = ~buffer[i];
    }
}


} // namespace hwp
} // namespace esphome

#ifdef USE_ARDUINO
#include <esp32-hal.h>

#else
#include "esp_timer.h"
static inline uint32_t millis() { return (uint32_t)(esp_timer_get_time() / 1000); }
#endif
