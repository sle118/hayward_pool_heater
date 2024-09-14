/**
 * @file DecodingStateFrame.h
 * @brief Defines the DecodingStateFrame class for handling the state of decoding while processing
 * pulses.
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
#include <map>
#include <cstring>
#include "esphome/core/log.h"
#include "HPUtils.h"
#include "base_frame.h"

namespace esphome {
namespace hayward_pool_heater {
static constexpr char TAG_DECODING[] = "hayward_pool_heater.decoder";

/**
 * @class DecodingStateFrame
 * @brief Manages the state of decoding while processing pulses for bus communication.
 *
 * This class handles the decoding of frames received from the bus by processing pulses,
 * managing the current frame state, and validating the checksum.
 */
class DecodingStateFrame : public BaseFrame {
 public:
  /**
   * @brief Constructs a new DecodingStateFrame object.
   */

  DecodingStateFrame()
      : BaseFrame(), passes_count(0), currentByte(0), bit_current_index(0), started(false), finalized(false) {}

  /**
   * @brief Copy constructor for DecodingStateFrame.
   *
   * @param other The DecodingStateFrame object to copy from.
   */

  DecodingStateFrame(const DecodingStateFrame &other)
      : BaseFrame(other),
        passes_count(0),
        currentByte(other.currentByte),
        bit_current_index(other.bit_current_index),
        started(other.started),
        finalized(other.finalized) {}

  /**
   * @brief Copy assignment operator for DecodingStateFrame.
   *
   * @param other The DecodingStateFrame object to copy from.
   * @return Reference to the assigned DecodingStateFrame object.
   */
  DecodingStateFrame &operator=(const DecodingStateFrame &other) {
    if (this != &other) {
      BaseFrame::operator=(other);
      currentByte = other.currentByte;
      bit_current_index = other.bit_current_index;
      started = other.started;
      finalized = other.finalized;
      passes_count = other.passes_count;
    }
    return *this;
  }

  /**
   * @brief Resets the current frame state.
   */
  void reset(const char *msg = "") {
#ifdef USE_LOGGER
    auto *log = logger::global_logger;
    bool debug_status = (log != nullptr && log->level_for(TAG_DECODING) >= ESPHOME_LOG_LEVEL_VERY_VERBOSE);
#else
    bool debug_status = false;
#endif

    if (this->started) {
      ESP_LOGVV(TAG_DECODING, "Resetting frame");
      if (debug_status && strlen(msg) > 0) {
        debug(msg);
      }
    }
    passes_count = 0;
    bit_current_index = 0;
    data_len_ = 0;
    currentByte = 0;
    started = false;
    finalized = false;
    source_ = SOURCE_UNKNOWN;
    memset(&packet, 0x00, sizeof(packet));
  }

  /**
   * @brief Finalizes the current frame by validating the checksum.
   *
   * @return true If the frame is valid.
   * @return false Otherwise.
   */
  bool finalize() {
    bool inverted = false;
    this->source_ = SOURCE_UNKNOWN;
    this->finalized = false;
    if (!started) {
      return false;
    }
    if (!this->is_size_valid()) {
      return false;
    }

    if (is_checksum_valid(inverted)) {
      if (inverted) {
        inverseLevels();
        this->source_ = SOURCE_HEATER;
      } else {
        this->source_ = SOURCE_CONTROLLER;
      }
      finalized = true;
      frame_time_ms_ = millis();
    }
    ESP_LOGVV(TAG_DECODING, "Finalize()->frame is %s", this->finalized ? "FINALIZED" : "NOT FINALIZED");
    return this->finalized;
  }

  /**
   * @brief Checks if the frame is valid.
   *
   * @return true If the frame is valid.
   * @return false Otherwise.
   */
  bool is_valid() const { return (finalized && BaseFrame::is_valid()); }

