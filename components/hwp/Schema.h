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
#include "esphome/components/climate/climate.h"
#include "esphome/components/climate/climate_mode.h"
#include "esphome/core/optional.h"
#include <bitset>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace esphome {
namespace hwp {

#ifndef STR_HELPER
#define STR_HELPER(x) #x
#endif
#ifndef STR
#define STR(x) STR_HELPER(x)
#endif

/**
 * Long Frame Structure (12 bytes)
 * --------------------------------
 * A long frame consists of 12 bytes with the following structure:
 *
 * -----------------------------------------------------------------------------
 * | XX | B1 | XX XX | XX XX | XX | XX | XX | XX XX | XX |
 * |    |    |       |       |    |    |    |       |    |
 * |    |    |       |       |    |    |    |       |    +--> Checksum byte
 * |    |    |       |       |    |    |    |       +------> Reserved or padding bytes
 * |    |    |       |       |    |    |    +-------------> Data byte
 * |    |    |       |       |    |    +------------------> Data byte
 * |    |    |       |       |    +-----------------------> Data byte
 * |    |    |       |       +----------------------------> Data byte
 * |    |    |       +------------------------------------> Data byte
 * |    |    +--------------------------------------------> Sub type or command identifier (B1)
 * +------------------------------------------------------> Frame type byte (XX)
 * -----------------------------------------------------------------------------
 *
 * Fields:
 *  - Frame type: The first byte identifies the type of frame, which determines the interpretation
 * of the rest of the packet.
 *  - Sub type: The second byte is fixed to `B1` in long frames and may signal that this is a
 * specific class of frame (e.g., configuration or state).
 *  - Data: Bytes 3 to 10 contain the actual data of the frame (e.g., temperature, mode, setpoints).
 * Unused bytes may be padding or reserved.
 *  - Checksum: The last byte (12th byte) is the checksum, calculated over the first 11 bytes to
 * ensure data integrity.
 */

/**
 * Short Frame Structure (9 bytes)
 * -------------------------------
 * A short frame consists of 9 bytes with the following structure:
 *
 * -----------------------------------------------------------------------------
 * | XX | XX | XX XX | XX XX | XX | XX | XX | XX |
 * |    |    |       |       |    |    |    |    |
 * |    |    |       |       |    |    |    +-----> Checksum byte
 * |    |    |       |       |    |    +----------> Data byte
 * |    |    |       |       |    +---------------> Data byte
 * |    |    |       |       +--------------------> Data byte
 * |    |    |       +----------------------------> Data byte
 * |    |    +------------------------------------> Data byte
 * +----------------------------------------------> Frame type byte (XX)
 * -----------------------------------------------------------------------------
 *
 * Fields:
 *  - Frame type: The first byte identifies the type of frame, and the value may vary in short
 * frames, depending on the specific functionality.
 *  - Sub type: The second byte is part of the data and can vary depending on the frame's purpose.
 *  - Data: Bytes 3 to 8 contain the main data, which could represent states, temperatures, or other
 * attributes. Unused bytes may be reserved or padding.
 *  - Checksum: The final byte (9th byte) is a checksum, calculated over the first 8 bytes,
 * excluding the frame type, ensuring data integrity.
 */

static constexpr uint8_t frame_data_length = 12;
static constexpr uint8_t frame_data_length_short = 9;

class FanMode {
  public:
    enum Value : uint8_t {
        LOW_SPEED = 0x00,         // 0000 - Low speed
        HIGH_SPEED = 0x01,        // 0001 - High speed
        AMBIENT = 0x02,           // 0010 - Ambient
        SCHEDULED = 0x03,         // 0011 - Time
        AMBIENT_SCHEDULED = 0x04, // 0100 - Ambient and Time
        UNKNOWN = 0xFF
    };
    static const FanMode unknown;
    static const FanMode low_speed;
    static const FanMode high_speed;
    static const FanMode ambient;
    static const FanMode scheduled;
    static const FanMode ambient_scheduled;
    static const char* ambient_desc;
    static const char* ambient_scheduled_desc;
    static const char* scheduled_desc;
    FanMode() = default;
    constexpr FanMode(Value value) : value_(value) {}
    FanMode(uint8_t value) : value_(LOW_SPEED) {
        if (value <= AMBIENT_SCHEDULED) {
            this->value_ = static_cast<Value>(value);
        }
    }
    uint8_t to_raw() const { return static_cast<uint8_t>(value_); }
    optional<climate::ClimateFanMode> to_climate_fan_mode() const {
        switch (value_) {
        case LOW_SPEED:
            return make_optional<climate::ClimateFanMode>(climate::ClimateFanMode::CLIMATE_FAN_LOW);
        case HIGH_SPEED:
            return make_optional<climate::ClimateFanMode>(
                climate::ClimateFanMode::CLIMATE_FAN_HIGH);
        default:
            return nullopt;
        }
    }
    /**
     * @brief Convert the fan mode to a custom fan mode string used in
     * the climate component, if applicable.
     *
     * @return optional<std::string>
     */
    optional<std::string> to_custom_fan_mode() const {
        switch (value_) {
        case AMBIENT:
            return make_optional<std::string>(ambient_desc);
        case SCHEDULED:
            return make_optional<std::string>(scheduled_desc);
        case AMBIENT_SCHEDULED:
            return make_optional<std::string>(ambient_scheduled_desc);
        default:
            return nullopt;
        }
    }
    /**
     * @brief Converts a climate custom fan mode string to
     * the corresponding fan mode if applicable
     *
     * @return optional<std::string>
     */
    static optional<FanMode> from_custom_fan_mode(const std::string& fan_mode) {
        if (fan_mode == ambient_desc) {
            return make_optional<FanMode>(FanMode::AMBIENT);
        } else if (fan_mode == scheduled_desc) {
            return make_optional<FanMode>(FanMode::SCHEDULED);
        } else if (fan_mode == ambient_scheduled_desc) {
            return make_optional<FanMode>(FanMode::AMBIENT_SCHEDULED);
        } else {
            return nullopt;
        }
    }
    /**
     * @brief Converts a climate fan mode to the corresponding
     * fan mode if applicable
     *
     * @param fan_mode
     * @return optional<FanMode>
     */
    static optional<FanMode> from_climate_fan_mode(const climate::ClimateFanMode& fan_mode) {
        switch (fan_mode) {
        case climate::ClimateFanMode::CLIMATE_FAN_LOW:
            return make_optional<FanMode>(FanMode::LOW_SPEED);
        case climate::ClimateFanMode::CLIMATE_FAN_HIGH:
            return make_optional<FanMode>(FanMode::HIGH_SPEED);
        default:
            return nullopt;
        }
    }
    /**
     * @brief Converts a climate call to the corresponding fan mode if applicable
     *
     * If the call contains a custom fan mode, it is tried to be converted first.
     * If that fails, the fan mode is tried to be converted.
     *
     * @param call A climate call
     * @return optional<FanMode> The fan mode if conversion was successful, nullopt otherwise
     */
    static optional<FanMode> from_call(const climate::ClimateCall& call) {
        if (call.get_custom_fan_mode().has_value()) {
            auto from_custom = from_custom_fan_mode(*call.get_custom_fan_mode());
            if (from_custom.has_value()) {
                return from_custom;
            }
        }
        if (call.get_fan_mode().has_value()) {
            auto from_climate = from_climate_fan_mode(call.get_fan_mode().value());
            if (from_climate.has_value()) {
                return from_climate;
            }
        }
        return nullopt;
    }

    /**
     * @brief Checks if two fan modes are equal
     *
     * Compares two fan modes for equality by comparing their underlying values.
     *
     * @param other The fan mode to compare to
     * @return true if the fan modes are equal, false otherwise
     */
    bool operator==(const FanMode& other) const { return value_ == other.value_; }
    /**
     * @brief Converts the fan mode to a human-readable string
     *
     * Converts the fan mode to a human-readable string.
     *
     * @return const char* A string describing the fan mode
     */

