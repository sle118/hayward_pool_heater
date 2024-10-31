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
 * @struct clock_time_t
 * @brief Represents the time-related data in the clock frame of the heat pump.
 *
 * This structure is part of the packet data and holds time-related information, but the
 * day/month/year fields are not actual calendar values. They likely represent the number of days or
 * an internal counter of how long the heat pump has been powered. This is typically reset when the
 * pump is powered off and back on, such as during seasonal use (e.g., after pool opening) or after
 * a power outage.
 *
 * - year: Likely represents the number of years the heat pump has been powered (on/off)
 * - month: Likely represents the number of months or some internal counter
 * - day: Likely represents the number of days since the heat pump was last powered on
 * - hour: Represents the current hour in 24-hour format (0-23)
 * - minute: Represents the current minute (0-59)
 */
typedef struct clock_time {
    uint8_t id;        ///< ID byte
    uint8_t reserved1; ///< Reserved or ID byte (00)
    uint8_t reserved2; ///< Reserved or ID byte (00)
    uint8_t
        year; ///< Likely the number of years the heat pump has been powered (not an actual year)
    uint8_t month; ///< Likely the number of months or some internal counter (not an actual month)
    uint8_t
        day; ///< Likely the number of days since the heat pump was powered on (not an actual day)
    uint8_t hour;      ///< Hour of the day (in 24-hour format, range 0-23)
    uint8_t minute;    ///< Minute of the hour (range 0-59)
    uint8_t reserved3; ///< Reserved byte
    uint8_t reserved4; ///< Reserved byte
    /**
     * @brief Converts the clock_time_t structure into a std::time_t value.
     *
     * This method converts the year, month, day, hour, and minute fields of the clock_time_t
     * structure into a std::tm structure and then into a std::time_t value.
     *
     * @return std::time_t The time represented by the structure.
     */
    std::time_t decode() const {
        // Create a std::tm structure to hold the time components
        std::tm tm_time = {};

        // Set the year, month, day, hour, and minute fields from the clock_time_t data
        // Since year and month are offsets, adjust them to be in the expected range.
        tm_time.tm_year =
            static_cast<int>(this->year); // Adjusted to the year 2000 (100 years offset from 1900)
        tm_time.tm_mon =
            static_cast<int>(this->month); // tm_mon is 0-based (0 = January, 11 = December)
        tm_time.tm_mday = static_cast<int>(this->day);
        tm_time.tm_hour = static_cast<int>(this->hour);
        tm_time.tm_min = static_cast<int>(this->minute);
        tm_time.tm_sec = 0; // Set seconds to zero as there's no field for seconds

        // Convert std::tm structure to std::time_t using mktime()
        std::time_t result_time = std::mktime(&tm_time);

        // Return the resulting std::time_t value
        return result_time;
    }
    std::string format() const {
        std::ostringstream oss;
        oss << std::setw(4) << std::setfill('0') << static_cast<int>(this->year)
            << "/"; // Assuming 'year' is an offset from 2000
        oss << std::setw(2) << std::setfill('0') << static_cast<int>(this->month) << "/";
        oss << std::setw(2) << std::setfill('0') << static_cast<int>(this->day) << " - ";
        oss << std::setw(2) << std::setfill('0') << static_cast<int>(this->hour) << ":";
        oss << std::setw(2) << std::setfill('0') << static_cast<int>(this->minute);
        return oss.str();
    }
    std::string diff(const struct clock_time& reference, const std::string& separator = "") const {
        std::string ref = reference.format();
        std::string current = this->format();
        CS cs;
        bool changed = (ref != current);
        cs.set_changed_base_color(changed);

        auto cs_inv = changed ? CS::invert : "";
        auto cs_inv_rst = changed ? CS::invert_rst : "";
        for (int i = 0; i < ref.length(); i++) {
            if (ref[i] != current[i]) {
                cs << cs_inv << ref[i] << cs_inv_rst;
            } else {
                cs << ref[i];
            }
        }
        cs << separator;
        return cs.str();
    }
    bool operator==(const optional<clock_time>& other) const {
        return other.has_value() && *this == *other;
    }
    bool operator==(const struct clock_time& other) const {
        return this->id == other.id && this->year == other.year && this->month == other.month &&
               this->day == other.day && this->hour == other.hour && this->minute == other.minute;
    }

    bool operator!=(const struct clock_time& other) const { return !(*this == other); }
    bool operator!=(const optional<struct clock_time>& other) const { return !(*this == other); }
} __attribute__((packed)) clock_time_t;
static_assert(sizeof(clock_time_t) == frame_data_length - 2, "frame structure has wrong size");

class FrameClock : public BaseFrame {
  public:
    CLASS_DEFAULT_IMPL(FrameClock, clock_time_t);
    static constexpr uint8_t FRAME_ID_CLOCK = 0xCF; // Clock
};

} // namespace hwp
} // namespace esphome
