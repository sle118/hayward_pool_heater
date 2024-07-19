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

#include "HPUtils.h"
namespace esphome {
namespace hayward_pool_heater {

// constexpr size_t MAX_FRAME_LENGTH = 40;

// Frame Types and Positions
// constexpr uint8_t FRAME_TYPE_TEMP_OUT = 0x4B;   // B01001011 (inv: 11010010)
// constexpr uint8_t FRAME_TYPE_TEMP_IN = 0x8B;    // B10001011 (inv: 11010001)
// constexpr uint8_t FRAME_TYPE_PROG_TEMP = 0x81;  // B10000001 (inv: 10000001)
// constexpr uint8_t FRAME_POS_TYPE = 0;
// constexpr uint8_t FRAME_POS_TEMPOUT = 4;



// extern const uint8_t heater_packet_length_bytes;
// extern const uint32_t controler_group_spacing_ms;
// extern const uint32_t controler_frame_spacing_duration_ms;
// extern const uint32_t heater_frame_spacing_duration_ms;
// extern const uint32_t frame_heading_low_duration_ms;
// extern const uint32_t frame_heading_high_duration_ms;
// extern const uint32_t bit_short_high_duration_ms;
// extern const uint32_t bit_long_high_duration_ms;
// extern const uint32_t bit_low_duration_ms;

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
      : BaseFrame(),
        frameIndex(0),
        currentByte(0),
        bit_current_index(0),
        lastHighStart(0),
        lastLowStart(0),
        lastHighDuration(0),
        lastLowDuration(0),
        started(false),
        finalized(false) {}

  /**
   * @brief Copy constructor for DecodingStateFrame.
   *
   * @param other The DecodingStateFrame object to copy from.
   */
  DecodingStateFrame(const DecodingStateFrame& other)
      : BaseFrame(other),
        frameIndex(other.frameIndex),
        currentByte(other.currentByte),
        bit_current_index(other.bit_current_index),
        lastHighStart(other.lastHighStart),
        lastLowStart(other.lastLowStart),
        lastHighDuration(other.lastHighDuration),
        lastLowDuration(other.lastLowDuration),
        started(other.started),
        finalized(other.finalized) {}

  /**
   * @brief Copy assignment operator for DecodingStateFrame.
   *
   * @param other The DecodingStateFrame object to copy from.
   * @return Reference to the assigned DecodingStateFrame object.
   */
  DecodingStateFrame& operator=(const DecodingStateFrame& other) {
    if (this != &other) {
      BaseFrame::operator=(other);
      frameIndex = other.frameIndex;
      currentByte = other.currentByte;
      bit_current_index = other.bit_current_index;
      lastHighStart = other.lastHighStart;
      lastLowStart = other.lastLowStart;
      lastHighDuration = other.lastHighDuration;
      lastLowDuration = other.lastLowDuration;
      started = other.started;
      finalized = other.finalized;
    }
    return *this;
  }

  /**
   * @brief Resets the current frame state.
   */
  void reset() {
    frameIndex = 0;
    bit_current_index = 0;
    currentByte = 0;
    lastHighDuration = 0;
    lastLowDuration = 0;
    started = false;
    finalized = false;
    source_ = SOURCE_UNKNOWN;
    buffer.clear();
  }

  /**
   * @brief Finalizes the current frame by validating the checksum.
   *
   * @return true If the frame is valid.
   * @return false Otherwise.
   */
  bool finalize() {
    if (frameIndex == 0) {
      return false;
    }
    if (!validateChecksum()) {
      inverseLevels();
      if (validateChecksum()) {
        source_ = SOURCE_HEATER;
        finalized = true;
        return true;
      }
    } else {
      source_ = SOURCE_CONTROLLER;
      finalized = true;
      return true;
    }
    source_ = SOURCE_UNKNOWN;
    return false;
  }

  /**
   * @brief Checks if the frame is valid.
   *
   * @return true If the frame is valid.
   * @return false Otherwise.
   */
  bool is_valid() const {
    return (source_ != SOURCE_UNKNOWN && finalized && BaseFrame::is_valid());
  }

  /**
   * @brief Appends a bit to the current byte in the frame.
   *
   * @param long_duration Indicates if the bit represents a long duration.
   */
  void append_bit(bool long_duration) {
    if (!started) {
      return;
    }
    if (long_duration) {
      set_bit(&currentByte, bit_current_index);
    }
    bit_current_index++;
    if (bit_current_index == 8) {
      buffer.push_back(currentByte);
      bit_current_index = 0;
      currentByte = 0;
    }
  }

