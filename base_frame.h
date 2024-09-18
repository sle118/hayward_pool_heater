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
#include <stdint.h>
// #include <cstring>  // for std::memcpy
#include <string>
#include <sstream>
#include <iomanip>
// #include <algorithm>  // for std::copy
#include <bitset>
#include "stddef.h"
#include "esphome/core/log.h"
#include "esphome/components/logger/logger.h"
#include <driver/rmt.h>
#include "HPUtils.h"

extern char *esp_log_system_timestamp(void);
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
typedef enum {
  FRAME_TYPE_UNKNOWN,
  FRAME_TEMP_OUT,
  FRAME_TEMP_OUT_CONT,
  FRAME_TEMP_IN,
  FRAME_TEMP_IN_B,
  FRAME_TEMP_PROG,
  FRAME_CLOCK,
  FRAME_UNKNOWN_82,
  FRAME_UNKNOWN_CA,
  FRAME_UNKNOWN_83,
  FRAME_UNKNOWN_84,
  FRAME_UNKNOWN_85,
  FRAME_UNKNOWN_86,
  FRAME_UNKNOWN_DD
} frame_type_t;

static constexpr uint16_t pulse_duration_threshold_us = 600;
static constexpr uint32_t frame_heading_low_duration_ms = 9;
static constexpr uint32_t frame_heading_high_duration_ms = 5;
static constexpr uint32_t bit_long_high_duration_ms = 3;
static constexpr uint32_t bit_low_duration_ms = 1;
static constexpr uint32_t bit_short_high_duration_ms = bit_low_duration_ms;
static constexpr uint32_t frame_end_threshold_ms = 50;  // spacing between each frame is 100ms.
static constexpr uint32_t controler_group_spacing_ms = 500;
static constexpr uint32_t controler_frame_spacing_duration_ms = 100;
static constexpr uint32_t delay_between_sending_messages_ms = 10 * 1000;  // restrict changes to once per 10 seconds
// static constexpr uint32_t heater_frame_spacing_duration_ms = 152;
static constexpr uint32_t delay_between_controller_messages_ms = 60 * 1000;
static constexpr uint32_t packet_expiry_time_s = 120;
static constexpr const char TAG_BF[] = "hayward_pool_heater.baseframe";
static constexpr const char TAG_BF_HEX[] = "hayward_pool_heater.hex";
static constexpr const char TAG_BF_FULL[] = "hayward_pool_heater.baseframe.debug";

static constexpr uint32_t frame_heading_total_duration_ms =
    frame_heading_low_duration_ms + frame_heading_high_duration_ms;
static constexpr uint8_t frame_data_length = 12;
static constexpr uint8_t frame_data_length_short = 9;

// Frame Types and Positions
constexpr uint8_t FRAME_TYPE_TEMP_OUT = 0xD2;   // B 01001011 (inv: 11010010)
constexpr uint8_t FRAME_TYPE_TEMP_IN = 0xD1;    // B 11010001 (inv: 10001011)
constexpr uint8_t FRAME_TYPE_PROG_TEMP = 0x81;  // B 10000001 (inv: 10000001)
constexpr uint8_t FRAME_TYPE_CLOCK = 0xCF;      // Clock
constexpr uint8_t FRAME_TYPE_UNKNOWN_CA = 0xCA;
constexpr uint8_t FRAME_TYPE_UNKNOWN_82 = 0x82;
constexpr uint8_t FRAME_TYPE_UNKNOWN_83 = 0x83;
constexpr uint8_t FRAME_TYPE_UNKNOWN_84 = 0x84;
constexpr uint8_t FRAME_TYPE_UNKNOWN_85 = 0x85;
constexpr uint8_t FRAME_TYPE_UNKNOWN_86 = 0x86;
constexpr uint8_t FRAME_TYPE_UNKNOWN_DD = 0xDD;

static constexpr unsigned char model_cmd[12] = {
    FRAME_TYPE_PROG_TEMP,  // 0 - HEADER
    0xB1,                  // LE: B10001101, BE: B10110001
    0x17,                  // LE: B11101000, BE: B00010111  POWER &MODE
    0x06,
    0x77,  // 4 - TEMP. LE: B11101110, BE: 01110111
    0x78,  // LE: 00011110, BE: 01111000
    0x3D,  // LE: 10111100, BE: 00111101
    0x3D,
    0x3D,
    0x3D,
    0,
    0,  // 11 - CHECKSUM
};

typedef struct {
  uint8_t power : 1;
  uint8_t unknown_1 : 1;
  uint8_t unknown_2 : 1;
  uint8_t unknown_3 : 1;
  uint8_t heat : 1;
  uint8_t auto_mode : 1;
  uint8_t unknown_4 : 1;
  uint8_t unknown_5 : 1;
} __attribute__((packed)) hp_mode_t;

// Define the temperature structure with bitfields
typedef struct {
  union {
    struct {
      uint8_t decimal : 1;
      uint8_t integer : 5;
      uint8_t unknown_1 : 1;
      uint8_t unknown_2 : 1;
    };
    uint8_t raw;
  };
} __attribute__((packed)) temperature_t;

struct bits_details_t {
  union {
    struct {
      uint8_t unknown_1 : 1;
      uint8_t unknown_2 : 1;
      uint8_t unknown_3 : 1;
      uint8_t unknown_4 : 1;
      uint8_t unknown_5 : 1;
      uint8_t unknown_6 : 1;
      uint8_t unknown_7 : 1;
      uint8_t unknown_8 : 1;
    };
    uint8_t raw;
  };

  bool operator==(const bits_details_t &other) const { return raw == other.raw; }

  bool operator!=(const bits_details_t &other) const { return !(*this == other); }
};

