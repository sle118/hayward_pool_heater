/**
 * @file PoolHeater.h
 * @brief Defines the PoolHeater class for handling communication with the heat pump.
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
#include "Bus.h"
#include "HeaterStatus.h"
#include "base_frame.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/core/application.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/macros.h"

/**
 * @brief Describes the structure and timing of packets on the NET port.
 *
 * There are two types of packets on the NET port:
 * Type A, from Heat pump:
 *      - Sent in groups of 4
 *      - 152ms space between frames, with line held HIGH
 *      - 1s space between groups
 *      - Binary 0 is 3ms, Binary 1 is 1ms
 *      - 12 bytes length
 *
 * Type B, from Keypad/Controller:
 *      - Sent in groups of 8 (to be confirmed)
 *      - 12 bytes length
 *      - 101ms space between frames, with line held HIGH
 *      - After 2 seconds, Type A frames resume
 *      - Even with no change, this frame type is sent by the keypad/controller every minute
 *      - Binary 1 is 3ms, Binary 0 is 1ms (reverse of Type A)
 *
 * In both cases, a start of frame is represented by 9ms LOW followed by 5ms HIGH.
 */

namespace esphome {
namespace hwp {

#define FRAME_TEMP_MASK 0B00111110
#define FRAME_TEMP_HALF_DEG_BIT 7
#define FRAME_STATE_POWER_BIT 7
#define FRAME_MODE_AUTO_BIT 2
#define FRAME_MODE_HEAT_BIT 3

// big endian (e.g. bits reversed from the bus)
#define FRAME_TEMP_MASK_BE 0B01111100
#define FRAME_TEMP_HALF_DEG_BIT_BE 0
#define FRAME_STATE_POWER_BIT_BE 0
#define FRAME_MODE_AUTO_BIT_BE 5
#define FRAME_MODE_HEAT_BIT_BE 4

#define FRAME_CHECKSUM_POS (FRAME_SIZE - 1)

#define DEFAULT_POWERMODE_MASK 0B01100000
#define DEFAULT_POWERMODE_MASK_BE 0B00000110
#define DEFAULT_TEMP_MASK 0B0010
#define DEFAULT_TEMP_MASK_BE 0B0100

#define HP_MODE_COOL B00000000
#define HP_MODE_HEAT B00001000
#define HP_MODE_AUTO B00000100

static constexpr uint8_t POOLHEATER_TEMP_MIN = 15;
static constexpr uint8_t POOLHEATER_TEMP_MAX = 33;
extern const char* POOL_HEATER_TAG;

/**
 * @brief Class to handle communication with the pool heater.
 */
class PoolHeater : public climate::Climate, public PollingComponent {
  public:
    PoolHeater(InternalGPIOPin* gpio_pin);
    void update() override;
    void set_out_temperature_sensor(sensor::Sensor* sensor);
    void set_actual_status_sensor(text_sensor::TextSensor* sensor);
    void set_heater_status_code_sensor(text_sensor::TextSensor* sensor);
    void set_heater_status_description_sensor(text_sensor::TextSensor* sensor);
    void set_heater_status_solution_sensor(text_sensor::TextSensor* sensor);
    void set_suction_temperature_T01_sensor(sensor::Sensor* sensor) {
        this->t01_temperature_suction_ = sensor;
    }
    void set_outlet_temperature_T03_sensor(sensor::Sensor* sensor) {
        this->t03_temperature_outlet_ = sensor;
    }
    
    void set_coil_temperature_T04_sensor(sensor::Sensor* sensor) {
        this->t04_temperature_coil_ = sensor;
    }

    void set_ambient_temperature_T05_sensor(sensor::Sensor* sensor) {
        this->t05_temperature_ambient_ = sensor;
    }

    void set_exhaust_temperature_T06_sensor(sensor::Sensor* sensor) {
        this->t06_temperature_exhaust_ = sensor;
    }

    // Generic temperature sensor setters

    void set_d01_defrost_start_sensor(number::Number* sensor) { this->d01_defrost_start_ = sensor; }
    void set_d02_defrost_end_sensor(number::Number* sensor) { this->d02_defrost_end_ = sensor; }
    void set_d03_defrosting_cycle_time_minutes_sensor(number::Number* sensor) {
        this->d03_defrosting_cycle_time_minutes_ = sensor;
    }
    void set_d04_max_defrost_time_minutes_sensor(number::Number* sensor) {
        this->d04_max_defrost_time_minutes_ = sensor;
    }
    void set_h02_mode_restrictions_sensor(select::Select* sensor) {
        this->h02_mode_restrictions_ = sensor;
    }

