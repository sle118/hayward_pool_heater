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

#include "Schema.h"
#include "base_frame.h"
#include "CS.h"
namespace esphome {
  namespace hwp {


    /**
     * @brief Structure for temperature output data
     *
     * - reserved_x: Unknown details of these bits, likely placeholders for future use.
     * - t03_temperature: The temperature value at byte 4.
     * - t06_temperature_exhaust: Temperature of the exhaust.
     * - t04_temperature_coil: Temperature of the coil.
     * - temperature_4: Fourth temperature sensor, possibly external.
     */
    typedef struct conditions2 {
      uint8_t id;
      bits_details_t reserved_1;
      bits_details_t reserved_2;
      temperature_t t03_temperature;  // Temperature byte at data[4]
      temperature_t t06_temperature_exhaust;
      temperature_t t04_temperature_coil;
      bits_details_t reserved_5;
      temperature_t temperature_4;
      bits_details_t reserved_7;
      bits_details_t reserved_8;
      bool operator==(const optional<struct conditions2>& other) const {
        return other.has_value() && *this == *other;
      }
bool operator==(const struct conditions2& other) const {
  return this->id == other.id &&
         this->reserved_1 == other.reserved_1 &&
         this->reserved_2 == other.reserved_2 &&
         this->t03_temperature == other.t03_temperature &&
         this->t06_temperature_exhaust == other.t06_temperature_exhaust &&
         this->t04_temperature_coil == other.t04_temperature_coil &&
         this->reserved_5 == other.reserved_5 &&
         this->temperature_4 == other.temperature_4 &&
         this->reserved_7 == other.reserved_7 &&
         this->reserved_8 == other.reserved_8;
    }
    bool operator!=(const optional<struct conditions2>& other) const { return !(*this == other); }
    bool operator!=(const struct conditions2& other) const { return !(*this == other); }
    } __attribute__((packed)) conditions2_t;

static_assert(sizeof(conditions2_t) == frame_data_length-2, "frame structure has wrong size");


    class FrameConditions2 : public BaseFrame {
    public:
      CLASS_DEFAULT_IMPL(FrameConditions2, conditions2_t);
      float get_out_temp() const;
      float get_exhaust_temp() const;
      float get_coil_temp() const;
      static constexpr uint8_t FRAME_ID_CONDITIONS2 = 0xD2;   // B 01001011 (inv: 11010010)

    };

  }  // namespace hwp
}  // namespace esphome