    const char* to_string() const {
        switch (value_) {
        case LOW_SPEED:
            return "Low Speed";
        case HIGH_SPEED:
            return "High Speed";
        case AMBIENT:
            return ambient_desc;
        case SCHEDULED:
            return scheduled_desc;
        case AMBIENT_SCHEDULED:
            return ambient_scheduled_desc;
        default:
            return "Unknown";
        }
    }
    /**
     * @brief Converts the fan mode to a short log string representation.
     *
     * Returns a short string for logging purposes based on the fan mode value.
     * Each fan mode is represented by a unique abbreviation, ensuring concise log entries.
     *
     * @return const char* A short string representing the fan mode for logging.
     */
    const char* log_string() const {
        switch (value_) {
        case LOW_SPEED:
            return "LOW   ";
        case HIGH_SPEED:
            return "HIGH  ";
        case AMBIENT:
            return "AMBI  ";
        case SCHEDULED:
            return "TIME  ";
        case AMBIENT_SCHEDULED:
            return "AMBTME";
        default:
            return "UNK ";
        }
    }
    /**
     * @brief Configures the supported fan modes for the climate device.
     *
     * This function sets the standard fan modes and adds custom fan modes
     * to the provided ClimateTraits object. It ensures that the climate
     * device recognizes and can operate with the specified fan modes,
     * including both predefined and custom ones.
     *
     * @param traits A reference to the ClimateTraits object to modify.
     */
    void set_supported_fan_modes(climate::ClimateTraits& traits) {
        traits.set_supported_fan_modes(
            {climate::ClimateFanMode::CLIMATE_FAN_LOW, climate::ClimateFanMode::CLIMATE_FAN_HIGH});
        traits.add_supported_custom_fan_mode(scheduled_desc);
        traits.add_supported_custom_fan_mode(ambient_desc);
        traits.add_supported_custom_fan_mode(ambient_scheduled_desc);
    }

  private:
    Value value_;
}; // namespace esphome
#pragma pack(push, 1) // Ensure strict packing without padding

/**
 * @brief Represents a single byte as a collection of bit fields.
 *
 * This structure is used to represent a single byte of data as a collection
 * of individual bit fields. It is useful for extracting specific information
 * from a byte of data, such as flags or status bits.
 *
 * The structure contains a single byte-sized member variable, `raw`, which
 * can be used to access the entire byte of data. The structure also contains
 * a union of bit fields, which can be used to extract individual bits from
 * the byte. The union is anonymous, so the bit fields can be accessed directly
 * using the dot operator.
 *
 * For example, to extract the least significant bit from the byte, you can
 * use the expression `bits.unknown_1`.
 *
 * The structure also contains a set of member functions that can be used to
 * compare two `bits_details_t` objects for equality or inequality. The
 * `diff` member function can be used to generate a string that describes the
 * differences between two `bits_details_t` objects. The `format` member
 * function can be used to generate a string that describes the bit fields
 * in a specific format.
 *
 * The structure is annotated with the `packed` attribute, which ensures that
 * the compiler does not add any padding between the bit fields.
 */
typedef struct bits_details {
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

    bool operator==(const struct bits_details& other) const { return raw == other.raw; }
    bool operator!=(const struct bits_details& other) const { return !(*this == other); }
    /**
     * @brief  Generates a string that describes the differences between the bit
     *
     * The string is generated by calling the `diff` member function with
     * the given reference object, a start index of 0, a length of 8
     * (the number of bits in a byte), and the given separator string.
     *
     * @param reference The reference object to compare against.
     * @param sep The separator string to use between differences.
     *
     * @return A string that describes the differences between the bit
     * fields of the current object and the given reference object.
     */
    std::string diff(const struct bits_details& reference, const char* sep) const {
        return diff(reference, 0, 8, sep);
    }
    /**
     * @brief  Generates a string that describes the differences between the bit
     *
     * Generates a string that describes the differences between the bit
     * fields of the current object and the given reference object.
     *
     * The string is generated by creating bitsets from the raw bits of the
     * current and reference objects, and then comparing bit by bit from
     * MSB to LSB, within the specified range. If a difference is detected,
     * the colors are inverted. If no difference is detected and the colors
     * are currently inverted, the inversion is reset. The string is
     * constructed by appending the current bit with or without inversion.
     * The function returns the constructed string.
     *
     * @param reference The reference object to compare against.
     * @param start The starting index of the range to compare.
     * @param len The length of the range to compare.
     * @param sep The separator string to use between differences.
     *
     * @return A string that describes the differences between the bit
     * fields of the current object and the given reference object.
     */
    std::string diff(const struct bits_details& reference, size_t start = 0, size_t len = 8,
        const char* sep = "") const {
        constexpr size_t num_bits = sizeof(raw) * 8;
        bool changed = this->raw != reference.raw;
        CS cs;

        cs.set_changed_base_color(changed);

        if (start >= num_bits) {
            start = 0; // Ensure start is within bounds
        }

        if (start + len > num_bits) {
            len = num_bits - start; // Adjust length to stay within bounds
        }

        // Create bitsets from the raw bits of current and reference
        std::bitset<num_bits> current_bits(this->raw);
        std::bitset<num_bits> reference_bits(reference.raw);

        bool inverted = false; // Track if we are currently in an inverted state

        // Compare bit by bit from MSB to LSB, within the specified range
        for (size_t i = start + len; i > start; --i) {
            bool is_different = current_bits[i - 1] != reference_bits[i - 1];

            // If difference detected and not already in inverted state, invert colors
            if (is_different && !inverted) {
                cs << CS::invert; // Apply inversion
                inverted = true;  // Mark that we are now in an inverted state
            }
            // If no difference and we are in an inverted state, revert inversion
            else if (!is_different && inverted) {
                cs << CS::invert_rst; // Reset inversion
                inverted = false;     // Mark that we are no longer in an inverted state
            }

            // Append the current bit with or without inversion
            cs << (current_bits[i - 1] ? "1" : "0");
        }

        // If still in inverted state at the end, reset it
        if (inverted) {
            cs << CS::invert_rst;
        }

        // Append separator if needed
        cs << sep;

        // Return the constructed string
        return cs.str();
    }
    /**
     * @brief  Generates a string that describes the differences between the bit
     *
     * @param raw The raw bits of the current object.
     * @param ref The raw bits of the reference object.
     *
     * @return A string that describes the differences between the bit
     * fields of the current object and the given reference object.
     */
    static std::string bit(uint8_t raw, uint8_t ref) {
        struct bits_details bits;
        struct bits_details ref_bits;
        bits.raw = raw;
        ref_bits.raw = ref;
        return bits.diff(ref_bits, 0, 1);
    }

    /**
     * @brief Formats the bitset into a string representation with a specified separator.
     *
     * This function utilizes the full range of bits (0-8) of the bitset for formatting.
     *
     * @param sep The separator to use between formatted bits.
     *
     * @return A string representing the formatted bitset with the specified separator.
     */
    std::string format(const char* sep) const { return this->format(0, 8, sep); }

    /**
     * @brief Formats the bitset into a string representation with a specified separator.
     *
     * This function allows the user to specify a subset of the bitset to format.
     * The subset is specified by providing a starting index and a length.
     * The function clamps the start and length to valid ranges.
     *
     * @param start The starting index (inclusive) of the subset to format.
     * @param len The length of the subset to format.
     * @param sep The separator to use between formatted bits.
     *
     * @return A string representing the formatted subset of the bitset with the specified
     * separator.
     */
    std::string format(size_t start = 0, size_t len = 8, const char* sep = "") const {
        std::bitset<8> bits(this->raw);
        // Clamp start and len to valid ranges
        if (start >= 8) {
            start = 0;
        }
        if (start + len > 8) {
            len = 8 - start;
        }
        std::string result;
        for (size_t i = start + len; i > start; --i) {
            result += bits[i - 1] ? '1' : '0';
        }

        result += sep;

        return result;
    }

} __attribute__((packed)) bits_details_t;

class DefrostEcoMode {
  public:
    // Define static instances representing the possible modes
    static const DefrostEcoMode Eco;
    static const DefrostEcoMode Normal;
    static const char* DefrostEcoStrEco;
    static const char* DefrostEcoStrNormal;

