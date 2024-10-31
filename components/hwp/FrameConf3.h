/**
 *
 * Copyright (c) 2024 S. Leclerc (sle118@hotmail.com)
 *
 * This file is part of the Pool Heater Controller component project.
 *
 * @project Pool Heater Controller Component
 * @developer S. Leclerc (sle118@hotmail.com)
 *
 * @license MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @disclaimer Use at your own risk. The developer assumes no responsibility
 * for any damage or loss caused by the use of this software.
 */
#pragma once

#include "CS.h"
#include "Schema.h"
#include "base_frame.h"
namespace esphome {
namespace hwp {

/**
 * @brief Defines the temperature setpoint limits for cooling and heating modes.
 *
 * - unknown_1 to unknown_5: Unknown reserved fields.
 * - r08_min_cool_setpoint: Minimum cooling setpoint temperature.
 * - r09_max_cooling_setpoint: Maximum cooling setpoint temperature.
 * - r10_min_heating_setpoint: Minimum heating setpoint temperature.
 * - r11_max_heating_setpoint: Maximum heating setpoint temperature.
 */
typedef struct conf_3 {
    uint8_t id;                                      ///< ID byte
    bits_details_t unknown_1;                        // Unknown field
    bits_details_t unknown_2;                        // Unknown field
    bits_details_t unknown_3;                        // Unknown field
    bits_details_t unknown_4;                        // Unknown field
    bits_details_t unknown_5;                        // Unknown field
    temperature_extended_t r08_min_cool_setpoint;    // Minimum cooling setpoint
    temperature_extended_t r09_max_cooling_setpoint; // Maximum cooling setpoint
    temperature_extended_t r10_min_heating_setpoint; // Minimum heating setpoint
    temperature_extended_t r11_max_heating_setpoint; // Maximum heating setpoint
    bool operator==(const optional<struct conf_3>& other) const {
        return other.has_value() && *this == *other;
    }
    bool operator==(const struct conf_3& other) const {
        return id == other.id && unknown_1 == other.unknown_1 && unknown_2 == other.unknown_2 &&
               unknown_3 == other.unknown_3 && unknown_4 == other.unknown_4 &&
               unknown_5 == other.unknown_5 &&
               r08_min_cool_setpoint == other.r08_min_cool_setpoint &&
               r09_max_cooling_setpoint == other.r09_max_cooling_setpoint &&
               r10_min_heating_setpoint == other.r10_min_heating_setpoint &&
               r11_max_heating_setpoint == other.r11_max_heating_setpoint;
    }

    bool operator!=(const struct conf_3& other) const { return !(*this == other); }
    bool operator!=(const optional<struct conf_3>& other) const {
        return !(*this == other);
    }
} __attribute__((packed)) conf_3_t;
static_assert(sizeof(conf_3_t) == frame_data_length - 2, "frame structure has wrong size");
static_assert(offsetof(conf_3_t, id) == 0, "id not at the start of the frame");
static_assert(
    offsetof(conf_3_t, unknown_1) == 1, "unknown_1 not at the start of the frame");
static_assert(
    offsetof(conf_3_t, unknown_2) == 2, "unknown_2 not at the start of the frame");
static_assert(
    offsetof(conf_3_t, unknown_3) == 3, "unknown_3 not at the start of the frame");
static_assert(
    offsetof(conf_3_t, unknown_4) == 4, "unknown_4 not at the start of the frame");
static_assert(
    offsetof(conf_3_t, unknown_5) == 5, "unknown_5 not at the start of the frame");
static_assert(offsetof(conf_3_t, r08_min_cool_setpoint) == 6,
    "r08_min_cool_setpoint not at the start of the frame");
static_assert(offsetof(conf_3_t, r09_max_cooling_setpoint) == 7,
    "r09_max_cooling_setpoint not at the start of the frame");
static_assert(offsetof(conf_3_t, r10_min_heating_setpoint) == 8,
    "r10_min_heating_setpoint not at the start of the frame");
static_assert(offsetof(conf_3_t, r11_max_heating_setpoint) == 9,
    "r11_max_heating_setpoint not at the start of the frame");

class FrameConf3 : public BaseFrame {
  public:
    CLASS_DEFAULT_IMPL(FrameConf3, conf_3_t);
    static constexpr uint8_t FRAME_ID_CONF_3 = 0x83;
    void traits(climate::ClimateTraits& traits, heat_pump_data_t& hp_data) override;
};

} // namespace hwp
} // namespace esphome
