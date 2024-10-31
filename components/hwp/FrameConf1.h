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
#include "esphome/components/climate/climate.h"
#include "esphome/components/climate/climate_mode.h"
static constexpr char TAG_TEMP[] = "hwp";
namespace esphome {
namespace hwp {

/**
 * @brief Heating pump mode structure
 *
 * - power: Indicates whether the heating pump is powered on or off.
 * - h02_mode: Represents H02 mode using the HeatPumpRestrict:
 *   - 0: Cooling only (MODE_RESTRICT_COOLING).
 *   - 1: Heating, cooling, and automatic modes (MODE_ANY).
 *   - 2: Heating only (MODE_RESTRICT_HEATING).
 * - heat: Enables heating mode.
 * - auto_mode: Enables automatic mode selection.
 * - unknown_4 and unknown_5: These bits are unknown at this time.
 */
typedef struct hp_mode {
    union {
        struct {
            uint8_t power : 1;
            bool h02_unknown : 1;
            bool h02_enable_auto : 1;
            bool h02_heating_only : 1;
            uint8_t heat : 1;      // Enables heating mode
            uint8_t auto_mode : 1; // Enables automatic mode
            uint8_t unknown_4 : 1;
            uint8_t unknown_5 : 1;
        };

        bits_details_t raw;
    };
    HeatPumpRestrict get_mode_restriction() const {
        return this->h02_heating_only  ? HeatPumpRestrict::Heating
               : this->h02_enable_auto ? HeatPumpRestrict::Any
                                       : HeatPumpRestrict::Cooling;
    }
    void set_mode_restriction(const HeatPumpRestrict& mode) {
        this->h02_heating_only = mode == HeatPumpRestrict::Heating;
        this->h02_enable_auto = mode == HeatPumpRestrict::Any;
    }
    bool operator==(const struct hp_mode& other) const { return raw == other.raw; }
    bool operator!=(const struct hp_mode& other) const { return !(*this == other); }
    bool operator==(const optional<hp_mode>& other) const {
        return other.has_value() && *this == *other;
    }
    bool operator!=(const optional<hp_mode>& other) const { return !(*this == other); }
    operator std::string() const { return get_mode_restriction().to_string(); }
    const char* log_format() const { return get_mode_restriction().log_format(); }

} __attribute__((packed)) hp_mode_t;

static_assert(sizeof(hp_mode_t) == 1, "sizeof(hp_mode_t) != 1");
/**
 * @brief Holds the temperature program settings for cooling, heating, and auto modes.
 *
 * - mode: Mode structure for controlling power, heating, and cooling modes.
 * - r01_setpoint_cooling: Cooling setpoint temperature.
 * - r02_setpoint_heating: Heating setpoint temperature.
 * - r03_setpoint_auto: Setpoint for auto mode.
 * - r04_return_diff_cooling: Return temperature difference during cooling.
 * - r05_shutdown_temp_diff_when_cooling: Shutdown temperature difference when cooling.
 * - r06_return_diff_heating: Return temperature difference during heating.
 * - r07_shutdown_diff_heating: Shutdown temperature difference when heating.
 * - reserved_7: Reserved for future or unknown use.
 */
typedef struct conf_1 {
    uint8_t id;                         // Frame ID
    hp_mode_t mode;                     // Mode structure at data[2]
    temperature_t r01_setpoint_cooling; // Cooling setpoint temperature
    temperature_t r02_setpoint_heating; // Heating setpoint temperature at data[4]
    temperature_t r03_setpoint_auto;    // Auto mode setpoint
    temperature_extended_t
        r04_return_diff_cooling; // low_range. Return temperature difference for cooling
    temperature_extended_t r05_shutdown_temp_diff_when_cooling; // low_range. Shutdown temperature
                                                                // difference when cooling
    temperature_extended_t
        r06_return_diff_heating; // low_range. Return temperature difference for heating
    temperature_extended_t
        r07_shutdown_diff_heating; // low_range. Shutdown temperature difference when heating
    bits_details_t reserved_7;     // Reserved for future use

    bool operator==(const optional<struct conf_1>& other) const {
        return other.has_value() && *this == other.value();
    }
    bool operator==(const struct conf_1& other) const {
        return mode == other.mode && r01_setpoint_cooling == other.r01_setpoint_cooling &&
               r02_setpoint_heating == other.r02_setpoint_heating &&
               r03_setpoint_auto == other.r03_setpoint_auto &&
               r04_return_diff_cooling == other.r04_return_diff_cooling &&
               r05_shutdown_temp_diff_when_cooling == other.r05_shutdown_temp_diff_when_cooling &&
               r06_return_diff_heating == other.r06_return_diff_heating &&
               r07_shutdown_diff_heating == other.r07_shutdown_diff_heating &&
               reserved_7 == other.reserved_7;
    }
    bool operator!=(const struct conf_1& other) const { return !(*this == other); }
    bool operator!=(const optional<struct conf_1>& other) const { return !(*this == other); }
} __attribute__((packed)) conf_1_t;
static_assert(sizeof(conf_1_t) == frame_data_length - 2, "frame structure has wrong size");

/**
 * @class FrameConf1
 * @brief A class to handle command frames in the Heat Pump Controller.
 *
 * This class extends the BaseFrame class to include additional functionality
 * specific to command frames.
 */
class FrameConf1 : public BaseFrame {
  public:
    CLASS_DEFAULT_IMPL(FrameConf1, conf_1_t);
    void reset();
    void set_target_cooling(float temperature);
    void set_target_heating(float temperature);
    void set_target_auto(float temperature);
    climate::ClimateMode get_climate_mode() const;
    active_modes_t get_active_mode() const;
    static active_modes_t get_active_mode(const FrameConf1& frame);
    bool is_power_on() const;
    void get_min_max_temperature(float& min_temp, float& max_temp) const;
    static bool is_power_on(hp_mode_t mode);
    static active_modes_t get_active_mode(hp_mode_t mode);
    const char* get_active_mode_desc() const;
    static const char* get_active_mode_desc(active_modes_t state);
    float get_target_temperature() const;
    static const char* get_power_mode_desc(bool power);

    void set_mode(climate::ClimateMode mode);
    static constexpr uint8_t FRAME_ID_CONF_1 = 0x81; // B 10000001 (inv: 10000001)
    void traits(climate::ClimateTraits& traits, heat_pump_data_t& hp_data) override;
};

} // namespace hwp
} // namespace esphome