    void set_s02_water_flow_sensor(binary_sensor::BinarySensor* sensor) {
        this->s02_water_flow_ = sensor;
    }
    // void set_min_target_temperature_sensor(sensor::Sensor* sensor) {
    //     this->min_target_temperature_ = sensor;
    // }
    // void set_max_target_temperature_sensor(sensor::Sensor* sensor) {
    //     this->max_target_temperature_ = sensor;
    // }
    void set_r01_setpoint_cooling_sensor(sensor::Sensor* sensor) {
        this->r01_setpoint_cooling_ = sensor;
    }
    void set_r02_setpoint_heating_sensor(sensor::Sensor* sensor) {
        this->r02_setpoint_heating_ = sensor;
    }
    void set_r03_setpoint_auto_sensor(sensor::Sensor* sensor) { this->r03_setpoint_auto_ = sensor; }
    void set_r04_return_diff_cooling_sensor(number::Number* sensor) {
        this->r04_return_diff_cooling_ = sensor;
    }
    void set_r05_shutdown_temp_diff_when_cooling_sensor(number::Number* sensor) {
        this->r05_shutdown_temp_diff_when_cooling_ = sensor;
    }
    void set_r06_return_diff_heating_sensor(number::Number* sensor) {
        this->r06_return_diff_heating_ = sensor;
    }
    void set_r07_shutdown_diff_heating_sensor(number::Number* sensor) {
        this->r07_shutdown_diff_heating_ = sensor;
    }
    void set_r08_min_cool_setpoint_sensor(sensor::Sensor* sensor) {
        this->r08_min_cool_setpoint_ = sensor;
    }
    void set_r09_max_cooling_setpoint_sensor(sensor::Sensor* sensor) {
        this->r09_max_cooling_setpoint_ = sensor;
    }
    void set_r10_min_heating_setpoint_sensor(sensor::Sensor* sensor) {
        this->r10_min_heating_setpoint_ = sensor;
    }
    void set_r11_max_heating_setpoint_sensor(sensor::Sensor* sensor) {
        this->r11_max_heating_setpoint_ = sensor;
    }
    void set_u01_flow_meter_sensor(select::Select* sensor) { this->u01_flow_meter_ = sensor; }
    void set_d06_defrost_eco_mode_sensor(select::Select* sensor) {
        this->d06_defrost_eco_mode_ = sensor;
    }
    void set_d05_min_economy_defrost_time_minutes_sensor(number::Number* sensor) {
        this->d05_min_economy_defrost_time_minutes_ = sensor;
    }
    void set_u02_pulses_per_liter_sensor(number::Number* sensor) {
        this->u02_pulses_per_liter_ = sensor;
    }

    /**
     * @brief Handle control requests from Home Assistant.
     * @param call The control call.
     */
    void control(const climate::ClimateCall& call) override;

    /**
     * @brief Get the climate traits.
     * @return The climate traits.
     */
    climate::ClimateTraits traits() override;

    HWPCall instantiate_call() {
        HWPCall hwpcall(
            climate::ClimateCall(this), *this, this->hp_data_, *this->actual_status_sensor_);
        return hwpcall;
    }
    /**
     * @brief Set passive mode.
     * @param passive True to enable passive mode, false otherwise.
     */
    void set_passive_mode(bool passive);
    void set_update_active(bool passive);
    bool get_passive_mode();
    bool is_update_active();
    heat_pump_data_t& data() { return hp_data_; }
    void control(const HWPCall& call);
    void generate_code();

  protected:
    heat_pump_data_t hp_data_;
    Bus driver_; ///< The bus driver for communication.
    HeaterStatus heater_status_;
    std::string actual_status_;
    bool passive_mode_ = true;
    bool update_active_ = false;
    text_sensor::TextSensor* actual_status_sensor_{nullptr};
    text_sensor::TextSensor* heater_status_code_sensor_{nullptr};
    text_sensor::TextSensor* heater_status_description_sensor_{nullptr};
    text_sensor::TextSensor* heater_status_solution_sensor_{nullptr};
    number::Number* d01_defrost_start_;
    number::Number* d02_defrost_end_;
    number::Number* d03_defrosting_cycle_time_minutes_;
    number::Number* d04_max_defrost_time_minutes_;
    select::Select* h02_mode_restrictions_;
    binary_sensor::BinarySensor* s02_water_flow_;
    // sensor::Sensor* min_target_temperature_;
    // sensor::Sensor* max_target_temperature_;
    sensor::Sensor* r01_setpoint_cooling_;
    sensor::Sensor* r02_setpoint_heating_;
    sensor::Sensor* r03_setpoint_auto_;
    number::Number* r04_return_diff_cooling_;
    number::Number* r05_shutdown_temp_diff_when_cooling_;
    number::Number* r06_return_diff_heating_;
    number::Number* r07_shutdown_diff_heating_;
    sensor::Sensor* r08_min_cool_setpoint_;
    sensor::Sensor* r09_max_cooling_setpoint_;
    sensor::Sensor* r10_min_heating_setpoint_;
    sensor::Sensor* r11_max_heating_setpoint_;
    select::Select* u01_flow_meter_;
    select::Select* d06_defrost_eco_mode_;
    number::Number* d05_min_economy_defrost_time_minutes_;
    number::Number* u02_pulses_per_liter_;

