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

#include "HPBusDriver.h"
#include "HeaterStatus.h"
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
namespace hayward_pool_heater {

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
class PoolHeater : public climate::Climate, public Component {
  public:
    PoolHeater(InternalGPIOPin* gpio_pin, uint8_t max_buffer_count);

    void set_out_temperature_sensor(sensor::Sensor* sensor);
    void set_actual_status_sensor(text_sensor::TextSensor* sensor);
    void set_heater_status_code_sensor(text_sensor::TextSensor* sensor);
    void set_heater_status_description_sensor(text_sensor::TextSensor* sensor);
    void set_heater_status_solution_sensor(text_sensor::TextSensor* sensor);

    void publish_state();
    /**
     * @brief Main loop to process state packets.
     */
    void loop() override;

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

    /**
     * @brief Set the temperature out value.
     * @param temp The temperature out value.
     */
    void set_temperature_out(float temp);

    /**
     * @brief Set the temperature in value.
     * @param temp The temperature in value.
     */
    void set_temperature_in(float temp);

    /**
     * @brief Set the target temperature.
     * @param temp The target temperature.
     */
    void set_target_temperature(float temp);

    /**
     * @brief Set the operating mode.
     * @param mode The operating mode.
     */
    void set_mode(climate::ClimateMode mode);

    /**
     * @brief Set passive mode.
     * @param passive True to enable passive mode, false otherwise.
     */
    void set_passive_mode(bool passive);
    bool get_passive_mode();

  protected:
    HPBusDriver driver_; ///< The bus driver for communication.
    HeaterStatus heater_status_;
    float temperature_out_;
    std::string actual_status_;
    bool passive_mode_ = true;
    text_sensor::TextSensor* actual_status_sensor_{nullptr};
    sensor::Sensor* out_temperature_sensor_{nullptr};
    text_sensor::TextSensor* heater_status_code_sensor_{nullptr};
    text_sensor::TextSensor* heater_status_description_sensor_{nullptr};
    text_sensor::TextSensor* heater_status_solution_sensor_{nullptr};

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
    void set_actual_status(const std::string status);
    /**
     * @brief Serialize a command frame.
     * @param frameptr Pointer to the command frame.
     * @param temperature The target temperature.
     * @param mode The operating mode.
     * @return True if serialization is successful, false otherwise.
     */
    bool serialize_command_frame(
        CommandFrame* frameptr, float temperature, climate::ClimateMode mode);

    /**
     * @brief Check if the temperature is valid.
     * @param temp The temperature to check.
     * @return True if the temperature is between 15 and 33 degrees, false otherwise.
     */
    bool is_temperature_valid(float temp) { return (temp >= 15 && temp <= 33); }

    /**
     * @brief Parse the temperature from a byte.
     * @param tempByte The byte containing the temperature.
     * @return The parsed temperature.
     */
    float parse_temperature(uint8_t tempByte);

    /**
     * @brief Parse a heater packet.
     * @param frame The frame to parse.
     */
    void parse_heater_packet(const DecodingStateFrame& frame);
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
        // PoolHeaterPreferences prefs{};
        // preferences_.save(&prefs);
    }
};

} // namespace hayward_pool_heater
} // namespace esphome