typedef struct {
  bits_details_t reserved_1;
  bits_details_t reserved_2;
  temperature_t temperature;  // Temperature byte at data[4]
  temperature_t temperature_exhaust;
  temperature_t temperature_coil;
  bits_details_t reserved_5;
  temperature_t temperature_4;
  bits_details_t reserved_7;
  bits_details_t reserved_8;
} __attribute__((packed)) tempout_t;
typedef struct {
  bits_details_t reserved_1;
  bits_details_t reserved_2;
  bits_details_t reserved_3;
  bits_details_t reserved_4;
  bits_details_t reserved_5;
  bits_details_t reserved_6;
  bits_details_t reserved_7;
  temperature_t temperature;  // Temperature byte at data[9]
  bits_details_t reserved_8;
} __attribute__((packed)) tempin_t;
// typedef struct {
//   bits_details_t reserved_1;
//   struct  {
//   union {
//     struct {
//       uint8_t fan_running : 1;
//       uint8_t reserved_2_2 : 1;
//       uint8_t reserved_2_3 : 1;
//       uint8_t reserved_2_4 : 1;
//       uint8_t reserved_2_5 : 1;
//       uint8_t reserved_2_6 : 1;
//       uint8_t reserved_2_7 : 1;
//       uint8_t reserved_2_8 : 1;
//     };
//     uint8_t raw;
//   };
//   } reserved_2;
//   bits_details_t reserved_3;
//   bits_details_t reserved_4;
//   bits_details_t reserved_5;
//   bits_details_t reserved_6;
//   bits_details_t reserved_7;
//   temperature_t temperature;  // Temperature byte at data[9]
//   bits_details_t reserved_8;
// } __attribute__((packed)) tempin_b_t;
typedef struct {
  hp_mode_t mode;  // Mode structure at data[2]
  temperature_t set_point_1;
  temperature_t set_point_2;  // Temperature byte at data[4]
  bits_details_t reserved_2;
  bits_details_t reserved_3;
  bits_details_t reserved_4;
  bits_details_t reserved_5;
  bits_details_t reserved_6;
  bits_details_t reserved_7;
} __attribute__((packed)) tempprog_t;

// Packet Breakdown: CF,B1,00,00,14,07,1F,17,2C,00,00,FD
// -----------------------------------------------------------------------------
// | CF | B1 | 00,00 | 14,07 | 1F | 17 | 2C | 00,00 | FD |
// |    |    |       |       |    |    |    |       |    |
// |    |    |       |       |    |    |    |       |    +--> Checksum byte (FD)
// |    |    |       |       |    |    |    |       +------> Reserved or padding bytes (00,00)
// |    |    |       |       |    |    |    +-------------> Minute value (2C)
// |    |    |       |       |    |    +------------------> Hour value (17)
// |    |    |       |       |    +-----------------------> Day value (1F, which is 31 in decimal)
// |    |    |       |       +----------------------------> Month value (07, which is July)
// |    |    |       +------------------------------------> Year or unknown identifier (14)
// |    |    +--------------------------------------------> Possibly reserved or another ID (00,00)
// |    +-------------------------------------------------> Frame type or command type identifier (B1)
// +------------------------------------------------------> Packet header (CF)
// -----------------------------------------------------------------------------

// Explanation:
// CF,B1: These bytes likely represent a fixed header or frame type identifier.
// 00,00: These bytes may be reserved or hold some other ID that doesn't change frequently.
// 14,07: These bytes could represent the year (14, which could correspond to 2020 if offset by 6) and the month (07,
// July). 1F: This byte likely represents the day of the month (1F is 31 in hexadecimal). 17: This byte likely
// represents the hour in 24-hour format (17 is 23 in decimal, which corresponds to 11 PM). 2C: This byte likely
// represents the minute within the hour (2C is 44 in decimal). 00,00: These might be padding bytes or reserved for
// future use. FD: This byte is the checksum, calculated from the other bytes in the packet.

// data content of clock frames. Calendar days may not be relevant as on many units, it is not possible
// to adjust date/time.
typedef struct {
  uint8_t reserved1;  // Reserved or ID byte (00)
  uint8_t reserved2;  // Reserved or ID byte (00)
  uint8_t year;       // Year or identifier byte (14)
  uint8_t month;      // Month (07)
  uint8_t day;        // Day (1F)
  uint8_t hour;       // Hour (17)
  uint8_t minute;     // Minute (2C)
  uint8_t reserved3;
  uint8_t reserved4;
} __attribute__((packed)) clock_time_t;

// data content of short frames, i.e. frames with 9 bytes
// for which analysis is not yet complete
typedef struct {
  bits_details_t reserved_1;
  bits_details_t reserved_2;
  bits_details_t reserved_3;
  bits_details_t reserved_4;
  bits_details_t reserved_5;
  bits_details_t reserved_6;
  bits_details_t reserved_7;
} __attribute__((packed)) short_packet_t;
;

// Main packet data structure (12 bytes)
// consisting of all known packet data types
typedef struct {
  union {
    uint8_t data[frame_data_length];  // Raw data array
    union {
      struct {
        uint8_t frame_type;  // Common field: frame type
        uint8_t reserved;
        union {
          tempout_t tempout;
          tempin_t tempin;
          // tempin_b_t tempinb;
          tempprog_t tempprog;
          clock_time_t clock;  // New clock packet structure
        };
        uint8_t checksum;  // Checksum byte at data[11]
      };
      struct {
        uint8_t frame_type;  // Common field: frame type
        union {
          short_packet_t bits_details;
        };
        uint8_t checksum;  // Checksum byte at data[8] for short frames
      } short_frame;
    };
  };
} __attribute__((packed)) hp_packetdata_t;

// Verify packet data size in order to prevent misalignment of struct
static_assert(sizeof(hp_packetdata_t) == 12, "Invalid packet data size");

