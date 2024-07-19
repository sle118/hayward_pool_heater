/**
 * @file base_frame.h
 * @brief Defines the BaseFrame class for handling frame data in the Heat Pump Controller.
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
#include "esphome/core/log.h"
#include <algorithm>
#include <cstring>  // for std::memcpy
#include <string>
#include <vector>

namespace esphome {
namespace hayward_pool_heater {

/**
 * @enum frame_source_t
 * @brief Enum representing the source of the frame.
 */
typedef enum { SOURCE_UNKNOWN, SOURCE_HEATER, SOURCE_CONTROLLER, SOURCE_LOCAL } frame_source_t;

/**
 * @enum frame_type_t
 * @brief Enum representing the type of the frame.
 */
typedef enum { FRAME_TYPE_UNKNOWN, FRAME_TEMP_OUT, FRAME_TEMP_IN, FRAME_TEMP_PROG } frame_type_t;

static constexpr uint8_t max_frame_length = 40;
static constexpr uint16_t pulse_duration_threshold_us = 600;
static constexpr uint32_t frame_heading_low_duration_ms = 9;
static constexpr uint32_t frame_heading_high_duration_ms = 5;
static constexpr uint32_t bit_long_high_duration_ms = 3;
static constexpr uint32_t bit_low_duration_ms = 1;
static constexpr uint32_t bit_short_high_duration_ms = bit_low_duration_ms;
static constexpr uint32_t frame_end_threshold_ms = 50;
static constexpr uint32_t controler_group_spacing_ms = 2 * 1000;
static constexpr uint32_t controler_frame_spacing_duration_ms = 101;
static constexpr uint32_t heater_frame_spacing_duration_ms = 152;
static constexpr uint32_t delay_between_controller_messages_ms = 60 * 1000;

static constexpr uint32_t frame_heading_total_duration_ms =
    frame_heading_low_duration_ms + frame_heading_high_duration_ms;

// Frame Types and Positions
static constexpr uint8_t FRAME_TYPE_TEMP_OUT = 0x4B;   // B01001011 (inv: 11010010)
static constexpr uint8_t FRAME_TYPE_TEMP_IN = 0x8B;    // B10001011 (inv: 11010001)
static constexpr uint8_t FRAME_TYPE_PROG_TEMP = 0x81;  // B10000001 (inv: 10000001)
static constexpr uint8_t FRAME_POS_TYPE = 0;
static constexpr uint8_t FRAME_POS_TEMPOUT = 4;
static constexpr uint8_t FRAME_POS_target_temperature = 4;
static constexpr uint8_t FRAME_POS_TEMPIN = 9;
static constexpr uint8_t FRAME_POS_MODE = 2;
static constexpr uint8_t heater_packet_length_bytes = 12;

#define TAG_BF "BaseFrame"

/**
 * @class BaseFrame
 * @brief A class to handle the basic frame operations in the Heat Pump Controller.
 */
class BaseFrame {
 public:
  /**
   * @brief Default constructor. Initializes the buffer and sets the source to unknown.
   */
  BaseFrame() : buffer(max_frame_length), source_(SOURCE_UNKNOWN) {}

  /**
   * @brief Copy constructor.
   * @param other The BaseFrame object to copy from.
   */
  BaseFrame(const BaseFrame& other) : buffer(other.buffer), source_(other.source_) {}

  /**
   * @brief Copy assignment operator.
   * @param other The BaseFrame object to copy from.
   * @return Reference to the assigned BaseFrame object.
   */
  BaseFrame& operator=(const BaseFrame& other) {
    if (this != &other) {
      buffer = other.buffer;
      source_ = other.source_;
    }
    return *this;
  }

  /**
   * @brief Template copy assignment operator to accept an array of unsigned char of any length.
   * @param base_data The array to copy from.
   * @return Reference to the assigned BaseFrame object.
   */
  template <size_t N>
  BaseFrame& operator=(const unsigned char (&base_data)[N]) {
    // Clear the buffer and resize it to match the size of cmdTrame
    buffer.clear();
    buffer.resize(N);

    // Copy the elements from cmdTrame to the buffer
    std::copy(std::begin(base_data), std::end(base_data), buffer.begin());
    source_ = SOURCE_UNKNOWN;
    return *this;
  }

  /**
   * @brief Gets the frame type based on the buffer content.
   * @return The frame type.
   */
  frame_type_t get_frame_type() const {
    auto pos_value = buffer[FRAME_POS_TYPE];
    if (pos_value == FRAME_TYPE_TEMP_OUT) {
      return FRAME_TEMP_OUT;
    } else if (pos_value == FRAME_TYPE_TEMP_IN) {
      return FRAME_TEMP_IN;
    } else if (pos_value == FRAME_TYPE_PROG_TEMP) {
      return FRAME_TEMP_PROG;
    }
    return FRAME_TYPE_UNKNOWN;
  }

  /**
   * @brief Sets the checksum for the frame.
   */
  void set_checksum() {
    this->buffer[buffer.size() - 1] = calculateChecksum(buffer.data(), buffer.size() - 1);
  }

  /**
   * @brief Appends a checksum to the frame.
   */
  void append_checksum() {
    this->buffer.push_back(0);
    set_checksum();
  }