    // Specific temperature sensors
    sensor::Sensor* t01_temperature_suction_; ///< Suction temperature sensor (T01)
    sensor::Sensor* t04_temperature_coil_;           ///< Coil temperature sensor (T04)
    sensor::Sensor* t05_temperature_ambient_; ///< Ambient temperature sensor (T05)
    sensor::Sensor* t03_temperature_outlet_; ///< Outlet temperature sensor (T03)
    sensor::Sensor* t06_temperature_exhaust_;        ///< Exhaust temperature sensor (T06)

    // // Generic temperature sensors for identification
    // sensor::Sensor* temperature_sensor_1_; ///< Generic temperature sensor 1
    // sensor::Sensor* temperature_sensor_2_; ///< Generic temperature sensor 2
    // sensor::Sensor* temperature_sensor_3_; ///< Generic temperature sensor 3
    // sensor::Sensor* temperature_sensor_4_; ///< Generic temperature sensor 4

    bool is_heater_offline() {
        return (!this->hp_data_.last_heater_frame.has_value() ||
                (millis() - this->hp_data_.last_heater_frame.value()) > 30000);
    }
    /**
     * @brief Push a command to the heater.
     * @param temperature The target temperature.
     * @param mode The operating mode.
     */
    void push_command(float temperature, climate::ClimateMode mode);

    /**
     * @brief Process state packets received from the heater.
     */
    void process_state_packets();
    void set_actual_status(const std::string status, bool force = false);
    /**
     * @brief Serialize a command frame.
     * @param frameptr Pointer to the command frame.
     * @param temperature The target temperature.
     * @param mode The operating mode.
     * @return True if serialization is successful, false otherwise.
     */
    bool serialize_command_frame(BaseFrame* frameptr, float temperature, climate::ClimateMode mode);

    ESPPreferenceObject preferences_;
    struct PoolHeaterPreferences {
        bool dummy;
    };
    void setup() override;
    void dump_config() override;
    void restore_preferences_() {
        // PoolHeaterPreferences prefs;
        // if (preferences_.load(&prefs)) {
        //   // currentTemperatureSource
        //   if (prefs.currentTemperatureSourceIndex.has_value() &&
        //       temperature_source_select_->has_index(prefs.currentTemperatureSourceIndex.value())
        //       &&
        //       temperature_source_select_->at(prefs.currentTemperatureSourceIndex.value()).has_value())
        //       {
        //     current_temperature_source_ =
        //     temperature_source_select_->at(prefs.currentTemperatureSourceIndex.value()).value();
        //     temperature_source_select_->publish_state(current_temperature_source_);
        //     ESP_LOGCONFIG(TAG, "Preferences loaded.");
        //   } else {
        //     ESP_LOGCONFIG(TAG, "Preferences loaded, but unsuitable values.");
        //     current_temperature_source_ = TEMPERATURE_SOURCE_INTERNAL;
        //     temperature_source_select_->publish_state(TEMPERATURE_SOURCE_INTERNAL);
        //   }
        // } else {
        //   // TODO: Shouldn't need to define setting all these defaults twice
        //   ESP_LOGCONFIG(TAG, "Preferences not loaded.");
        //   current_temperature_source_ = TEMPERATURE_SOURCE_INTERNAL;
        //   temperature_source_select_->publish_state(TEMPERATURE_SOURCE_INTERNAL);
        // }
    }

    void save_preferences_() {
        // todo: implement saving preferences
        //  PoolHeaterPreferences prefs{};
        //  preferences_.save(&prefs);
    }

    template <typename T, typename U>
    void publish_sensor_value(const optional<U>& value, T* sensor) {
        if (sensor == nullptr || !this->update_active_ || !value.has_value()) return;
        this->call_publish_sensor_value(value.value(), sensor);
    }
    template <typename T>
    inline void publish_sensor_value(const optional<T>& value, select::Select* sensor) {
        if (sensor == nullptr || !this->update_active_ || !value.has_value()) return;
        this->call_publish_sensor_value(value.value().to_string(), sensor);
    }
    template <typename T, typename U> void publish_sensor_value(const U& value, T* sensor) {
        if (sensor == nullptr || !this->update_active_) return;
        this->call_publish_sensor_value(value, sensor);
    }

    template <typename T, typename U>
    inline void call_publish_sensor_value(const U& value, T* sensor) {
        sensor->publish_state(value);
    }
    template <typename T>
    inline void call_publish_sensor_value(const optional<T>& value, T& target) {
        target = value.value_or(target);
    }
};
} // namespace hwp
} // namespace esphome