class BaseFrame {
 public:
  /**
   * @brief Constructs a new BaseFrame object.
   *
   * @details Initializes the BaseFrame object with the default values:
   *   - source_ is set to SOURCE_UNKNOWN
   *   - data_len_ is set to 0
   *   - packet is cleared with memset
   *   - frame_time_ms_ is set to the current millisecond value
   */
  BaseFrame() : source_(SOURCE_UNKNOWN), data_len_(0) {
    std::memset(&packet, 0, sizeof(packet));
    this->frame_time_ms_ = millis();
    ESP_LOGVV(TAG_BF_FULL, "New BaseFrame instance created with frame time: %d ", this->frame_time_ms_);
  }

  /**
   * @brief Copy constructor for BaseFrame.
   *
   * @param other The BaseFrame object to copy from.
   *
   * This constructor creates a new BaseFrame object from a const reference to another BaseFrame object.
   * The new object will have the same data as the original object, but will be a separate entity.
   */
  BaseFrame(const BaseFrame &other)
      : packet(other.packet), source_(other.source_), data_len_(other.data_len_), frame_time_ms_(other.frame_time_ms_) {
    ESP_LOGVV(TAG_BF_FULL, "New BaseFrame from other const reference created with frame time: %d ",
              this->frame_time_ms_);
  }

  /**
   * @brief Compares two BaseFrame objects to check if they contain the same data.
   *
   * @param other The BaseFrame object to compare with.
   * @return True if the two BaseFrame objects contain the same data, false otherwise.
   *
   * This operator compares the frame type, raw data bytes, source identifier, and data length of the two
   * BaseFrame objects. If all of these values are equal, then the two objects are considered equal.
   *
   * frame time is ignored in the comparison in this context, as the comparator focuses on the data bytes 
   * rather than the object itself.
   */
  bool operator==(const BaseFrame &other) const {
    return (packet.frame_type == other.packet.frame_type &&
            std::memcmp(packet.data, other.packet.data, data_len_) == 0 && source_ == other.source_ &&
            data_len_ == other.data_len_);
  }

  /**
   * @brief Constructs a new BaseFrame from a static array of bytes.
   *
   * @param base_data The static array of bytes to construct the BaseFrame from.
   *
   * @details This constructor is used to create a BaseFrame from a static array of bytes.
   * The size of the array is used to determine the size of the packet to be constructed.
   * The array is copied into the packet data, and the frame time is set to the current
   * millisecond value.
   */
  template<size_t N> BaseFrame(const unsigned char (&base_data)[N]) {
    static_assert(N <= sizeof(this->packet.data), "Array size exceeds packet data size");
    std::memcpy(this->packet.data, base_data, N);
    this->data_len_ = N;
    this->frame_time_ms_ = millis();
    this->source_ = SOURCE_UNKNOWN;  // Assuming SOURCE_UNKNOWN is a valid identifier
    ESP_LOGVV(TAG_BF_FULL, "New BaseFrame from data buffer[N] created with frame time: %d ", this->frame_time_ms_);
  }

/**
 * @brief Copy assignment operator.
 *
 * @param other The BaseFrame to copy.
 */ 
  BaseFrame &operator=(const BaseFrame &other) {
    if (this != &other || this->frame_time_ms_ != other.frame_time_ms_) {
      // the equality operator doesn't consider frame_time in its logic,
      // as it is written to check if the data was changed over time
      this->packet = other.packet;
      this->source_ = other.source_;
      this->data_len_ = other.data_len_;
      this->frame_time_ms_ = other.frame_time_ms_;
    }
    ESP_LOGVV(TAG_BF_FULL, "New BaseFrame operator '=' from other const reference created with frame time: %d ",
              this->frame_time_ms_);
    return *this;
  }

  /**
   * @brief Copy constructor.
   *
   * @param other The BaseFrame to copy.
   */
  template<size_t N> BaseFrame &operator=(const unsigned char (&base_data)[N]) {
    static_assert(N <= sizeof(this->packet.data), "Array size exceeds packet data size");
    std::memcpy(this->packet.data, base_data, N);
    this->source_ = SOURCE_UNKNOWN;
    this->data_len_ = N;
    this->frame_time_ms_ = millis();
    ESP_LOGVV(TAG_BF_FULL, "New BaseFrame from operator '=' with data buffer[N] created with frame time: %d ",
              this->frame_time_ms_);
    return *this;
  }

  frame_type_t get_frame_type() const { return get_frame_type(*this); }

  /**
   * @brief Get the frame type of a BaseFrame.
   *
   * @param[in] frame The BaseFrame to get the frame type from.
   * @return The frame type as a frame_type_t enum.
   *
   * This function takes a BaseFrame and returns the frame type as a frame_type_t enum.
   * It does this by switching on the frame type stored in the BaseFrame, and returning
   * the appropriate enum value.
   */
  static frame_type_t get_frame_type(const BaseFrame &frame) {
    switch (frame.packet.frame_type) {
      case FRAME_TYPE_TEMP_OUT:
        return (frame.size() == frame_data_length_short) ? FRAME_TEMP_OUT_CONT : FRAME_TEMP_OUT;
      case FRAME_TYPE_TEMP_IN:
        return frame.packet.tempin.reserved_1.raw == 0x05 ? FRAME_TEMP_IN : FRAME_TEMP_IN_B;
      case FRAME_TYPE_PROG_TEMP:
        return FRAME_TEMP_PROG;
      case FRAME_TYPE_CLOCK:
        return FRAME_CLOCK;
      case FRAME_TYPE_UNKNOWN_82:
        return FRAME_UNKNOWN_82;
      case FRAME_TYPE_UNKNOWN_83:
        return FRAME_UNKNOWN_83;
      case FRAME_TYPE_UNKNOWN_84:
        return FRAME_UNKNOWN_84;
      case FRAME_TYPE_UNKNOWN_85:
        return FRAME_UNKNOWN_85;
      case FRAME_TYPE_UNKNOWN_86:
        return FRAME_UNKNOWN_86;
      case FRAME_TYPE_UNKNOWN_DD:
        return FRAME_UNKNOWN_DD;
      case FRAME_TYPE_UNKNOWN_CA:
        return FRAME_UNKNOWN_CA;

      default:
        return FRAME_TYPE_UNKNOWN;
    }
  }

