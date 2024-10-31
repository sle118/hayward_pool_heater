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
 * @brief Represents configuration A packet with defrost and flow meter settings.
 *
 * - `unknown_1_3`: Unknown bits (bits 1-3).
 * - `U01_flow_meter`: Flow meter activation (bit 4).
 * - `d06_defrost_eco_mode`: Defrosting economy mode (bit 5).
 * - `unknown_4_6`: Unknown bits (bits 6-8).
 * - `unknown_bit_8`: Reserved for future use.
 * - `d05_min_economy_defrost_time_minutes`: Minimum defrost time in economy mode (in minutes).
 * - `unknown_4` to `unknown_8`: Reserved or unknown fields.
 * - `U02_pulses_per_liter`: Flow meter pulses per liter (10th byte).
 */
#pragma pack(push, 1)

typedef struct conf_5_byte_2 {
    union {
        struct {
            uint8_t unknown_1 : 1;            // Unknown field
            uint8_t unknown_2 : 1;            // Unknown field
            uint8_t U01_flow_meter : 1;       // Flow meter (3rd bit)
            uint8_t unknown_4 : 1;            // Unknown field
            uint8_t unknown_5 : 1;            // Unknown field
            uint8_t unknown_6 : 1;            // Unknown field
            uint8_t d06_defrost_eco_mode : 1; // Defrost mode economy (7th bit)
            uint8_t unknown_8 : 1;            // Unknown field
        };
        bits_details_t raw_byte_2; // Raw byte representation
    };
    const char* get_defrost_mode_name() const {
        return (this->d06_defrost_eco_mode > 0 ? "ECO   " : "NORMAL");
    }
    bool is_eco_mode() const { return this->d06_defrost_eco_mode > 0; }
    DefrostEcoMode get_eco_mode() const {
        return this->d06_defrost_eco_mode > 0 ? DefrostEcoMode::Eco : DefrostEcoMode::Normal;
    }
    FlowMeterEnable get_flow_meter() const { return is_flow_meter_on() ? FlowMeterEnable::Enabled : FlowMeterEnable::Disabled; }
    void set_eco_mode(const DefrostEcoMode& eco) { this->d06_defrost_eco_mode = eco ? 1 : 0; }
    bool is_flow_meter_on() const { return this->U01_flow_meter > 0; }
    void set_flow_meter(const FlowMeterEnable& on) { this->U01_flow_meter = on ? 1 : 0; }
    const char* get_flow_meter_name() const { return (this->U01_flow_meter > 0 ? "ON " : "OFF"); }
    std::string format(const struct conf_5_byte_2& other, const char* sep = "") const {
        CS oss;
        bool changed = (*this != other);

        oss.set_changed_base_color(changed);
        oss << "( " << bits_details::bit(this->unknown_1, other.unknown_1)
            << bits_details::bit(this->unknown_2, other.unknown_2) << "[U01 Flow meter: "
            << format_diff(this->get_flow_meter_name(), other.get_flow_meter_name(), "] ")
            << bits_details::bit(this->unknown_4, other.unknown_4)
            << bits_details::bit(this->unknown_5, other.unknown_5)
            << bits_details::bit(this->unknown_6, other.unknown_6) << " [d06 defrost: "
            << format_diff(this->get_defrost_mode_name(), other.get_defrost_mode_name(), "] ")
            << bits_details::bit(this->unknown_8, other.unknown_8) << " )" << sep;

        return oss.str();
    }
    bool operator==(const struct conf_5_byte_2& other) const {
        return this->raw_byte_2 == other.raw_byte_2;
    }
    bool operator!=(const struct conf_5_byte_2& other) const { return !(*this == other); }
} conf_5_byte_2_t;
typedef struct conf_5 {
    uint8_t id; ///< ID byte

    conf_5_byte_2_t flags_a;

    decimal_number_t
        d05_min_economy_defrost_time_minutes; // Minimum defrost time in economy mode (minutes)
    bits_details_t unknown_4;                 // Reserved field
    bits_details_t unknown_5;                 // Reserved field
    bits_details_t unknown_6;                 // Reserved field
    bits_details_t unknown_7;                 // Reserved field
    bits_details_t unknown_8;                 // Reserved field
    large_integer_t U02_pulses_per_liter;             // Flow meter pulses per liter (10th byte)

    bool operator==(const optional<conf_5>& other) const {
        return other.has_value() && *this == *other;
    }
    bool operator==(const struct conf_5& other) const {
        return this->id == other.id && this->flags_a == other.flags_a &&
               this->d05_min_economy_defrost_time_minutes ==
                   other.d05_min_economy_defrost_time_minutes &&
               this->unknown_4 == other.unknown_4 && this->unknown_5 == other.unknown_5 &&
               this->unknown_6 == other.unknown_6 && this->unknown_7 == other.unknown_7 &&
               this->unknown_8 == other.unknown_8 &&
               this->U02_pulses_per_liter == other.U02_pulses_per_liter;
    }
    bool operator!=(const optional<struct conf_5>& other) const { return !(*this == other); }
    bool operator!=(const struct conf_5& other) const { return !(*this == other); }

} __attribute__((packed)) conf_5_t;
static_assert(sizeof(conf_5_t) == 10, "frame structure has wrong size");
#pragma pack(pop)

class FrameConf5 : public BaseFrame {
  public:
    CLASS_DEFAULT_IMPL(FrameConf5, conf_5_t);
    static constexpr uint8_t FRAME_ID_CONF_5 = 0x85;
};

} // namespace hwp
} // namespace esphome