    // Overload assignment operator from std::string to enum-like class
    DefrostEcoMode& operator=(const std::string& mode_str) {
        *this = from_string(mode_str);
        return *this;
    }

    // Overload assignment from boolean (eco -> true, normal -> false)
    DefrostEcoMode& operator=(bool eco_mode) {
        this->value_ = eco_mode ? 0 : 1; // 0 for eco, 1 for normal
        return *this;
    }

    // Implicit conversion to std::string
    operator std::string() const { return this->to_string(); }

    // Implicit conversion to boolean (eco -> true, normal -> false)
    operator bool() const { return this->value_ == 0; }

    // Equality operators
    bool operator==(const DefrostEcoMode& other) const { return this->value_ == other.value_; }
    bool operator==(const std::string& other) const { return this->to_string() == other; }
    bool operator!=(const DefrostEcoMode& other) const { return !(*this == other); }
    bool operator!=(const std::string& other) const { return !(*this == other); }

    // log_format function for returning a fixed-length uppercase string
    const char* log_format() const {
        switch (this->value_) {
        case 0:
            return "ECO ";
        case 1:
            return "NORM";
        default:
            return "UNKN";
        }
    }

    // Function to convert to string
    std::string to_string() const {
        switch (this->value_) {
        case 0:
            return DefrostEcoStrEco;
        case 1:
            return DefrostEcoStrNormal;
        default:
            return "Unknown";
        }
    }

    // Static function to convert from string to enum-like class
    static DefrostEcoMode from_string(const std::string& mode_str) {
        if (mode_str == DefrostEcoStrEco) {
            return DefrostEcoMode::Eco;
        } else if (mode_str == DefrostEcoStrNormal) {
            return DefrostEcoMode::Normal;
        }
        return DefrostEcoMode::Normal;
    }
    DefrostEcoMode() : value_(0) {}

  private:
    // Private constructor with an integer value
    explicit DefrostEcoMode(uint8_t value) : value_(value) {}

    uint8_t value_; // Internal representation of the mode (0 for eco, 1 for normal)
};

// Define the static constants

/**
 * @class HeatPumpRestrict
 * @brief Represents mode restrictions for a heat pump, allowing configuration for cooling only,
 * heating only, or no restriction.
 *
 * The HeatPumpRestrict class encapsulates the operational mode restrictions for a heat pump. It
 * provides static instances for different modes and enables conversion from and to string
 * representation. The class also supports implicit conversions and equality comparisons.
 */
class HeatPumpRestrict {
  public:
    // Define static instances representing the possible modes
    static const HeatPumpRestrict Cooling; ///< Cooling only mode
    static const HeatPumpRestrict Any;     ///< Any mode (cooling, heating, and auto)
    static const HeatPumpRestrict Heating; ///< Heating only mode
    static const char* HeatPumpStrCooling; ///< String representation for Cooling
    static const char* HeatPumpStrAny;     ///< String representation for Any
    static const char* HeatPumpStrHeating; ///< String representation for Heating

    /**
     * @brief Assignment operator from string to enum-like class.
     * @param mode_str String representing the mode.
     * @return Reference to the updated HeatPumpRestrict object.
     */
    HeatPumpRestrict& operator=(const std::string& mode_str) {
        *this = from_string(mode_str);
        return *this;
    }

    /**
     * @brief Get the internal value representing the mode.
     * @return Integer value of the mode.
     */
    uint8_t get_value() const { return value_; }

    /**
     * @brief Implicit conversion to std::string.
     * @return String representation of the mode.
     */
    operator std::string() const { return this->to_string(); }

    /**
     * @brief Equality operator.
     * @param other Another HeatPumpRestrict object.
     * @return True if both objects are equal, false otherwise.
     */
    bool operator==(const HeatPumpRestrict& other) const { return this->value_ == other.value_; }

    /**
     * @brief Inequality operator.
     * @param other Another HeatPumpRestrict object.
     * @return True if both objects are not equal, false otherwise.
     */
    bool operator!=(const HeatPumpRestrict& other) const { return !(*this == other); }

    /**
     * @brief Get a fixed-length uppercase string for logging purposes.
     * @return A string representation for logging.
     */
    const char* log_format() const {
        switch (this->value_) {
        case 0:
            return "COOLING ONLY";
        case 1:
            return "ANY MODE    ";
        case 2:
            return "HEATING ONLY";
        default:
            return "UNKNOWN     ";
        }
    }

    /**
     * @brief Convert the mode to a string.
     * @return String representation of the mode.
     */
    std::string to_string() const {
        switch (this->value_) {
        case 0:
            return HeatPumpStrCooling;
        case 1:
            return HeatPumpStrAny;
        case 2:
            return HeatPumpStrHeating;
        default:
            return "Unknown";
        }
    }

    /**
     * @brief Static function to convert from string to enum-like class.
     * @param mode_str String representing the mode.
     * @return A HeatPumpRestrict object corresponding to the string.
     */
    static HeatPumpRestrict from_string(const std::string& mode_str) {
        if (mode_str == HeatPumpStrCooling) {
            return HeatPumpRestrict::Cooling;
        } else if (mode_str == HeatPumpStrAny) {
            return HeatPumpRestrict::Any;
        } else if (mode_str == HeatPumpStrHeating) {
            return HeatPumpRestrict::Heating;
        }
        return HeatPumpRestrict::Any;
    }

    /** @brief Default constructor initializing to 'Any' mode. */
    HeatPumpRestrict() : value_(1) {}

  private:
    /**
     * @brief Private constructor with an integer value.
     * @param value Internal representation of the enum value.
     */
    explicit HeatPumpRestrict(uint8_t value) : value_(value) {}

    uint8_t value_; ///< Internal representation of the enum value
};

/**
 * @brief Represents whether or not a flow meter is installed (and enabled in the heatpump).
 *
 * The flow meter is a device that measures the flow rate of a fluid (gas or liquid)
 * through a pipe. It is used to calculate the energy consumption of the heat pump.
 *
 * This class is an enum-like class with two possible values: `Enabled` and `Disabled`.
 * It can be used as a boolean value, with `Enabled` being `true` and `Disabled` being
 * `false`.
 *
 * @note When enabling the flow meter, use parameter U02 - Pulses per liter to set the flow rate.
 */
class FlowMeterEnable {
  public:
    // Define static instances representing the possible modes
    static const FlowMeterEnable Enabled;    ///< Flow meter is enabled
    static const FlowMeterEnable Disabled;   ///< Flow meter is disabled
    static const char* FlowMeterStrEnabled;  ///< String representation of Enabled
    static const char* FlowMeterStrDisabled; ///< String representation of Disabled

    /**
     * @brief Overload assignment operator from std::string to enum-like class.
     * @param mode_str String representing the mode.
     * @return A reference to the current object.
     */
    FlowMeterEnable& operator=(const std::string& mode_str) {
        *this = from_string(mode_str);
        return *this;
    }

    /**
     * @brief Overload assignment from boolean (enabled -> true, disabled -> false).
     * @param enabled Boolean value representing the mode.
     * @return A reference to the current object.
     */
    FlowMeterEnable& operator=(bool enabled) {
        this->value_ = enabled ? Enabled : Disabled; // 0 for enabled, 1 for disabled
        return *this;
    }
    FlowMeterEnable& operator=(uint8_t value) {
        this->value_ = value > 0 ? Enabled : Disabled;
        return *this;
    }
    // Implicit conversion to std::string
    /**
     * @brief Implicit conversion to std::string.
     * @return A string representing the mode.
     */
    operator std::string() const { return this->to_string(); }

    // Implicit conversion to boolean (enabled -> true, disabled -> false)
    /**
     * @brief Implicit conversion to boolean (enabled -> true, disabled -> false).
     * @return A boolean value representing the mode.
     */
    operator bool() const { return this->value_ == Enabled.value_; }