  /**
   * @brief Checks if the frame is a short frame.
   *
   * @return true If the frame is a short frame.
   * @return false Otherwise
   */
  bool is_short_frame() const { return this->data_len_ == frame_data_length_short; }

  /**
   * @brief Checks if the frame is a long frame.
   *
   * @return true If the frame is a long frame.
   * @return false Otherwise
   */
  bool is_long_frame() const { return this->data_len_ == frame_data_length; }

  /**
   * @brief Returns the data length of the frame.
   *
   * Returns the data length of the frame, taking into account whether the frame is a short frame or not.
   *
   * @return The data length of the frame
   */
  size_t get_data_len() const { return this->data_len_; }
  /**
   * @brief Returns the checksum byte of the frame.
   *
   * Returns the checksum byte of the frame, taking into account whether the frame is a short frame or not.
   *
   * @return The checksum byte of the frame
   */
  uint8_t get_checksum_byte() const { return get_checksum_byte_val(*this); }

  /**
   * @brief Returns a reference to the checksum byte of the frame.
   *
   * Returns a reference to the checksum byte of the frame, taking into account whether the frame is a short frame or
   * not.
   *
   * @param frame The frame to get the checksum from
   * @return A reference to the checksum byte of the frame
   */
  static uint8_t &get_checksum_byte_ref(BaseFrame &frame) {
    return (frame.is_short_frame()) ? frame.packet.short_frame.checksum : frame.packet.checksum;
  }

  /**
   * @brief Returns the checksum byte of the frame.
   *
   * Returns the checksum byte of the frame, taking into account whether the frame is a short frame or not.
   *
   * @param frame The frame to get the checksum from
   * @return The checksum byte of the frame
   */
  static uint8_t get_checksum_byte_val(const BaseFrame &frame) {
    return (frame.is_short_frame()) ? frame.packet.short_frame.checksum : frame.packet.checksum;
  }
  

  
  /**
   * @brief Calculates and sets the checksum for the current frame.
   *
   * Calculates the checksum of the current frame and sets it in the packet.
   */
  void set_checksum() { 
    get_checksum_byte_ref(*this) = calculateChecksum(this->packet.data, this->data_len_); 
    }
  
  /**
   * @brief Debug print a buffer of bytes in hex format.
   * @param buffer The buffer to print.
   * @param length The length of the data in the buffer.
   * @param source The source of the frame.
   */
  template<size_t N>
  static void debug_print_hex(const uint8_t (&buffer)[N], const size_t length, const frame_source_t source) {
    std::string hexString;
    for (size_t i = 0; i < sizeof(buffer) && i < length; ++i) {
      char hex[10];
      snprintf(hex, sizeof(hex), "0x%02X", buffer[i]);
      hexString += hex;
      if (i < length - 1) {
        hexString += ", ";
      }
    }
    ESP_LOGV(TAG_BF_HEX, "%s(%zu): %s", source_string(source), length, hexString.c_str());
  }
  void debug_print_hex() const { debug_print_hex(this->packet.data, this->data_len_, this->source_); }

  static const char *source_string(frame_source_t source) {
    switch (source) {
      case SOURCE_CONTROLLER:
        return "CONT";
      case SOURCE_HEATER:
        return "HEAT";
      case SOURCE_LOCAL:
        return "LOC ";
      default:
        return "UNK ";
    }
  }
    /**
   * @brief Returns a string representation of the frame source.
   *
   * @return A string representation of the frame source.
   */ 
  const char *source_string() const { return source_string(this->source_); }

  /**
   * @brief Returns a string representation of the frame type.
   *
   * @return A string representation of the frame type.
   */ 
  const char *type_string() const {
    switch (this->get_frame_type()) {
      case FRAME_TEMP_OUT:
        return "TEMP_OUT  ";
      case FRAME_TEMP_OUT_CONT:
        return "TEMP_OUT_C";
      case FRAME_TEMP_IN:
        return "TEMP_IN   ";
      case FRAME_TEMP_IN_B:
        return "TEMP_IN_B ";
      case FRAME_TEMP_PROG:
        return "TEMP_PROG ";
      case FRAME_CLOCK:
        return "CLOCK     ";
      case FRAME_UNKNOWN_CA:
        return "UNK_CA    ";
      case FRAME_UNKNOWN_82:
        return "UNK_82    ";
      case FRAME_UNKNOWN_83:
        return "UNK_83    ";
      case FRAME_UNKNOWN_84:
        return "UNK_84    ";
      case FRAME_UNKNOWN_85:
        return "UNK_85    ";
      case FRAME_UNKNOWN_86:
        return "UNK_86    ";
      case FRAME_UNKNOWN_DD:
        return "UNK_DD    ";
      default:
        return "UNKNOWN   ";
    }
  }
  static inline bool log_active(const char *tag, int min_level = ESPHOME_LOG_LEVEL_VERBOSE) {
    auto *log = logger::global_logger;
    if (log == nullptr || log->level_for(tag) < min_level) {
      return false;
    }
    return true;
  }

