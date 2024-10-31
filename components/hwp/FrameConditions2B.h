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
     * @brief Represents possibly a continuation of the conditions2
     * structure, since the heading shares the same value but with a shorter
     * frame size.
     *
     * - reserved_1 to reserved_7: Reserved or unknown fields.
     */
    typedef struct conditions2b{
      bits_details_t reserved_1;  // Reserved field
      bits_details_t reserved_2;  // Reserved field
      bits_details_t reserved_3;  // Reserved field
      bits_details_t reserved_4;  // Reserved field
      bits_details_t reserved_5;  // Reserved field
      bits_details_t reserved_6;  // Reserved field
      bits_details_t reserved_7;  // Reserved field
      bool operator==(const optional<struct conditions2b>& other) const {
        return other.has_value() && *this == other.value();
      }

 bool operator==(const struct conditions2b& other) const {
        return this->reserved_1==other.reserved_1 && this->reserved_2==other.reserved_2 &&
          this->reserved_3==other.reserved_3 && this->reserved_4==other.reserved_4 && this->reserved_5==other.reserved_5 &&
          this->reserved_6==other.reserved_6 && this->reserved_7==other.reserved_7;
      }
      bool operator!=(const optional<struct conditions2b>& other) const { return !(*this==other); }
      bool operator!=(const struct conditions2b& other) const { return !(*this==other); }
    } __attribute__((packed)) conditions2b_t;
static_assert(sizeof(conditions2b_t) == frame_data_length_short-2, "frame structure has wrong size");


    class FrameConditions2B : public BaseFrame {
    public:
      CLASS_DEFAULT_IMPL(FrameConditions2B, conditions2b_t);
      

    };

  }  // namespace hwp
}  // namespace esphome
