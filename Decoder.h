/**
 * @file DecodingStateFrame.h
 * @brief Defines the DecodingStateFrame class for handling the state of decoding while processing
 * pulses.
 *
 * Copyright (c) 2024 S. Leclerc (sle118@hotmail.com)
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
#include <cstring>
#include "esphome/core/log.h"
#include "HPUtils.h"
#include "base_frame.h"

namespace esphome {
namespace hayward_pool_heater {

class Decoder : public BaseFrame {
 public:
  Decoder();
  Decoder(const Decoder &other);
  Decoder &operator=(const Decoder &other);

  void reset(const char *msg = "");
  bool finalize();
  bool is_valid() const;
  void append_bit(bool long_duration);
  void start_new_frame();
  static int32_t get_high_duration(const rmt_item32_t *item);
  static uint32_t get_low_duration(const rmt_item32_t *item);
  static bool matches_duration(uint32_t target_us, uint32_t actual_us);
  static bool is_start_frame(const rmt_item32_t *item);
  static bool is_long_bit(const rmt_item32_t *item);
  static bool is_short_bit(const rmt_item32_t *item);
  bool is_started() const;
  void set_started(bool value);
  void debug(const char *msg = "");
  inline bool is_complete() const;
  void is_changed(const BaseFrame &frame);

  uint32_t passes_count;

 private:
  uint8_t currentByte;
  uint8_t bit_current_index;
  bool started;
  bool finalized;
};

}  // namespace hayward_pool_heater
}  // namespace esphome