  /**
   * @brief Inverts the levels of the frame buffer.
   */
  template<size_t N> static void inverseLevels(uint8_t (&buffer)[N], const size_t length) {
    if (log_active(TAG_BF_FULL)) {
      ESP_LOGD(TAG_BF_FULL, "Before inverting bytes");
      debug_print_hex(buffer, length, SOURCE_UNKNOWN);
    }
    for (unsigned long i = 0; i < length; i++) {
      buffer[i] = ~buffer[i];
    }
    if (log_active(TAG_BF_FULL)) {
      ESP_LOGD(TAG_BF_FULL, "After inverting bytes");
      debug_print_hex(buffer, length, SOURCE_UNKNOWN);
    }
  }
  void inverseLevels() { inverseLevels(this->packet.data, this->data_len_); }
  bool is_valid() const { return (this->source_ != SOURCE_UNKNOWN); }

  template<size_t N> static uint8_t calculateChecksum(const uint8_t (&buffer)[N], const size_t length) {
    unsigned int total = 0;

    // Ensure length is within bounds
    if (length > N) {
      ESP_LOGE(TAG_BF, "Length larger than buffer size (%zu>%lu)", length, N);
      return 0;  // Return 0 or some error indicator
    }

    // Start from index 1 if length == frame_data_length_short, otherwise start from 0
    size_t start_index = (length == frame_data_length_short) ? 1 : 0;

    // Loop over the range [start_index, length - 1)
    for (size_t i = start_index; i < length - 1; ++i) {
      total += buffer[i];
    }

    // Return the checksum (modulo 256)
    uint8_t checksum = total % 256;

    return checksum;
  }

  uint8_t calculateChecksum() { return calculateChecksum(this->packet.data, this->data_len_); }
  bool validateChecksum() const {
    // Calculate the checksum based on the packet data
    auto chksum = calculateChecksum(this->packet.data, this->data_len_);

    // Check if the calculated checksum matches the actual checksum
    bool result = (chksum == get_checksum_byte());

    if (!result) {
      ESP_LOGVV(TAG_BF, "Invalid checksum: calc 0x%02X != actual 0x%02X", chksum, get_checksum_byte());
    }

    return result;
  }

  bool is_checksum_valid() const {
    bool inverted = false;
    return is_checksum_valid(inverted);
  }
  bool is_checksum_valid(bool &inverted) const {
    // First, check the checksum with the original data
    inverted = false;
    if (validateChecksum()) {
      return true;
    }
    BaseFrame inv_bf = BaseFrame(*this);
    inv_bf.inverseLevels();
    if (inv_bf.validateChecksum()) {
      inverted = true;
      return true;
    }
    return false;
  }
  static void print_diff(const std::string &prefix, const BaseFrame &frame, const BaseFrame &reference,
                         const char *tag = TAG_BF, int min_level = ESPHOME_LOG_LEVEL_VERBOSE, int line = __LINE__) {
    if (!log_active(tag, min_level)) {
      return;
    }
    // Format the prefix to ensure consistent alignment
    char normalized_prefix[16];  // Assuming 15 chars + null terminator
    snprintf(normalized_prefix, sizeof(normalized_prefix), "%-5s", prefix.c_str());

    // Create a buffer for the frame data with the difference indication
    char output_buffer[5 * sizeof(frame.packet.data) + 20];  // Size calculation
    // int offset = snprintf(output_buffer, sizeof(output_buffer), "[%2zu] ", frame.data_len_);
    int offset = 0;
    for (size_t i = 0; i < sizeof(frame.packet.data); ++i) {
      if (i < frame.data_len_) {
        if (i == frame.data_len_ - 1 || frame.packet.data[i] == reference.packet.data[i]) {
          offset += snprintf(output_buffer + offset, sizeof(output_buffer) - offset, "XX");
        } else {
          offset += snprintf(output_buffer + offset, sizeof(output_buffer) - offset, "%02X", frame.packet.data[i]);
        }
        // if (i < frame.data_len_ - 1) {
        //   offset += snprintf(output_buffer + offset, sizeof(output_buffer) - offset, ",");
        // } else {
        //   offset += snprintf(output_buffer + offset, sizeof(output_buffer) - offset, " ");
        // }
      } else {
        offset += snprintf(output_buffer + offset, sizeof(output_buffer) - offset, "  ");
      }
    }

    // Get the diff between the frames
    std::string diff = frame.get_format_diff(reference);

    // Log the diff with the normalized prefix
    esp_log_printf_(min_level, tag, line, ESPHOME_LOG_FORMAT("%s [%s] %s"), normalized_prefix, output_buffer,
                    diff.c_str());
    // ESP_LOGD(TAG_BF, "%s [%s] %s", normalized_prefix, output_buffer, diff.c_str());
  }

  void print(const std::string &prefix) const { print(prefix, *this); }
  static void print(const std::string &prefix, const BaseFrame &frame, const char *tag = TAG_BF,
                    int min_level = ESPHOME_LOG_LEVEL_VERBOSE, int line = __LINE__) {
    if (!log_active(tag, min_level)) {
      return;
    }
    // Normalize the prefix to a fixed length of 15 characters
    std::string normalized_prefix = prefix;
    if (normalized_prefix.length() > 5) {
      normalized_prefix = normalized_prefix.substr(0, 5);  // Truncate if longer
    } else {
      normalized_prefix.append(5 - normalized_prefix.length(), ' ');  // Pad with spaces if shorter
    }

    // Calculate the required buffer size for the frame data
    char output_buffer[5 * sizeof(frame.packet.data) + 20];  // Size calculation
    // int offset = snprintf(output_buffer, sizeof(output_buffer), "[%2zu] ", frame.data_len_);
    int offset = 0;
    // Format the frame data into the buffer
    for (size_t i = 0; i < sizeof(frame.packet.data); ++i) {
      if (i < frame.data_len_) {
        offset += snprintf(output_buffer + offset, sizeof(output_buffer) - offset, "%02X", frame.packet.data[i]);
        // if (i < frame.data_len_ - 1) {
        //   offset += snprintf(output_buffer + offset, sizeof(output_buffer) - offset, ",");
        // } else {
        //   offset += snprintf(output_buffer + offset, sizeof(output_buffer) - offset, " ");
        // }
      } else {
        offset += snprintf(output_buffer + offset, sizeof(output_buffer) - offset, "  ");
      }
    }
    esp_log_printf_(min_level, tag, line, ESPHOME_LOG_FORMAT("%s [%s] %s"), normalized_prefix.c_str(), output_buffer,
                    get_format(frame).c_str());
    // Log the normalized prefix, frame data, and formatted frame string
    // ESP_LOGD(TAG_BF, "%s [%s] %s", normalized_prefix.c_str(), output_buffer, get_format(frame).c_str());
  }