  /**
   * @brief Appends a bit to the current byte in the frame.
   *
   * @param long_duration Indicates if the bit represents a long duration.
   */
  void append_bit(bool long_duration) {
    if (!started) {
      ESP_LOGW(TAG_DECODING, "Frame not started. Ignoring bit");
      return;
    }
    if (long_duration) {
      set_bit(&currentByte, bit_current_index);
    }
    bit_current_index++;
    if (bit_current_index == 8) {
      if (this->data_len_ < sizeof(this->packet.data)) {
        ESP_LOGVV(TAG_DECODING, "New byte #%d: 0X%02X", this->data_len_, currentByte);
        this->packet.data[this->data_len_] = currentByte;
      } else {
        ESP_LOGW(TAG_DECODING, "Frame overflow %d/%d. New byte: 0X%2X", this->data_len_, sizeof(this->packet.data),
                 currentByte);
      }
      bit_current_index = 0;
      currentByte = 0;
      this->data_len_++;
    }
  }

  /**
   * @brief Starts a new frame.
   */
  void start_new_frame() {
    reset("Start of new frame");
    started = true;
  }
  static int32_t get_high_duration(const rmt_item32_t *item) {
    return item->level0 ? item->duration0 : item->level1 ? item->duration1 : 0;
  }
  static uint32_t get_low_duration(const rmt_item32_t *item) {
    return !item->level0 ? item->duration0 : !item->level1 ? item->duration1 : 0;
    ;
  }
  static bool matches_duration(uint32_t target_us, uint32_t actual_us) {
    return actual_us >= (target_us - pulse_duration_threshold_us) &&
           actual_us <= (target_us + pulse_duration_threshold_us);
  }
  static bool is_start_frame(const rmt_item32_t *item) {
    // return matches_duration(get_low_duration(item), frame_heading_low_duration_ms * 1000) &&
    return matches_duration(get_high_duration(item), frame_heading_high_duration_ms * 1000);
  }

  static bool is_long_bit(const rmt_item32_t *item) {
    return matches_duration(get_high_duration(item), bit_long_high_duration_ms * 1000) &&
           matches_duration(get_low_duration(item), bit_low_duration_ms * 1000);
  }

  static bool is_short_bit(const rmt_item32_t *item) {
    return matches_duration(get_high_duration(item), bit_short_high_duration_ms * 1000) &&
           matches_duration(get_low_duration(item), bit_low_duration_ms * 1000);
  }
  /**
   * @brief Checks if a frame has started.
   *
   * @return true If a frame has started.
   * @return false Otherwise.
   */
  bool is_started() const { return started; }

  /**
   * @brief Sets the started state of a frame.
   *
   * @param value The started state of a frame.
   */
  void set_started(bool value) { started = value; }
  void debug(const char *msg = "") {
#ifdef USE_LOGGER
    auto *log = logger::global_logger;
    bool debug_status = (log != nullptr && log->level_for(TAG_DECODING) >= ESPHOME_LOG_LEVEL_DEBUG);
#else
    bool debug_status = false;
#endif

    if (this->data_len_ == 0 || strlen(msg) == 0)
      return;

    BaseFrame inv_bf = BaseFrame(*this);
    inv_bf.inverseLevels();

    ESP_LOGV(TAG_DECODING,
             "%s%s packet. data_len: %d, currentByte: %d, bit_current_index: %d, "
             "started: %s, finalized: %s, passes: %d, checksum: %d?=%d, inv checksum: %d?=%d",
             msg, this->source_string(), this->data_len_, currentByte, bit_current_index,
             started ? "STARTED" : "NOT STARTED", finalized ? "FINALIZED" : "NOT FINALIZED", passes_count,
             this->get_checksum_byte(), calculateChecksum(), inv_bf.get_checksum_byte(), inv_bf.calculateChecksum());
    if (debug_status && (is_size_valid())) {
      debug_print_hex();
      inv_bf.debug_print_hex();
    }
  }

  inline bool is_complete() const { return this->started && (this->is_size_valid()) && is_checksum_valid(); }
  uint32_t passes_count;
  void is_changed(const BaseFrame &frame);

 protected:
 private:
  uint8_t currentByte;        ///< The current byte being processed.
  uint8_t bit_current_index;  ///< The current bit index in the byte.
  bool started;               ///< Indicates if a frame has started.
  bool finalized;             ///< Indicates if the frame has been finalized.
};

}  // namespace hayward_pool_heater
}  // namespace esphome