    // Equality operators
    /**
     * @brief Equality operator.
     * @param other FlowMeterEnable object to compare with.
     * @return True if the two objects represent the same mode, false otherwise.
     */
    bool operator==(const FlowMeterEnable& other) const { return this->value_ == other.value_; }
    /**
     * @brief Equality operator.
     * @param other String to compare with.
     * @return True if the two objects represent the same mode, false otherwise.
     */
    bool operator==(const std::string& other) const { return this->to_string() == other; }
    /**
     * @brief Inequality operator.
     * @param other FlowMeterEnable object to compare with.
     * @return False if the two objects represent the same mode, true otherwise.
     */
    bool operator!=(const FlowMeterEnable& other) const { return !(*this == other); }
    /**
     * @brief Inequality operator.
     * @param other String to compare with.
     * @return False if the two objects represent the same mode, true otherwise.
     */
    bool operator!=(const std::string& other) const { return !(*this == other); }

    // log_format function for returning a fixed-length uppercase string
    /**
     * @brief Log format function for returning a fixed-length uppercase string.
     * @return A fixed-length uppercase string representing the mode.
     */
    const char* log_format() const {
        switch (this->value_) {
        case 0:
            return "ENBL";
        case 1:
            return "DIS ";
        default:
            return "UNKN";
        }
    }

    // Function to convert to string
    /**
     * @brief Function to convert to string.
     * @return A string representing the mode.
     */
    std::string to_string() const {
        switch (this->value_) {
        case 0:
            return FlowMeterStrEnabled;
        case 1:
            return FlowMeterStrDisabled;
        default:
            return "Unknown";
        }
    }

    // Static function to convert from string to enum-like class
    /**
     * @brief Static function to convert from string to enum-like class.
     * @param mode_str String representing the mode.
     * @return A FlowMeterEnable object representing the mode.
     */
    static FlowMeterEnable from_string(const std::string& mode_str) {
        if (mode_str == FlowMeterStrEnabled) {
            return FlowMeterEnable::Enabled;
        } else if (mode_str == FlowMeterStrDisabled) {
            return FlowMeterEnable::Disabled;
        }
        return FlowMeterEnable::Disabled;
    }
    FlowMeterEnable() : value_(0) {}

  private:
    // Private constructor with an integer value
    explicit FlowMeterEnable(uint8_t value) : value_(value) {}

    uint8_t value_; // Internal representation of the mode (0 for enabled, 1 for disabled)
};

/**
 * @brief Enumeration for heat pump operational states.
 *
 * - `STATE_OFF`: The heat pump is turned off.
 * - `STATE_COOLING_MODE`: The heat pump is in cooling mode.
 * - `STATE_HEATING_MODE`: The heat pump is in heating mode.
 * - `STATE_AUTO_MODE`: The heat pump is in automatic mode, adjusting between heating and cooling as
 * needed.
 */
typedef enum { STATE_OFF, STATE_COOLING_MODE, STATE_HEATING_MODE, STATE_AUTO_MODE } active_modes_t;

/**
 * @brief Structure to hold decimal number representations from raw temperature bytes exchanged with
 * the heat pump.
 *
 * The structure holds a single byte representing a decimal number, where the integer part is in the
 * lower 5 bits and the decimal part is in the higher 1 bit. The remaining 2 bits are reserved for
 * negative number and offset respectively.
 *
 * The structure also provides methods to encode and decode the temperature from and to the raw byte
 * representation.
 *
 * The encoding method takes a floating-point number as input and stores it in the structure. The
 * decoding method takes the structure as input and returns the decoded temperature.
 */

typedef struct temperature {
    union {
        struct {
            uint8_t decimal : 1;
            uint8_t integer : 5;
            uint8_t offset : 1;
            uint8_t negative : 1;
        };
        uint8_t raw;
    };

    /**
     * @brief Encode the given floating-point temperature into the structure.
     *
     * Takes a floating-point number as input and stores it in the structure. The encoding is done
     * according to the following rules:
     * - The sign bit (negative) is set if the temperature is negative.
     * - The offset bit is set if the temperature is greater than or equal to 2 and the low range bit
     *   is not set.
     * - The decimal bit is set if the temperature has a fractional part.
     * - The integer part is stored in the lower 5 bits.
     *
     * @param temperature The temperature to encode.
     */
    void encode(float temperature) {
        this->negative = (temperature < 0) ? 1 : 0;
        float abs_temp = std::abs(temperature);

        // Set the offset if the temperature is >= 2 and not low range
        this->offset = (abs_temp >= 2) ? 1 : 0;
        if (this->offset) {
            abs_temp -= 2;
        }
        this->decimal = (fabs(temperature - static_cast<int>(temperature)) >= 0.5f) ? 1 : 0;
        this->integer = static_cast<uint8_t>(abs_temp);
    }
    /**
     * @brief Compare two temperature values and highlight differences.
     *
     * Compares the temperature value with the given reference temperature and returns a string
     * representation of the comparison result.
     *
     * The comparison is done with a given tolerance to avoid small precision issues.
     *
     * @param reference The reference temperature value for comparison.
     * @param sep The separator to be used in the formatted string. Default is "".
     * @return The formatted string.
     */
    std::string diff(const struct temperature& reference, const char* sep = "") const {
        std::ostringstream cs;
        // Tolerance for floating-point comparison to avoid small precision issues
        const float tolerance = 0.01f;
        bool changed = (std::fabs(this->decode() - reference.decode()) > tolerance);
        auto cs_inv = changed ? CS::invert : "";
        auto cs_inv_rst = changed ? CS::invert_rst : "";

        // Compare decoded values (with tolerance)
        cs << cs_inv << std::fixed << std::setprecision(1) << std::setw(4) << this->decode()
           << cs_inv_rst << "C(0x" << cs_inv << std::setw(2) << std::setfill('0') << std::hex
           << static_cast<int>(this->raw) << cs_inv_rst << ")" << sep;
        return cs.str();
    }
    /**
     * @brief Format the temperature value as a string.
     *
     * Formats the temperature value as a string with the given separator.
     *
     * @param sep The separator to be used in the formatted string. Default is "".
     * @return The formatted string.
     */
    std::string format(const char* sep = "") const {
        std::ostringstream oss;
        // Set fixed-point notation with one decimal precision
        oss << std::fixed << std::setprecision(1) << std::setw(2) << this->decode();
        // Append the raw hexadecimal representation
        oss << "C(0x" << std::setw(2) << std::setfill('0') << std::hex
            << static_cast<int>(this->raw) << ")";
        oss << sep; // Append the separator
        return oss.str();
    }

    /**
     * @brief Decodes the temperature from the structure.
     *
     * This method takes the individual parts of the temperature
     * (integer, decimal, offset, and negative) and combines them
     * into a single floating-point value representing the temperature.
     *
     * @return The decoded temperature as a float.
     */
    float decode() const {
        // Combine the integer and decimal parts into a single float
        float result =
            static_cast<float>(this->integer) + (static_cast<bool>(this->decimal) ? 0.5f : 0.0f);

        // Apply the offset if necessary
        if (this->offset) {
            result += 2;
        }

        // Apply the negative sign if necessary
        return result * (static_cast<bool>(this->negative) ? -1.0f : 1.0f);
    }
    bool operator==(const struct temperature& other) const { return this->raw == other.raw; }
    bool operator==(const optional<struct temperature>& other) const {
        return other.has_value() && *this == *other;
    }
    bool operator!=(const struct temperature& other) const { return !(*this == other); }
    bool operator!=(const optional<struct temperature>& other) const { return !(*this == other); }
    struct temperature& operator=(float value) {
        this->encode(value);
        return *this;
    }
    struct temperature& operator=(uint8_t value) {
        this->raw = value;
        return *this;
    }
} __attribute__((packed)) temperature_t;

static_assert(sizeof(temperature_t) == 1, "sizeof(temperature_t) != 1");

/**
 * @brief Structure to hold decimal number representations from raw temperature bytes exchanged with
 * the heat pump for temperatures with a large range.
 *
 * The structure holds a single byte representing a decimal number, where the integer part is in the
 * lower 7 bits and the decimal part is in the higher 1 bit.
 *
 * The structure also provides methods to encode and decode the temperature from and to the raw byte
 * representation.
 *
 * The encoding method takes a floating-point number as input and stores it in the structure. The
 * decoding method takes the structure as input and returns the decoded temperature.
 */
typedef struct temperature_extended {
    union {
        struct {
            uint8_t decimal : 1;
            uint8_t integer : 7;
        };
        uint8_t raw;
    };

    /**
     * @brief Format the temperature value as a string.
     *
     * Formats the temperature value as a string with the given separator.
     *
     * @param sep The separator to be used in the formatted string. Default is " ".
     *
     * @return The formatted string.
     */
    std::string format(const char* sep = "") const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << std::setw(2) << this->decode();
        oss << "C(0x" << std::setw(2) << std::setfill('0') << std::hex
            << static_cast<int>(this->raw) << ")";
        oss << sep;
        return oss.str();
    }

  
    /**
     * @brief Encode the temperature into the structure.
     *
     * This method takes a floating-point number as input and stores it in the structure.
     * The encoding is done by adding 30 to the temperature and storing the result in the
     * integer part, and storing the decimal part in the decimal bit.
     *
     * @param temperature The temperature to encode.
     */
    void encode(float temperature) {
        this->decimal = (fabs(temperature - static_cast<int>(temperature)) >= 0.5f) ? 1 : 0;
        this->integer = static_cast<uint8_t>(temperature + 30);
    }