  size_t size() const { return this->data_len_; }
  bool is_size_valid() const {
    return (this->data_len_ == frame_data_length_short || this->data_len_ == frame_data_length);
  }

  uint8_t *data() { return this->packet.data; }

  const uint8_t *data() const { return this->packet.data; }

  uint8_t &operator[](size_t index) { return this->packet.data[index]; }

  const uint8_t &operator[](size_t index) const { return this->packet.data[index]; }

  template<typename T> static std::string format_bits_details(const T &value) {
    // Calculate the number of bits in the type T
    constexpr size_t num_bits = sizeof(T) * 8;

    // Create a bitset from the raw bits of the value
    std::bitset<num_bits> bits_representation(*(reinterpret_cast<const uint64_t *>(&value)));

    // Convert to string and return
    return bits_representation.to_string();
  }

  static std::string format_bits_details(uint8_t byte) {
    std::bitset<8> bits(byte);
    return bits.to_string();
  }

  template<typename T> static std::string format_bits_details_diff(const T &current, const T &reference) {
    // Calculate the number of bits in the type T
    constexpr size_t num_bits = sizeof(T) * 8;

    // Create bitsets from the raw bits of current and reference
    std::bitset<num_bits> current_bits(*(reinterpret_cast<const uint64_t *>(&current)));
    std::bitset<num_bits> reference_bits(*(reinterpret_cast<const uint64_t *>(&reference)));

    // Create a string to hold the result
    std::string result;

    // Compare bit by bit from MSB to LSB
    for (size_t i = num_bits; i > 0; --i) {
      if (current_bits[i - 1] == reference_bits[i - 1]) {
        result += 'x';  // unchanged bit
      } else {
        result += current_bits[i - 1] ? '1' : '0';  // changed bit
      }
    }

    // Return the diff string
    return result;
  }

