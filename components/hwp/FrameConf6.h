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
typedef struct conf_6 {
    uint8_t id;                                      ///< ID byte
    bits_details_t unknown_1;                        // Unknown field
    bits_details_t unknown_2;                        // Unknown field
    bits_details_t unknown_3;                        // Unknown field
    bits_details_t unknown_4;                        // Unknown field
    bits_details_t unknown_5;                        // Unknown field
    bits_details_t unknown_6;                        // Unknown field
    bits_details_t unknown_7;                        // Unknown field
    bits_details_t unknown_8;                        // Unknown field
    bits_details_t unknown_9;                        // Unknown field
    
    bool operator==(const optional<struct conf_6>& other) const {
        return other.has_value() && *this == *other;
    }
    bool operator==(const struct conf_6& other) const {
        return id == other.id && unknown_1 == other.unknown_1 && unknown_2 == other.unknown_2 &&
               unknown_3 == other.unknown_3 && unknown_4 == other.unknown_4 &&
               unknown_5 == other.unknown_5 && unknown_6 == other.unknown_6 &&
               unknown_7 == other.unknown_7 && unknown_8 == other.unknown_8 &&
               unknown_9 == other.unknown_9;
    }

    bool operator!=(const struct conf_6& other) const { return !(*this == other); }
    bool operator!=(const optional<struct conf_6>& other) const {
        return !(*this == other);
    }
} __attribute__((packed)) conf_6_t;
static_assert(sizeof(conf_6_t) == frame_data_length - 2, "frame structure has wrong size");

class FrameConf6 : public BaseFrame {
  public:
    CLASS_DEFAULT_IMPL(FrameConf6, conf_6_t);
    static constexpr uint8_t FRAME_ID_CONF_6 = 0x86;
    void traits(climate::ClimateTraits& traits, heat_pump_data_t& hp_data) override;
};

} // namespace hwp
} // namespace esphome