    /**
     * @brief Compare the given temperature with the current temperature.
     * @param reference The temperature to compare with.
     * @return A string representation of the comparison result.
     */
    std::string diff(const struct temperature_extended& reference, const char* sep = "") const {
        CS cs;
        bool changed = (this->raw != reference.raw);
        cs.set_changed_base_color(changed);

        auto cs_inv = changed ? CS::invert : "";
        auto cs_inv_rst = changed ? CS::invert_rst : "";

        // Compare decoded values (with tolerance)
        cs << cs_inv << std::fixed << std::setprecision(1) << std::setw(4) << this->decode()
           << cs_inv_rst << "C "
           << "(0x" << cs_inv << std::setw(2) << std::setfill('0') << std::hex
           << static_cast<int>(this->raw) << cs_inv_rst << ")" << sep;
        return cs.str();
    }

    /**
     * @brief Decode the temperature from the structure.
     * @return The decoded temperature.
     */
    float decode() const {
        return static_cast<float>(this->integer) - 30.0f +
               (static_cast<bool>(this->decimal) ? 0.5f : 0.0f);
    }

    bool operator==(const struct temperature_extended& other) const {
        return this->raw == other.raw;
    }
    bool operator!=(const struct temperature_extended& other) const { return !(*this == other); }

    bool operator==(const optional<struct temperature_extended>& other) const {
        return other.has_value() && *this == *other;
    }
    bool operator!=(const optional<struct temperature_extended>& other) const {
        return !(*this == other);
    }

    struct temperature_extended& operator=(uint8_t value) {
        this->raw = value;
        return *this;
    }
    struct temperature_extended& operator=(float value) {
        this->encode(value);
        return *this;
    }
} __attribute__((packed)) temperature_extended_t;

static_assert(sizeof(temperature_extended_t) == 1, "sizeof(temperature_extended_t) != 1");



/**
 * @brief Represents a large integer number with 16 bits.
 *
 * This structure holds large integer number representations from raw integer bytes exchanged with the heat pump.
 * - raw: The raw 16-bit representation of the number.
 */
typedef struct large_integer {
    uint16_t raw;

    // Constructor for initialization
    large_integer() : raw(0) {}
    large_integer(uint16_t value) { this->encode(value); }
    large_integer& operator=(uint16_t value) {
        this->encode(value);
        return *this;
    }
    large_integer& operator=(float value) {
        this->encode(static_cast<uint16_t>(std::round(value)));
        return *this;
    }

    // Encode function to handle endianness and store a uint16_t value
    /**
     * @brief Encode the given uint16_t value into the raw byte representation.
     *
     * This function handles endianness and stores the given uint16_t value into
     * the raw byte representation.
     *
     * @param value The uint16_t value to encode.
     */
    void encode(uint16_t value) {
        // Reverse byte order for encoding (little-endian to big-endian or vice versa)
        this->raw = (value >> 8) | (value << 8);
    }

    // Decode function to retrieve the original uint16_t value
    /**
     * @brief Decode the raw byte representation into the original uint16_t value.
     *
     * This function handles endianness and retrieves the original uint16_t value
     * from the raw byte representation.
     *
     * @return The decoded uint16_t value.
     */
    uint16_t decode() const {
        // Reverse byte order for decoding
        return (this->raw >> 8) | (this->raw << 8);
    }

    // Format the decoded value into a human-readable string with optional separator
    /**
     * @brief Format the decoded value into a human-readable string with an optional separator.
     *
     * This function formats the decoded value into a human-readable string with
     * an optional separator.
     *
     * @param sep The separator to be used in the formatted string. Default is "".
     *
     * @return The formatted string.
     */
    std::string format(const char* sep = "") const {
        std::ostringstream oss;
        oss << decode(); // Convert the decoded value into string
        oss << " (0x" << std::hex << std::setw(4) << std::setfill('0') << this->raw << ")";
        oss << sep;
        return oss.str();
    }

    // Compare two large_integer values and highlight differences
    /**
     * @brief Compare two large_integer values and highlight differences in a human-readable string.
     *
     * This function compares two large_integer values and highlights differences
     * in a human-readable string.
     *
     * @param reference The reference large_integer value to compare against.
     * @param sep The separator to be used in the formatted string. Default is "".
     *
     * @return The formatted string that highlights differences.
     */
    std::string diff(const large_integer& reference, const char* sep = "") const {
        bool changed = this->raw != reference.raw;
        std::ostringstream oss;
        if (changed) {
            oss << "\033[1;31m"; // ANSI escape code for red color to highlight differences
        }
        // Compare and format the value
        oss << decode();
        if (changed) {
            oss << "\033[0m"; // Reset formatting
        }

        oss << sep;
        return oss.str();
    }

    // Operator overloads for comparison
    bool operator==(const large_integer& other) const { return this->raw == other.raw; }
    bool operator==(uint16_t other) const { return this->raw == other; }
    bool operator!=(const large_integer& other) const { return this->raw != other.raw; }

    // Assignment operator to set value

} __attribute__((packed)) large_integer_t;

/**
 * @brief Represents a decimal number with an integer and fractional part.
 *
 * This structure holds decimal number representations from raw decimal bytes exchanged with the heat pump.
 * - decimal: 0 means no decimal part, 1 means there is a 0.5 decimal part.
 * - integer: The integer part of the number, represented with 6 bits.
 * - negative: If 1, the number is negative, otherwise, it's positive.
 */