  frame_source_t get_source() const { return this->source_; }
  void set_source(frame_source_t source) { this->source_ = source; }
  hp_packetdata_t packet;  ///< Static packet data structure.
  /**
   * @brief Parse the temperature from a byte.
   * @param tempByte The byte containing the temperature.
   * @return The parsed temperature.
   */
  static inline float parse_temperature(temperature_t tempdata) {
    float halfDeg = tempdata.decimal ? 0.5f : 0.0f;
    // char buffer[101] = {};
    // itoa(tempdata.raw, buffer, 2);
    // ESP_LOGD(POOL_HEATER_TAG, "Temperature raw : %d|%d (%s) ",
    // tempdata.integer,tempdata.decimal,buffer);

    return tempdata.integer + 2 + halfDeg;
  }
  static void set_temperature(temperature_t &encoded_temp, float temperature) {
    encoded_temp.integer = static_cast<uint8_t>(temperature) - 2;
    encoded_temp.decimal = std::round(temperature) - std::trunc(temperature) > 0 ? true : false;
  }
  static std::string format_temperature_data(const temperature_t &temperature) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << std::setw(2) << parse_temperature(temperature);
    oss << "ºC(" << (temperature.unknown_2 ? '1' : '0');
    oss << (temperature.unknown_1 ? '1' : '0') << ")";
    oss << "(0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(temperature.raw) << ")";
    return oss.str();
  }
  std::string format_temperature_data_diff(const temperature_t &current, const temperature_t &reference) const {
    std::ostringstream oss;
    // Compare and format the temperature value
    if (parse_temperature(current) != parse_temperature(reference)) {
      oss << std::fixed << std::setprecision(1) << std::setw(2);
      oss << parse_temperature(current);
    } else {
      oss << "xx.x";  // No change
    }
    oss << "ºC(" << (current.unknown_2 != reference.unknown_2 ? (current.unknown_2 ? '1' : '0') : 'x');
    oss << (current.unknown_1 != reference.unknown_1 ? (current.unknown_1 ? '1' : '0') : 'x') << ")";
    if (current.raw != reference.raw) {
      oss << "(0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(current.raw) << ")";
    } else {
      oss << "(xxxx)";
    }
    return oss.str();
  }
  std::string get_format_diff(const BaseFrame &reference) const {
    if (get_frame_type() == FRAME_TEMP_OUT) {
      return format_tempout_frame_diff(reference);
    } else if (get_frame_type() == FRAME_TEMP_IN || get_frame_type() == FRAME_TEMP_IN_B) {
      return format_tempin_frame_diff(reference);
    } else if (get_frame_type() == FRAME_TEMP_PROG) {
      return format_tempprog_frame_diff(reference);
    } else if (get_frame_type() == FRAME_CLOCK) {
      return format_clock_frame_diff(reference);
    } else {
      return format_unknown_frame_diff(reference);
    }
  }

  static std::string get_format(const BaseFrame &frame) {
    if (frame.get_frame_type() == FRAME_TEMP_OUT) {
      return frame.format_tempout_frame();
    } else if (frame.get_frame_type() == FRAME_TEMP_IN || frame.get_frame_type() == FRAME_TEMP_IN_B) {
      return frame.format_tempin_frame();
    } else if (frame.get_frame_type() == FRAME_TEMP_PROG) {
      return frame.format_tempprog_frame();
    } else if (frame.get_frame_type() == FRAME_CLOCK) {
      return frame.format_clock_frame();
    } else {
      return frame.format_unknown_frame();
    }
    return "";
  }

  static std::string format_tempout_frame(const BaseFrame &frame) {
    tempout_t tempout = frame.packet.tempout;
    std::ostringstream oss;

    oss << frame.type_string() << "(" << frame.source_string() << "): ";
    oss << format_temperature_data(tempout.temperature) << ", ";
    oss << "Coil " << format_temperature_data(tempout.temperature_coil) << ", ";
    oss << "Exhaust " << format_temperature_data(tempout.temperature_exhaust) << ", ";
    oss << "4?? " << format_temperature_data(tempout.temperature_4) << ", ";
    oss << "R12[" << format_bits_details(tempout.reserved_1) << ", ";
    oss << format_bits_details(tempout.reserved_2) << "], ";
    oss << "R578[" << format_bits_details(tempout.reserved_5) << ",";
    oss << format_bits_details(tempout.reserved_7) << ",";
    oss << format_bits_details(tempout.reserved_8) << "] ";
    // oss << "Checksum: " << static_cast<int>(this->packet.checksum);

    return oss.str();
  }
  std::string format_tempout_frame_diff(const BaseFrame &reference) const {
    tempout_t tempout = this->packet.tempout;
    tempout_t ref_tempout = reference.packet.tempout;
    std::ostringstream oss;

    oss << type_string() << "(" << source_string() << "): ";
    oss << format_temperature_data_diff(tempout.temperature, ref_tempout.temperature) << ", ";
    oss << "Coil " << format_temperature_data_diff(tempout.temperature_coil, ref_tempout.temperature_coil) << ", ";
    oss << "Exhaust " << format_temperature_data_diff(tempout.temperature_exhaust, ref_tempout.temperature_exhaust)
        << ", ";
    oss << "4?? " << format_temperature_data_diff(tempout.temperature_4, ref_tempout.temperature_4) << ", ";
    oss << "R12[" << format_bits_details_diff(tempout.reserved_1, ref_tempout.reserved_1) << ", "
        << format_bits_details_diff(tempout.reserved_2, ref_tempout.reserved_2) << "], ";
    oss << "R578[" << format_bits_details_diff(tempout.reserved_5, ref_tempout.reserved_5) << ","
        << format_bits_details_diff(tempout.reserved_7, ref_tempout.reserved_7) << ","
        << format_bits_details_diff(tempout.reserved_8, ref_tempout.reserved_8) << "] ";

    return oss.str();
  }

  static std::string format_tempin_frame(const BaseFrame &frame) {
    tempin_t tempin = frame.packet.tempin;
    // tempin_b_t tempin_b = frame.packet.tempinb;
    std::ostringstream oss;

    oss << frame.type_string() << "(" << frame.source_string() << "): ";
    oss << format_temperature_data(tempin.temperature) << ", ";
    // if(frame.get_frame_type() == FRAME_TEMP_IN){
    oss << "R16[" << format_bits_details(tempin.reserved_1) << ", ";
    // }
    // else {
    //   oss << format_bits_details(tempin_b.reserved_1)
    //   oss << "Res 2-6: ["  << ", ";
    // }
    oss << format_bits_details(tempin.reserved_2) << ", ";
    oss << format_bits_details(tempin.reserved_3) << ", ";
    oss << format_bits_details(tempin.reserved_4) << ", ";
    oss << format_bits_details(tempin.reserved_5) << ", ";
    oss << format_bits_details(tempin.reserved_6) << "], ";
    oss << "R8" << format_bits_details(tempin.reserved_8) << " ";

    return oss.str();
  }

  
  static std::string format_unknown_frame(const BaseFrame &frame) {
    std::ostringstream oss;
    oss << frame.type_string() << "(" << frame.source_string() << "): ";

    // Format bits for each byte from index 1 to data_len_ - 1
    // so we skip the frame type byte and the checksum
    for (size_t i = 1; i < frame.data_len_-1; ++i) {
      oss << format_bits_details(frame.packet.data[i]) << " ";
    }

    return oss.str();
  }

  std::string format_unknown_frame_diff(const BaseFrame &reference) const {
    std::ostringstream oss;
    oss << type_string() << "(" << source_string() << "): ";

    // Compare bits for each byte from index 1 to data_len_, not including the
    // header and checksum
    for (size_t i = 1; i < this->data_len_-1; ++i) {
      oss<< format_bits_details_diff(this->packet.data[i],reference.packet.data[i]) ;
      if(i<this->data_len_-1) {
        oss << " ";
      }
    }

    return oss.str();
  }

  static std::string format_clock_frame(const BaseFrame &frame) {
    clock_time_t clock = frame.packet.clock;
    std::ostringstream oss;
    oss << frame.type_string() << "(" << frame.source_string() << "): ";
    oss << std::setw(4) << std::setfill('0') << static_cast<int>(clock.year + 2000)
        << "/";  // Assuming 'year' is an offset from 2000
    oss << std::setw(2) << std::setfill('0') << static_cast<int>(clock.month) << "/";
    oss << std::setw(2) << std::setfill('0') << static_cast<int>(clock.day) << " - ";
    oss << std::setw(2) << std::setfill('0') << static_cast<int>(clock.hour) << ":";
    oss << std::setw(2) << std::setfill('0') << static_cast<int>(clock.minute);
    return oss.str();
  }

  static std::string format_tempprog_frame(const BaseFrame &frame) {
    tempprog_t tempprog = frame.packet.tempprog;
    std::ostringstream oss;
    oss << frame.type_string() << "(" << frame.source_string() << "): ";
    oss << format_temperature_data(tempprog.set_point_1);
    oss << " - " << format_temperature_data(tempprog.set_point_2) << ", ";
    oss << "Mode: Power=" << (tempprog.mode.power ? "TRUE " : "FALSE")

        << ", Heat=" << (tempprog.mode.heat ? "TRUE " : "FALSE")
        << ", Auto Mode=" << (tempprog.mode.auto_mode ? "TRUE " : "FALSE") << ", ";

    oss << "R27[" << format_bits_details(tempprog.reserved_2) << ", ";
    oss << format_bits_details(tempprog.reserved_3) << ", ";
    oss << format_bits_details(tempprog.reserved_4) << ", ";
    oss << format_bits_details(tempprog.reserved_5) << ", ";
    oss << format_bits_details(tempprog.reserved_6) << ", ";
    oss << format_bits_details(tempprog.reserved_7) << "] ";
    return oss.str();
  }

  std::string format_tempin_frame_diff(const BaseFrame &reference) const {
    tempin_t tempin = this->packet.tempin;
    tempin_t ref_tempin = reference.packet.tempin;
    std::ostringstream oss;

    oss << type_string() << "(" << source_string() << "): ";
    oss << format_temperature_data_diff(tempin.temperature, ref_tempin.temperature) << ", ";
    oss << "R16[" << format_bits_details_diff(tempin.reserved_1, ref_tempin.reserved_1) << ", "
        << format_bits_details_diff(tempin.reserved_2, ref_tempin.reserved_2) << ", "
        << format_bits_details_diff(tempin.reserved_3, ref_tempin.reserved_3) << ", "
        << format_bits_details_diff(tempin.reserved_4, ref_tempin.reserved_4) << ", "
        << format_bits_details_diff(tempin.reserved_5, ref_tempin.reserved_5) << ", "
        << format_bits_details_diff(tempin.reserved_6, ref_tempin.reserved_6) << "], ";
    oss << "R8" << format_bits_details_diff(tempin.reserved_8, ref_tempin.reserved_8) << " ";

    return oss.str();
  }

  std::string format_tempprog_frame_diff(const BaseFrame &reference) const {
    tempprog_t tempprog = this->packet.tempprog;
    tempprog_t ref_tempprog = reference.packet.tempprog;
    std::ostringstream oss;

    oss << type_string() << "(" << source_string() << "): ";
    oss << format_temperature_data_diff(tempprog.set_point_1, ref_tempprog.set_point_1) << " - ";
    oss << format_temperature_data_diff(tempprog.set_point_2, ref_tempprog.set_point_2) << ", ";
    oss << "Mode: Power=" << format_bool_diff(tempprog.mode.power, ref_tempprog.mode.power)
        << ", Heat=" << format_bool_diff(tempprog.mode.heat, ref_tempprog.mode.heat)
        << ", Auto Mode=" << format_bool_diff(tempprog.mode.auto_mode, ref_tempprog.mode.auto_mode) << ", ";

    oss << "R27[" << format_bits_details_diff(tempprog.reserved_2, ref_tempprog.reserved_2) << ", "
        << format_bits_details_diff(tempprog.reserved_3, ref_tempprog.reserved_3) << ", "
        << format_bits_details_diff(tempprog.reserved_4, ref_tempprog.reserved_4) << ", "
        << format_bits_details_diff(tempprog.reserved_5, ref_tempprog.reserved_5) << ", "
        << format_bits_details_diff(tempprog.reserved_6, ref_tempprog.reserved_6) << ", "
        << format_bits_details_diff(tempprog.reserved_7, ref_tempprog.reserved_7) << "] ";

    return oss.str();
  }

  std::string format_clock_frame_diff(const BaseFrame &reference) const {
    clock_time_t clock = this->packet.clock;
    clock_time_t ref_clock = reference.packet.clock;
    std::ostringstream oss;
    oss << type_string() << "(" << source_string() << "): ";
    oss << format_time_diff(clock.year, ref_clock.year, 2000, "/")
        << format_time_diff(clock.month, ref_clock.month, 0, "/")
        << format_time_diff(clock.day, ref_clock.day, 0, " - ") << format_time_diff(clock.hour, ref_clock.hour, 0, ":")
        << format_time_diff(clock.minute, ref_clock.minute);

    return oss.str();
  }

  std::string get_format() const { return get_format(*this); }
  std::string format_tempout_frame() const { return format_tempout_frame(*this); }

  std::string format_tempin_frame() const { return format_tempin_frame(*this); }

  std::string format_unknown_frame() const { return format_unknown_frame(*this); }

  std::string format_clock_frame() const { return format_clock_frame(*this); }
  std::string format_tempprog_frame() const { return format_tempprog_frame(*this); }

  uint32_t get_frame_age_ms() const { return millis() - frame_time_ms_; }

  uint32_t get_frame_time_ms() const { return frame_time_ms_; }

  std::string format_bool_diff(bool current, bool reference) const {
    return (current == reference) ? "XXXXX" : (current ? "TRUE " : "FALSE");
  }

  std::string format_time_diff(uint8_t current, uint8_t reference, int offset = 0,
                               const std::string &separator = "") const {
    if (current == reference) {
      return "xx" + separator;
    } else {
      return std::to_string(current + offset) + separator;
    }
  }
  bool has_expired() { return ((millis() - this->frame_time_ms_) / 1000 > packet_expiry_time_s); }

 protected:
  frame_source_t source_;  ///< Source of the frame.
  size_t data_len_;
  uint32_t frame_time_ms_;
};

}  // namespace hayward_pool_heater
}  // namespace esphome