  /**
   * @brief Prints the buffer content in hexadecimal format.
   */
  void debug_print_hex() const {
    std::string hexString;
    for (size_t i = 0; i < buffer.size(); ++i) {
      char hex[4];  // 2 digits + "0x" + null terminator
      snprintf(hex, sizeof(hex), "0x%02X", buffer[i]);
      hexString += hex;
      if (i < buffer.size() - 1) {
        hexString += ", ";
      }
    }
    ESP_LOGD(TAG_BF, "%s: %s", source_string(), hexString.c_str());
  }

  /**
   * @brief Gets the source of the frame as a string.
   * @return The source as a string.
   */
  const char* source_string() const {
    switch (this->source_) {
      case SOURCE_CONTROLLER:
        return "CONTROLLER";
      case SOURCE_HEATER:
        return "HEATER";
      case SOURCE_LOCAL:
        return "LOCAL";
      default:
        return "UNKNOWN";
    }
  }

  /**
   * @brief Gets the frame type as a string.
   * @return The frame type as a string.
   */
  const char* type_string() {
    switch (this->get_frame_type()) {
      case FRAME_TEMP_OUT:
        return "TEMP_OUT";
      case FRAME_TEMP_IN:
        return "TEMP_IN";
      case FRAME_TEMP_PROG:
        return "TEMP_PROG";
      case FRAME_TYPE_UNKNOWN:
      default:
        return "UNKNOWN";
    }
  }

  /**
   * @brief Checks if the frame is valid.
   * @return true if the frame is valid, false otherwise.
   */
  bool is_valid() const { return (this->source_ != SOURCE_UNKNOWN); }

  /**
   * @brief Calculates the checksum of a buffer.
   * @param buffer The buffer to calculate the checksum for.
   * @param length The length of the buffer.
   * @return The calculated checksum.
   */
  static uint8_t calculateChecksum(const uint8_t* buffer, size_t length) {
    unsigned int total = 0;
    for (size_t i = 0; i < length; i++) {
      total += buffer[i];
    }
    uint8_t checksum = total % 256;
    return checksum;
  }

  /**
   * @brief Validates the checksum of the frame.
   * @return true if the checksum is valid, false otherwise.
   */
  bool validateChecksum() const {
    return (calculateChecksum(buffer.data(), buffer.size() - 1) == buffer[buffer.size() - 1]);
  }

  /**
   * @brief Reverses the order of bits in a byte.
   * @param x The byte to reverse.
   * @return The byte with reversed bits.
   */
  static uint8_t reverse_bits(unsigned char x) {
    x = ((x >> 1) & 0x55) | ((x << 1) & 0xaa);
    x = ((x >> 2) & 0x33) | ((x << 2) & 0xcc);
    x = ((x >> 4) & 0x0f) | ((x << 4) & 0xf0);
    return x;
  }

  /**
   * @brief Prints the frame content.
   */
  void print() const {
    if (esp_log_level_get(TAG_BF) == ESP_LOG_DEBUG) {
      // Calculate the buffer size needed
      size_t buffer_size =
          buffer.size() * 5 +
          20;  // 5 chars per byte (including comma and space) + extra for header info
      char* output_buffer = new char[buffer_size];

      // Print the header information
      int offset =
          snprintf(output_buffer, buffer_size, "[%s/%zu] ", source_string(), buffer.size());

      // Print the buffer contents
      for (size_t i = 0; i < buffer.size(); ++i) {
        offset += snprintf(output_buffer + offset, buffer_size - offset, "%02X", buffer[i]);
        if (i < buffer.size() - 1) {
          offset += snprintf(output_buffer + offset, buffer_size - offset, ",");
        }
      }

      // Log the output
      ESP_LOGI(TAG_BF, "%s", output_buffer);

      // Free the allocated buffer
      delete[] output_buffer;
    }
  }

  /**
   * @brief Gets the size of the buffer.
   * @return The size of the buffer.
   */
  std::size_t size() const { return buffer.size(); }

  /**
   * @brief Gets a pointer to the buffer data.
   * @return A pointer to the buffer data.
   */
  uint8_t* data() { return buffer.data(); }

  /**
   * @brief Gets a const pointer to the buffer data.
   * @return A const pointer to the buffer data.
   */
  const uint8_t* data() const { return buffer.data(); }

  /**
   * @brief Accesses a buffer element by index.
   * @param index The index of the element.
   * @return A reference to the element.
   */
  uint8_t& operator[](std::size_t index) { return buffer[index]; }
  // /**
  //  * @brief Accesses a buffer element by index.
  //  * @param index The index of the element.
  //  * @return A pointer to the element.
  //  */
  // uint8_t* operator[](std::size_t index) { return &buffer[index]; }  

  /**
   * @brief Accesses a buffer element by index.
   * @param index The index of the element.
   * @return A const reference to the element.
   */
  const uint8_t& operator[](std::size_t index) const { return buffer[index]; }

  /**
   * @brief Gets the source of the frame.
   * @return The source of the frame.
   */
  frame_source_t get_source() { return this->source_; }

 protected:
  std::vector<uint8_t> buffer;  ///< Buffer to hold frame data.
  frame_source_t source_;    ///< Source of the frame.
};

}  // namespace hayward_pool_heater
}