typedef struct decimal_number {
    union {
        struct {
            uint8_t decimal : 1;  // 0 = whole number, 1 = half (0.5)
            uint8_t integer : 6;  // Integer part of the decimal number
            uint8_t negative : 1; // 1 = negative, 0 = positive
        };
        uint8_t raw; // Raw byte representation
    };

    /**
     * @brief Format the decimal number as a string.
     *
     * Formats the decimal number as a string with the given separator.
     *
     * @param sep The separator to be used in the formatted string. Default is "".
     * @return The formatted string.
     */
    std::string format(const char* sep = "") const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << std::setw(2) << this->decode();
        oss << "(0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(this->raw)
            << ")";
        oss << sep;
        return oss.str();
    }

    /**
     * @brief Compare and format the difference between two decimal numbers.
     *
     * This function compares the current decimal number with a reference number
     * and formats the difference using the provided separator.
     *
     * @param reference The reference decimal number to compare against.
     * @param sep The separator to be used in the formatted string. Default is "".
     * @return The formatted difference string.
     */
    std::string diff(const struct decimal_number& reference, const char* sep = "") const {
        CS cs;
        bool changed = this->raw != reference.raw;
        cs.set_changed_base_color(changed);

        auto cs_inv = changed ? CS::invert : "";
        auto cs_inv_rst = changed ? CS::invert_rst : "";

        cs << cs_inv << std::fixed << std::setprecision(1) << std::setw(2) << this->decode()
           << cs_inv_rst << "(0x" << cs_inv << std::setw(2) << std::setfill('0') << std::hex
           << static_cast<int>(this->raw) << cs_inv_rst << ")" << sep;
        return cs.str();
    }

    /**
     * @brief Encode a float value into the decimal number representation.
     *
     * This function encodes a float value into the internal representation,
     * handling negative numbers by flipping the negative bit.
     *
     * @param decimal_value The float value to encode.
     */
    void encode(float decimal_value) {
        this->negative = decimal_value < 0 ? 1 : 0;
        this->integer = static_cast<uint8_t>(std::abs(decimal_value));
        this->decimal = (fabs(decimal_value - static_cast<int>(decimal_value)) >= 0.5f) ? 1 : 0;
    }

    /**
     * @brief Decode the decimal number to a float value.
     *
     * This function decodes the internal representation to a float value,
     * considering the integer, decimal, and negative components.
     *
     * @return The decoded float value.
     */
    float decode() const {
        return static_cast<float>(this->integer) * (static_cast<bool>(this->negative) ? -1 : 1) +
               (static_cast<bool>(this->decimal) ? 0.5f : 0.0f);
    }

    // Operator overloads for comparison
    bool operator==(const struct decimal_number& other) const { return this->raw == other.raw; }
    bool operator!=(const struct decimal_number& other) const { return !(*this == other); }

    bool operator==(const optional<struct decimal_number>& other) const {
        return other.has_value() && *this == *other;
    }
    bool operator!=(const optional<struct decimal_number>& other) const {
        return !(*this == other);
    }

    // Assignment operators for setting value
    struct decimal_number& operator=(uint8_t value) {
        this->raw = value;
        return *this;
    }
    struct decimal_number& operator=(float value) {
        this->encode(value);
        return *this;
    }
} __attribute__((packed)) decimal_number_t;

/**
 * @brief Structure to represent a short frame (9 bytes) that holds data from short packets exchanged with the heat pump.
 *
 * This structure holds the data from short packets types exchanged with the heat pump.
 */
typedef struct {
    union {
        uint8_t raw[frame_data_length_short]; // Raw byte representation of the short frame
        union {
            uint8_t frame_type; // Common field: frame type for short frames
            uint8_t payload[frame_data_length_short - 2]; // Payload data of the short frame
            uint8_t checksum; // Checksum byte at data[8] for short frames
        };
    };

    /**
     * @brief Calculates the checksum for the given length of the payload.
     *
     * This function calculates the checksum for the given length of the payload.
     * It loops over the range [start_index, length - 1) and adds up the bytes.
     * Then it returns the checksum (modulo 256).
     *
     * @param length Length of the payload.
     * @return The calculated checksum.
     */
    uint8_t calculate_checksum(size_t length = (frame_data_length_short - 1)) const {
        unsigned int total = 0;

        // Loop over the range [start_index, length - 1)
        for (size_t i = 1; i < length - 1; ++i) {
            total += this->payload[i];
        }
        // Return the checksum (modulo 256)
        return total % 256;
    }

    /**
     * @brief Explains the checksum calculation for the given length of the payload.
     *
     * This function explains the checksum calculation for the given length of the payload.
     * It loops over the range [start_index, length - 1) and adds up the bytes.
     * Then it returns the calculated checksum (modulo 256) and the raw bytes used for the calculation.
     *
     * @param length Length of the payload.
     * @return The calculated checksum and the raw bytes used for the calculation.
     */
    std::string explain_checksum(size_t length = (frame_data_length_short - 1)) const {
        unsigned int total = 0;
        std::ostringstream oss;

        for (size_t i = 1; i < length - 1; ++i) {
            total += this->raw[i];
            oss << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(this->raw[i])
                << std::dec << "(" << (total) << "),";
        }
        // Return the checksum (modulo 256)
        oss << "calculated:" << std::setw(2) << std::setfill('0') << std::hex
            << static_cast<int>(total % 256) << " from struct " << std::setw(2) << std::setfill('0')
            << std::hex << static_cast<int>(this->raw[8]);
        return oss.str();
    }

} __attribute__((packed)) short_frame_t;

static_assert(sizeof(short_frame_t) == 9, "Invalid packet data size for short_frame_t.");

/**
 * @brief Structure to represent a long frame that holds data from long packets exchanged with the heat pump.
 *
 * This structure holds the data from long packet types exchanged with the heat pump.
 */
typedef struct {
    union {
        uint8_t raw[frame_data_length]; // Raw byte representation of the long frame
        union {
            uint8_t frame_type; // Common field: frame type for long frames
            uint8_t payload[frame_data_length - 2]; // Payload data of the long frame
            uint8_t checksum; // Checksum byte at data[11] for long frames
        };
    };

    /**
     * @brief Calculates the checksum for the given length of the payload.
     *
     * This function calculates the checksum for the given length of the payload.
     * It loops over the range [0, length - 1) and adds up the bytes.
     * Then it returns the checksum (modulo 256).
     *
     * @param length Length of the payload.
     * @return The calculated checksum.
     */
    uint8_t calculate_checksum(size_t length = sizeof(payload) + sizeof(frame_type)) const {
        unsigned int total = 0;

        for (size_t i = 0; i < length - 1; ++i) {
            total += this->raw[i];
        }
        // Return the checksum (modulo 256)
        return total % 256;
    }

    /**
     * @brief Explains the checksum calculation for the given length of the payload.
     *
     * This function explains the checksum calculation for the given length of the payload.
     * It loops over the range [0, length - 1) and adds up the bytes.
     * Then it returns the calculated checksum (modulo 256) and the raw bytes used for the calculation.
     *
     * @param length Length of the payload.
     * @return The calculated checksum and the raw bytes used for the calculation.
     * @see calculate_checksum
     * 
     * @note This function is useful when troubleshooting checksum issues.
     */
    std::string explain_checksum(size_t length = sizeof(payload) + sizeof(frame_type)) const {
        unsigned int total = 0;
        std::ostringstream oss;

        for (size_t i = 0; i < length - 1; ++i) {
            total += this->raw[i];
        }
        // Return the checksum (modulo 256)
        oss << "calculated:" << std::setw(2) << std::setfill('0') << std::hex
            << static_cast<int>(total % 256) << " from struct " << std::setw(2) << std::setfill('0')
            << std::hex << static_cast<int>(this->checksum);
        return oss.str();
    }

} __attribute__((packed)) long_frame_t;

static_assert(sizeof(long_frame_t) == frame_data_length, " long_frame_t length is wrong ");

static_assert(offsetof(long_frame_t, checksum) != 1, "checksum is at position 1");
static_assert(offsetof(long_frame_t, checksum) != 2, "checksum is at position 2");
static_assert(offsetof(long_frame_t, checksum) != 3, "checksum is at position 3");
static_assert(offsetof(long_frame_t, checksum) != 4, "checksum is at position 4");
static_assert(offsetof(long_frame_t, checksum) != 5, "checksum is at position 5");
static_assert(offsetof(long_frame_t, checksum) != 6, "checksum is at position 6");
static_assert(offsetof(long_frame_t, checksum) != 7, "checksum is at position 7");
static_assert(offsetof(long_frame_t, checksum) != 8, "checksum is at position 8");
static_assert(offsetof(long_frame_t, checksum) != 9, "checksum is at position 9");
static_assert(offsetof(long_frame_t, checksum) != 10, "checksum is at position 10");
static_assert(offsetof(long_frame_t, checksum) != 11, "checksum is at position 11");
static_assert(offsetof(long_frame_t, checksum) != 12, "checksum is at position 12");
static_assert(offsetof(long_frame_t, checksum) != 13, "checksum is at position 13");
static_assert(offsetof(long_frame_t, checksum) != 14, "checksum is at position 14");

