/**
 * @file HeaterStatus.h
 * @brief Provides error code handling for the Hayward Pool Heater component.
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

#include <string>
#include <vector>

namespace esphome {
namespace hwp {

// Enum for error sources
typedef enum { ERROR_SOURCE_HARDWARE, ERROR_SOURCE_OPERATIONAL } ErrorSources;

// Enum for error statuses
typedef enum {
    ERROR_STATUS_S99,
    ERROR_STATUS_S00,
    ERROR_STATUS_P01,
    ERROR_STATUS_P02,
    ERROR_STATUS_P05,
    ERROR_STATUS_P04,
    ERROR_STATUS_E06,
    ERROR_STATUS_E07,
    ERROR_STATUS_E19,
    ERROR_STATUS_E29,
    ERROR_STATUS_E01,
    ERROR_STATUS_E02,
    ERROR_STATUS_E03,
    ERROR_STATUS_EE8
} ErrorStatuses;

/**
 * @class ErrorEntry
 * @brief Represents an error entry with code, description, solution, and source.
 */
class ErrorEntry {
  public:
    // Default constructor
    ErrorEntry()
        : value_(0), code_("S99"), source_(ERROR_SOURCE_OPERATIONAL), status_(ERROR_STATUS_S99),
          description_("Waiting For Data"), solution_("") {}

    // Constructor with parameters
    ErrorEntry(uint8_t value, std::string code, ErrorSources source, ErrorStatuses status,
               std::string description, std::string solution)
        : value_(value), code_(code), source_(source), status_(status), description_(description),
          solution_(solution) {}

    // Equality operator for comparison with uint8_t
    bool operator==(const uint8_t value) const { return this->value_ == value; }

    // Getters
    const std::string &get_code() const { return this->code_; }
    const std::string &get_description() const { return this->description_; }
    const std::string &get_solution() const { return this->solution_; }
    ErrorSources get_source() const { return this->source_; }
    ErrorStatuses get_status() const { return this->status_; }

    // To-String Method
    std::string to_string() const {
        return "Code: " + this->code_ + "\nDescription: " + this->description_ +
               "\nSolution: " + this->solution_ + "\nSource: " +
               (this->source_ == ERROR_SOURCE_HARDWARE ? "Hardware Issue" : "Operational Problem");
    }

  protected:
    uint8_t value_;
    std::string code_;
    ErrorSources source_;
    ErrorStatuses status_;
    std::string description_;
    std::string solution_;
};

// Vector containing all error codes
const std::vector<ErrorEntry> error_codes_ = {
    {0, "S00", ERROR_SOURCE_OPERATIONAL, ERROR_STATUS_S00, "Operational", ""},
    {1, "P01", ERROR_SOURCE_HARDWARE, ERROR_STATUS_P01, "Water inlet sensor malfunction",
        "Check or replace the sensor."},
    {2, "P02", ERROR_SOURCE_HARDWARE, ERROR_STATUS_P02, "Water outlet sensor malfunction",
        "Check or replace the sensor."},
    {5, "P05", ERROR_SOURCE_HARDWARE, ERROR_STATUS_P05, "Defrost sensor malfunction",
        "Check or replace the sensor."},
    {4, "P04", ERROR_SOURCE_HARDWARE, ERROR_STATUS_P04, "Outside temperature sensor malfunction",
        "Check or replace the sensor."},
    {6, "E06", ERROR_SOURCE_OPERATIONAL, ERROR_STATUS_E06,
        "Large temperature difference between inlet and outlet water",
        "Check the water flow or system obstruction."},
    {7, "E07", ERROR_SOURCE_OPERATIONAL, ERROR_STATUS_E07, "Antifreeze protection in cooling mode",
        "Check the water flow or outlet water temperature sensor."},
    {19, "E19", ERROR_SOURCE_OPERATIONAL, ERROR_STATUS_E19, "Level 1 antifreeze protection",
        "Ambient or inlet water temperature is too low."},
    {29, "E29", ERROR_SOURCE_OPERATIONAL, ERROR_STATUS_E29, "Level 2 antifreeze protection",
        "Ambient or inlet water temperature is even lower."},
    {1, "E01", ERROR_SOURCE_OPERATIONAL, ERROR_STATUS_E01, "High pressure protection",
        "Check the high pressure switch and refrigerant circuit pressure.\nCheck the water or air "
        "flow.\nEnsure the flow controller is working properly.\nCheck the inlet/outlet water "
        "valves.\nCheck the bypass setting."},
    {2, "E02", ERROR_SOURCE_OPERATIONAL, ERROR_STATUS_E02, "Low pressure protection",
        "Check the low pressure switch and refrigerant circuit pressure for leaks.\nClean the "
        "evaporator surface.\nCheck the fan speed.\nEnsure air can circulate freely through the "
        "evaporator."},
    {3, "E03", ERROR_SOURCE_OPERATIONAL, ERROR_STATUS_E03, "Flow detector malfunction",
        "Check the water flow.\nCheck the filtration pump and flow detector for faults."},
    {8, "EE8", ERROR_SOURCE_OPERATIONAL, ERROR_STATUS_EE8, "Communication problem",
        "Check the cable connections."}};

/**
 * @class HeaterStatus
 * @brief Manages the heater status by matching error codes to descriptions and solutions.
 */
class HeaterStatus {
  public:
    // Default constructor
    HeaterStatus() : error_entry_(error_codes_[0]) {}

    // Constructor with error code
    HeaterStatus(uint8_t value) { find_error_entry(value); }

    // Method to update the current error entry based on the error code
    void update(uint8_t value) { find_error_entry(value); }

    // Getters
    const std::string &get_code() const { return this->error_entry_.get_code(); }
    const std::string &get_description() const { return this->error_entry_.get_description(); }
    const std::string &get_solution() const { return this->error_entry_.get_solution(); }
    ErrorSources get_source() const { return this->error_entry_.get_source(); }
    ErrorStatuses get_status() const { return this->error_entry_.get_status(); }

    // To-String Method
    std::string to_string() const { return this->error_entry_.to_string(); }

  protected:
    ErrorEntry error_entry_;

    // Method to find and set the error entry based on the error code
    void find_error_entry(uint8_t value) {
        for (const auto &entry : error_codes_) {
            if (entry == value) {
                this->error_entry_ = entry;
                return;
            }
        }
        // Default to "Operational" if code not found
        this->error_entry_ = error_codes_[0];
    }
};

}  // namespace hwp
}  // namespace esphome
