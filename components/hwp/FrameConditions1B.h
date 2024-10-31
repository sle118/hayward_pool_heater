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
 * @brief Represents alternative internal temperature sensor data with water flow detection.
 *
 * - reserved_1 to reserved_8: Reserved fields, unknown purpose.
 * - bits_1: Unknown bit.
 * - S02_water_flow: 1 indicates water flow is detected.
 * - bits_3_8: Other unknown bits.
 * - t02_temperature: Temperature reading from the sensor at data[9].
 */
typedef struct conditions_1b {
    uint8_t id;                // ID byte
    bits_details_t reserved_1; // Reserved field
    bits_details_t reserved_2; // Reserved field
    union {
        struct {
            uint8_t bits_1 : 1;         // Unknown bit
            uint8_t S02_water_flow : 1; // 1 = Water flow detected
            uint8_t bits_3_8 : 6;       // Unknown bits
        };
        bits_details_t raw_byte_2; // Raw byte representation
    };
    bits_details_t reserved_4;     // Reserved field
    bits_details_t reserved_5;     // Reserved field
    bits_details_t reserved_6;     // Reserved field
    bits_details_t reserved_7;     // Reserved field
    temperature_t t02_temperature; // Temperature byte at data[9]
    bits_details_t reserved_8;     // Reserved field
    bool operator==(const optional<struct conditions_1b>& other) const {
        return other.has_value() && *this == *other;
    }
    FlowMeterEnable get_flow_meter_enable() const {
        return S02_water_flow ? FlowMeterEnable::Enabled : FlowMeterEnable::Disabled;
    }
    void set_flow_meter_enable(FlowMeterEnable flow_meter_enable) {
        S02_water_flow = (flow_meter_enable == FlowMeterEnable::Enabled ? 1 : 0);
    }
    bool operator==(const struct conditions_1b& other) const {
        return id == other.id && reserved_1 == other.reserved_1 && reserved_2 == other.reserved_2 &&
               raw_byte_2 == other.raw_byte_2 && reserved_4 == other.reserved_4 &&
               reserved_5 == other.reserved_5 && reserved_6 == other.reserved_6 &&
               reserved_7 == other.reserved_7 && t02_temperature == other.t02_temperature &&
               reserved_8 == other.reserved_8;
    }

    bool operator!=(const struct conditions_1b& other) const { return !(*this == other); }
    bool operator!=(const optional<struct conditions_1b>& other) const { return !(*this == other); }
} __attribute__((packed)) conditions_1b_t;
static_assert(sizeof(conditions_1b_t) == frame_data_length - 2, "frame structure has wrong size");

class FrameConditions1B : public BaseFrame {
  public:
    CLASS_DEFAULT_IMPL(FrameConditions1B, conditions_1b_t);
    static const char* get_flow_string(bool flow);
};

} // namespace hwp
} // namespace esphome