// Define the overarching hp_packetdata_t structure
typedef struct packet_data {
    union {
        uint8_t data[frame_data_length];
        union {
            short_frame_t short_f;
            long_frame_t long_f;
        };
    };
    uint8_t data_len;

    /// @brief Get the type of the frame.
    ///
    /// @return The frame type of the packet, either from the short or long frame structure.
    ///
    uint8_t get_type() const { return this->data[0]; }

    /// @brief Get a reference to the checksum byte of the packet.
    ///
    /// @return A reference to the checksum byte of the packet.
    ///
    uint8_t& get_checksum_byte_ref() {
        uint8_t position = this->data_len >= sizeof(data) ? sizeof(data) - 1 : this->data_len - 1;
        return this->data[position];
    }

    uint8_t get_checksum_pos() const {
        uint8_t position = this->data_len >= sizeof(data) ? sizeof(data) - 1 : this->data_len - 1;
        return position;
    }

    /// @brief Get a reference to the checksum byte of the packet.
    ///
    /// @return A reference to the checksum byte of the packet.
    ///
    const uint8_t& get_checksum_byte_ref() const { return this->data[this->get_checksum_pos()]; }

    /// @brief Check if the packet is a short frame.
    ///
    /// @return `true` if the packet is a short frame, `false` otherwise.
    inline bool is_short_frame() const { return this->data_len == frame_data_length_short; }

    /// @brief Get the checksum of the packet as sent on the bus.
    ///
    /// The checksum is the last byte of the packet, which is the same as the
    /// checksum calculated by the bus (i.e. the last byte of a short frame
    /// or the last byte of a long frame).
    ///
    /// @return The checksum of the packet as sent on the bus.
    uint8_t bus_checksum() const { return this->data[this->get_checksum_pos()]; }
    /// @brief Calculate the checksum of the packet.
    ///
    /// The checksum is the last byte of the packet, which is the same as the
    /// checksum calculated by the bus (i.e. the last byte of a short frame
    /// or the last byte of a long frame).
    ///
    /// @return The checksum of the packet.
    uint8_t calculate_checksum() const {
        if (is_short_frame()) {
            return this->short_f.calculate_checksum(this->data_len);
        } else {
            return this->long_f.calculate_checksum(this->data_len);
        }
    }

    /// @brief Returns a string explaining how the checksum was calculated.
    ///
    /// This function is useful for debugging checksum issues. It returns a
    /// string explaining how the checksum was calculated, which can be
    /// useful for understanding why a particular checksum is not valid.
    ///
    /// @return A string explaining how the checksum was calculated.
    std::string explain_checksum() const {
        if (is_short_frame()) {
            return this->short_f.explain_checksum(this->data_len);
        } else {
            return this->long_f.explain_checksum(this->data_len);
        }
    }

    /// @brief Sets the checksum byte of the frame to the calculated value.
    ///
    /// This function calculates the checksum using the
    /// `calculate_checksum()` function and then sets the checksum byte
    /// stored in the frame to this calculated value.
    void set_checksum() { get_checksum_byte_ref() = this->calculate_checksum(); }

    /**
     * @brief Determines if the checksum is valid
     * if the length of data doesn't match a short or a long frame
     * it returns false, otherwise it checks if the checksum is valid
     *
     * @return true if the checksum is valid, false otherwise
     */
    bool is_checksum_valid() const {
        if (this->data_len != frame_data_length_short && this->data_len != frame_data_length) {
            return false;
        }
        return (this->data[this->get_checksum_pos()] == this->calculate_checksum());
    }
    /**
     * @brief Reset the packet data structure to its initial state.
     *
     */
    void reset() {
        this->data_len = 0;
        memset(&this->data, 0x00, sizeof(data));
    }

    /**
     * @brief Converts the packet data to a structure of type T
     *
     * @tparam T  The target structure type
     * @param offset  the offset of the first byte in the target structure (default: 1)
     * @return The target structure
     */
    template <typename T> T as_type(size_t offset = 1) const {
        static_assert(sizeof(T) <= sizeof(data) - 1,
            "Target structure size exceeds packet data size when skipping the first byte");

        T frame;
        std::memcpy(&frame, this->data + offset, sizeof(T));
        return frame;
    }
    /**
     * @brief Initializes the packet data structure from a structure of type T
     *
     * @tparam T  the type of the source structure
     * @param frame  the source structure
     * @param offset  the offset of the first byte in the source structure (default: 1)
     */
    template <typename T> void from_type(const T& frame, size_t offset = 1) {
        static_assert(sizeof(T) <= sizeof(data) - 1,
            "Source structure size exceeds packet data size when skipping the first byte");

        std::memcpy(this->data + offset, &frame, sizeof(T));
        this->data_len = offset + sizeof(T) + 1; // Update data length
        this->set_checksum();
    }

    /**
     * @brief Convert the packet data to a type T
     *
     * @tparam T  the target type
     * @return The target type
     */
    template <typename T> operator T() const { return as_type<T>(); }

    /**
     * @brief Compares two packet data structures for equality.
     *
     * This function compares the data length and the contents of the data array
     * of two packet data structures. It returns true if they are equal, false
     * otherwise.
     *
     * @param other The other packet data structure to compare with.
     *
     * @return true if the data length and contents of the two packet data
     * structures are equal, false otherwise.
     *
     */
    bool operator==(const packet_data& other) const {
        return this->data_len == other.data_len &&
               memcmp(&this->data, &other.data, sizeof(data)) == 0;
    }
    bool operator!=(const packet_data& other) const { return !(*this == other); }
} __attribute__((packed)) hp_packetdata_t;

static_assert(
    sizeof(hp_packetdata_t) == 13, "Invalid hp_packetdata_t size"); // 12 + 1 byte for the length

#pragma pack(pop) // Restore default packing

/**
 * @brief This structure represents the data exchanged on the heat pump data bus.
 *
 * It is used to pass data between the low level specialized bus packets and the esphome pool heater
 * custom component.
 */