  /**
   * @brief Starts a new frame.
   */
  void start_new_frame() {
    reset();
    started = true;
  }

  /**
   * @brief Checks if the current frame is a start frame.
   *
   * @return true If the current frame is a start frame.
   * @return false Otherwise.
   */
  bool is_start_frame() const {
    return last_low_duration_matches(frame_heading_low_duration_ms) &&
           last_high_duration_matches(frame_heading_high_duration_ms);
  }

  /**
   * @brief Checks if the pulse represents a long bit.
   *
   * @return true If the pulse is a long bit.
   * @return false Otherwise.
   */
  inline bool is_long_bit() const { return last_high_duration_matches(bit_long_high_duration_ms); }

  /**
   * @brief Checks if the pulse represents a short bit.
   *
   * @return true If the pulse is a short bit.
   * @return false Otherwise.
   */
  inline bool is_short_bit() const {
    return last_high_duration_matches(bit_short_high_duration_ms);
  }

  /**
   * @brief Gets the duration of the last high pulse.
   *
   * @return The duration of the last high pulse in microseconds.
   */
  uint32_t get_last_high_duration() const { return lastHighDuration; }

  /**
   * @brief Sets the duration of the last high pulse.
   *
   * @param duration The duration of the last high pulse in microseconds.
   */
  void set_last_high_duration(uint32_t duration) { lastHighDuration = duration; }

  /**
   * @brief Gets the duration of the last low pulse.
   *
   * @return The duration of the last low pulse in microseconds.
   */
  uint32_t get_last_low_duration() const { return lastLowDuration; }

  /**
   * @brief Sets the duration of the last low pulse.
   *
   * @param duration The duration of the last low pulse in microseconds.
   */
  void set_last_low_duration(uint32_t duration) { lastLowDuration = duration; }

  /**
   * @brief Gets the timestamp of the last high start.
   *
   * @return The timestamp of the last high start in microseconds.
   */
  uint32_t get_last_high_start() const { return lastHighStart; }

  /**
   * @brief Sets the timestamp of the last high start.
   *
   * @param timestamp The timestamp of the last high start in microseconds.
   */
  void set_last_high_start(uint32_t timestamp) { lastHighStart = timestamp; }

  /**
   * @brief Gets the timestamp of the last low start.
   *
   * @return The timestamp of the last low start in microseconds.
   */
  uint32_t get_last_low_start() const { return lastLowStart; }

  /**
   * @brief Sets the timestamp of the last low start.
   *
   * @param timestamp The timestamp of the last low start in microseconds.
   */
  void set_last_low_start(uint32_t timestamp) { lastLowStart = timestamp; }

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

 private:
  uint8_t frameIndex;         ///< Index of the current frame.
  uint8_t currentByte;           ///< The current byte being processed.
  uint8_t bit_current_index;     ///< The current bit index in the byte.
  uint32_t lastHighStart;     ///< Timestamp of the last high start.
  uint32_t lastLowStart;      ///< Timestamp of the last low start.
  uint32_t lastHighDuration;  ///< Duration of the last high pulse.
  uint32_t lastLowDuration;   ///< Duration of the last low pulse.
  bool started;               ///< Indicates if a frame has started.
  bool finalized;             ///< Indicates if the frame has been finalized.

  /**
   * @brief Checks if the last high duration matches the target duration.
   *
   * @param target_duration_ms The target duration in milliseconds.
   * @return true If the last high duration matches the target duration.
   * @return false Otherwise.
   */
  inline bool last_high_duration_matches(uint32_t target_duration_ms) const {
    auto target_duration_us = target_duration_ms * 1000;
    return lastHighDuration >= (target_duration_us - pulse_duration_threshold_us) &&
           lastHighDuration <= (target_duration_us + pulse_duration_threshold_us);
  }

  /**
   * @brief Checks if the last low duration matches the target duration.
   *
   * @param target_duration_ms The target duration in milliseconds.
   * @return true If the last low duration matches the target duration.
   * @return false Otherwise.
   */
  inline bool last_low_duration_matches(uint32_t target_duration_ms) const {
    auto target_duration_us = target_duration_ms * 1000;
    return lastLowDuration >= (target_duration_us - pulse_duration_threshold_us) &&
           lastLowDuration <= (target_duration_us + pulse_duration_threshold_us);
  }

  /**
   * @brief Inverts the levels of the frame buffer.
   */
  void inverseLevels() {
    for (int i = 0; i < frameIndex; i++) {
      buffer[i] = ~buffer[i];
    }
  }
};

}  // namespace hayward_pool_heater
}