typedef struct {
    /// @brief optional time_t value
    /// This is the heat pump's clock value. The clock is typically used by the heat pump for
    /// operating modes which depend on the time, for example fan mode time.
    /// @see FanMode
    optional<std::time_t> time;

    /// @brief Heating mode restrictions
    /// This is the heat pump's heating mode restrictions. It can be cooling only, heating only, or
    /// no restrictions.
    /// @see HeatPumpRestrict
    optional<HeatPumpRestrict> mode_restrictions;

    /// @brief Fan mode
    /// This is the heat pump's fan mode.
    /// @see FanMode
    optional<FanMode> fan_mode;

    /// @brief Climate action
    /// This is the current activity being performed by the heat pump
    /// @see climate::ClimateAction
    optional<climate::ClimateAction> action;

    /// @brief Climate mode that is requested from the heat pump
    /// @see climate::ClimateMode
    optional<climate::ClimateMode> mode;

    /// @brief Start defrost temperature in degrees Celsius.
    /// Represents the temperature threshold at which the heat pump initiates the defrost cycle.
    /// The condition must be sustained for the time specified in
    /// `d03_defrosting_cycle_time_minutes` before the defrost cycle begins.
    optional<float> d01_defrost_start;

    /// @brief End defrost temperature in degrees Celsius.
    /// Establishes the temperature threshold above which the defrost cycle ends,
    /// allowing the system to return to normal heating mode.
    optional<float> d02_defrost_end;

    /// @brief Defrost cycle time in minutes.
    /// Represents the required delay (in minutes) between two successive defrost cycles.
    /// For the first defrost initiation, the coil temperature must remain below `d01_defrost_start`
    /// for the entire duration of `d03_defrosting_cycle_time_minutes`.
    optional<float> d03_defrosting_cycle_time_minutes;

    /// @brief Maximum defrost duration in minutes.
    /// Represents the maximum allowable duration for a single defrost cycle. The defrost cycle
    /// will end when this time is reached, even if the temperature threshold (`d02_defrost_end`)
    /// has not been met.
    ///
    /// Special scenarios for abnormal defrost termination:
    /// 1) If the unit is shut off during defrosting, the system will complete the defrost cycle
    /// before stopping.
    ///
    /// 2) If the high-pressure (HP) switch malfunctions during defrost, the unit
    /// will shut down and indicate an HP malfunction. Upon recovery, the system will return to
    /// normal heating mode.
    ///
    /// 3) If the low-pressure (LP) switch malfunctions, the unit skips the LP fault and continues
    /// defrosting. After defrosting completes and normal heating resumes, the LP switch will be
    /// re-checked after 5 minutes.
    ///
    /// 4) If the flow switch malfunctions, the unit will shut off and indicate a Flow malfunction.
    /// After recovering, the system will continue defrosting until `d04_max_defrost_time_minutes`
    /// elapses.
    ///
    /// 5) If the exhaust temperature is too high, the unit will shut down to indicate the issue.
    /// After recovery, defrosting resumes until the maximum defrost time (`d04`) is reached.
    ///
    /// 6) If the temperature difference between inlet and outlet is too high during defrosting,
    /// the unit will shut down and show a malfunction. Once recovered, the system will continue
    /// defrosting until the maximum defrost time is reached.
    ///
    /// 7) If antifreeze protection is triggered during defrosting, the unit will shut down and show
    /// a malfunction. After recovery, defrosting will continue until the maximum defrost time
    /// (`d04`) is reached.
    optional<float> d04_max_defrost_time_minutes;

    /// @brief Minimum defrost time in minutes in economy mode.
    /// Defines the minimum duration for the defrost cycle when operating in defrost economy mode,
    /// ensuring the defrost cycle achieves effective operation under energy-saving constraints.
    /// @see DefrostEcoMode
    optional<float> d05_min_economy_defrost_time_minutes;

    /// @brief Defrost economy mode.
    /// Configures the heat pump's economy mode for defrosting, allowing for reduced energy usage
    /// when certain operational thresholds are met, typically extending the defrost cycle under
    /// less demanding conditions.
    /// @see DefrostEcoMode
    optional<DefrostEcoMode> d06_defrost_eco_mode;

    /// @brief Suction temperature in degrees Celsius.
    /// Represents the temperature of the refrigerant entering the compressor, aiding in assessing
    /// cooling performance.
    /// @todo suction temperature position in the buffer has to be determined
    optional<float> t01_temperature_suction;

    /// @brief Inlet water temperature in degrees Celsius.
    /// Measures the temperature of the water entering the heat pump, determining the heating
    /// demand.
    optional<float> t02_temperature_inlet;

    /// @brief Outlet water temperature in degrees Celsius.
    /// Measures the temperature of the water exiting the heat pump, reflecting heat transfer
    /// effectiveness.
    /// @todo outlet temperature position in the buffer has to be determined
    optional<float> t03_temperature_outlet;

    /// @brief Coil temperature in degrees Celsius.
    /// Indicates the temperature of the evaporator or condenser coil, helping assess the efficiency
    /// of heat transfer.
    optional<float> t04_temperature_coil;

    /// @brief Ambient temperature in degrees Celsius.
    /// Reflects the external air temperature around the heat pump, impacting efficiency and
    /// operational adjustments.
    /// @todo ambient temperature position in the buffer needs to be confirmed
    optional<float> t05_temperature_ambient;

    /// @brief Exhaust temperature in degrees Celsius.
    /// Represents the temperature of the refrigerant or air exiting the compressor or heat
    /// exchanger, indicating heat pump output performance.
    optional<float> t06_temperature_exhaust;

    /// @brief Water flow meter status. If true, the flow meter is enabled and the heat pump can
    /// operate.
    /// @see FlowMeterEnable
    optional<FlowMeterEnable> S02_water_flow;

    /// @brief Target temperature in degrees Celsius. This is the desired temperature that the heat
    /// pump should maintain.
    optional<float> target_temperature;

    /// @brief Minimum target temperature in degrees Celsius. This is the lowest temperature that
    /// the heat pump can maintain in cooling mode.
    optional<float> min_target_temperature;

    /// @brief Maximum target temperature in degrees Celsius. This is the highest temperature that
    /// the heat pump can maintain in heating mode.
    optional<float> max_target_temperature;

    /// @brief Cooling setpoint temperature in degrees Celsius. This is the temperature at which the
    /// heat pump starts cooling.
    optional<float> r01_setpoint_cooling;

    /// @brief Heating setpoint temperature in degrees Celsius. This is the temperature at which the
    /// heat pump starts heating.
    optional<float> r02_setpoint_heating;

    /// @brief Auto mode setpoint in degrees Celsius. This is the temperature at which the heat pump
    /// operates in auto mode.
    optional<float> r03_setpoint_auto;

    /// @brief Temperature difference to maintain during cooling operation in degrees Celsius.
    /// This value represents the temperature difference between the setpoint and the actual water
    /// temperature that the heat pump strives to maintain while cooling. It's used to ensure
    /// efficient cooling performance.
    optional<float> r04_return_diff_cooling;

    /// @brief Temperature difference that triggers shutdown when cooling in degrees Celsius.
    /// This value defines the temperature difference threshold that, when exceeded, causes the heat
    /// pump to shut down the cooling operation to prevent overcooling. It's a safety feature to
    /// protect the system and maintain desired conditions.
    optional<float> r05_shutdown_temp_diff_when_cooling;

    /// @brief Temperature difference to maintain during heating operation in degrees Celsius.
    /// This value represents the temperature difference between the setpoint and the actual water
    /// temperature that the heat pump strives to maintain while heating. It's used to ensure
    /// efficient heating performance.
    optional<float> r06_return_diff_heating;

    /// @brief Temperature difference that triggers shutdown when heating in degrees Celsius.
    /// This value defines the temperature difference threshold that, when exceeded, causes the heat
    /// pump to shut down the heating operation to prevent overheating. It's a safety feature to
    /// protect the system and maintain desired conditions.
    optional<float> r07_shutdown_diff_heating;

    /// @brief Minimum cooling setpoint in degrees Celsius. This is the lowest temperature that the
    /// heat pump can maintain in cooling mode.
    optional<float> r08_min_cool_setpoint;

    /// @brief Maximum cooling setpoint in degrees Celsius. This is the highest temperature that the
    /// heat pump can maintain in cooling mode.
    optional<float> r09_max_cooling_setpoint;

    /// @brief Minimum heating setpoint in degrees Celsius. This is the lowest temperature that the
    /// heat pump can maintain in heating mode.
    optional<float> r10_min_heating_setpoint;

    /// @brief Maximum heating setpoint in degrees Celsius. This is the highest temperature that the
    /// heat pump can maintain in heating mode.
    optional<float> r11_max_heating_setpoint;

    /// @brief Last heater frame
    optional<uint32_t> last_heater_frame;

    /// @brief Last controller frame
    optional<uint32_t> last_controller_frame;

    /// @brief Indicates if a Flow Meter is installed and enabled
    /// @see FlowMeterEnable
    optional<FlowMeterEnable> U01_flow_meter;

    /// @brief Flow meter pulses per liter
    /// @see FlowMeterEnable
    optional<float> U02_pulses_per_liter;

    /**
     * @brief Determines if a given temperature is within the valid range
     *
     * The valid range is determined by the heat pump's operating mode
     *
     * @see esphome::climate::ClimateMode
     * @param temp  Temperature in degrees Celsius
     * @return true
     * @return false
     */
    bool is_temperature_valid(float temp) const {
        return temp >= this->get_min_target() && temp <= this->get_max_target();
    }

    /**
     * @brief Get the min target value in degrees Celsius
     *
     * The min value is determined by the heat pump's operating mode, each of which
     * having a different min value
     *
     * @see esphome::climate::ClimateMode
     *
     * @return float
     */
    float get_min_target() const { return this->min_target_temperature.value_or(15); }

    /**
     * @brief Get the max target value in degrees Celsius
     *
     * The max value is determined by the heat pump's operating mode, each of which
     * having a different max value
     *
     * @see esphome::climate::ClimateMode
     *
     * @return float
     */
    float get_max_target() const { return this->max_target_temperature.value_or(33); }
} heat_pump_data_t;

} // namespace hwp
} // namespace esphome
