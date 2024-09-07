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
#include "PoolHeater.h"

#include "esphome/core/defines.h"
#ifdef USE_LOGGER
#include "esphome/components/logger/logger.h"
#endif
#include <cmath>

#include "HPUtils.h"

namespace esphome {

namespace hayward_pool_heater {

const char *POOL_HEATER_TAG = "hayward_pool_heater";
PoolHeater::PoolHeater(InternalGPIOPin *gpio_pin, uint8_t max_buffer_count) {
  this->driver_.set_gpio_pin(gpio_pin);
  this->driver_.set_max_buffer_count(max_buffer_count);
}

void PoolHeater::setup() {
  ESP_LOGI(POOL_HEATER_TAG, "Restoring state");
  restore_state_();
  ESP_LOGI(POOL_HEATER_TAG, "Setting up driver");
  this->driver_.setup();

  this->current_temperature = NAN;

  // Using App.get_compilation_time() means these will get reset each time the firmware is
  // updated, but this is an easy way to prevent wierd conflicts if e.g. select options change.
  preferences_ = global_preferences->make_preference<PoolHeaterPreferences>(get_object_id_hash() ^
                                                                            fnv1_hash(App.get_compilation_time()));
  restore_preferences_();
  set_actual_status("Ready");

  this->status_set_warning("Waiting for heater state");
  this->publish_state();
  ESP_LOGI(POOL_HEATER_TAG, "Setup complete");
}

void PoolHeater::set_out_temperature_sensor(sensor::Sensor *sensor) { this->out_temperature_sensor_ = sensor; }
void PoolHeater::set_actual_status_sensor(text_sensor::TextSensor *sensor) { this->actual_status_sensor_ = sensor; }

void PoolHeater::set_heater_status_code_sensor(text_sensor::TextSensor *sensor) {
  this->heater_status_code_sensor_ = sensor;
}
void PoolHeater::set_heater_status_description_sensor(text_sensor::TextSensor *sensor) {
  this->heater_status_description_sensor_ = sensor;
}
void PoolHeater::set_heater_status_solution_sensor(text_sensor::TextSensor *sensor) {
  this->heater_status_solution_sensor_ = sensor;
}

void PoolHeater::loop() {
  if (this->driver_.get_bus_mode() == BUSMODE_ERROR) {
    this->status_momentary_error("Bus Error", 5000);
  }
  process_state_packets();
  if (is_heater_offline()) {
    this->status_set_warning("Heater offline");
  }
}
void PoolHeater::dump_config() {
  ESP_LOGCONFIG(POOL_HEATER_TAG, "hayward_pool_heater:");
  if (this->driver_.get_gpio_pin()) {
    ESP_LOGCONFIG(POOL_HEATER_TAG, "      - txrx_pin: %d", this->driver_.get_gpio_pin()->get_pin());
  } else {
    ESP_LOGCONFIG(POOL_HEATER_TAG, "      - txrx_pin: NULLPTR");
  }
  dump_traits_(POOL_HEATER_TAG);
}

void PoolHeater::control(const climate::ClimateCall &call) {
  if (!this->command_frame_.has_previous()) {
    this->Component::status_momentary_warning("Waiting for initial heater state", 5000);
    return;
  }
  ESP_LOGD(POOL_HEATER_TAG, "Base command : ");
  CommandFrame command_frame = this->command_frame_.get_base_command();
  command_frame.debug_print_hex();

  if (call.get_mode().has_value()) {
    this->mode = *call.get_mode();
    command_frame.set_mode(this->mode);
  }

  if (call.get_target_temperature().has_value()) {
    if (!is_temperature_valid(*call.get_target_temperature())) {
      char error_msg[101] = {};
      sprintf(error_msg, "Invalid temperature %.1d. Must be between 15C and 33C.", *call.get_target_temperature());
      ESP_LOGE(POOL_HEATER_TAG, "Error setTemp:  %s", error_msg);
      set_actual_status(error_msg);
      return;
    }
    this->target_temperature = *call.get_target_temperature();
    if (command_frame.packet.tempprog.mode.auto_mode) {
      command_frame.set_target_temperature_1(this->target_temperature);
    } else {
      command_frame.set_target_temperature_2(this->target_temperature);
    }
  }

  if (this->passive_mode_) {
    ESP_LOGW(POOL_HEATER_TAG, "Passive mode. Ignoring inbound changes");
    this->status_momentary_warning("Passive mode. Ignoring changes", 5000);
    this->publish_state();
    return;
  }
  command_frame.set_checksum();
  ESP_LOGD(POOL_HEATER_TAG, "Command frame before queuing");

  if (!this->driver_.queue_frame_data(command_frame)) {
    ESP_LOGE(POOL_HEATER_TAG, "Error queuing data frame");
    this->status_momentary_error("Command queuing error");
    this->publish_state();
  }
}
void PoolHeater::publish_state() {
  save_preferences_();
  climate::Climate::publish_state();
  if (this->out_temperature_sensor_) {
    this->out_temperature_sensor_->publish_state(this->temperature_out_);
  }
  if (this->actual_status_sensor_) {
    this->actual_status_sensor_->publish_state(this->actual_status_);
  }

  if (this->heater_status_code_sensor_) {
    this->heater_status_code_sensor_->publish_state(this->heater_status_.get_code());
  }
  if (this->heater_status_description_sensor_) {
    this->heater_status_description_sensor_->publish_state(this->heater_status_.get_description());
  }
  if (this->heater_status_solution_sensor_) {
    this->heater_status_solution_sensor_->publish_state(this->heater_status_.get_solution());
  }
}

void PoolHeater::parse_heater_packet(const DecodingStateFrame &frame) {
  float temperature = 0.0f;
#ifdef USE_LOGGER
  auto *log = logger::global_logger;
  bool debug_status = (log != nullptr && log->level_for(POOL_HEATER_TAG) >= ESPHOME_LOG_LEVEL_DEBUG);
#else
  bool debug_status = false;
#endif
  frame_type_t result = frame.get_frame_type();

  switch (result) {
    case FRAME_TEMP_OUT:
      temperature = BaseFrame::parse_temperature(frame.packet.tempout.temperature);
      ESP_LOGV(POOL_HEATER_TAG, "Temperature OUT: %2.1fºC", temperature);
      if (frame.get_source() == SOURCE_HEATER) {
        set_temperature_out(temperature);
        this->last_heater_frame_ = millis();
      }

      //  [[12] D2,B1,11,66,75,52,5F,00,64,00,00,84] TEMP_OUT  (HEATER    ): T: 28.5ºC (0x75), Fan out? T: 11.0ºC
      //  (0x52), 3?? T: 17.5ºC (0x5f), 4?? T: 20.0ºC (0x64), Res 1-2: [00010001, 01100110], Res 5,7,8:
      //  [00000000,00000000,00000000]

      this->set_coil_temperature(BaseFrame::parse_temperature(frame.packet.tempout.temperature_coil));
      this->set_exhaust_temperature(BaseFrame::parse_temperature(frame.packet.tempout.temperature_exhaust));
      // this->set_temperature_sensor_4(BaseFrame::parse_temperature(frame.packet.tempout.temperature_4));

      break;
    case FRAME_TEMP_IN:
    case FRAME_TEMP_IN_B:
      temperature = BaseFrame::parse_temperature(frame.packet.tempin.temperature);
      ESP_LOGV(POOL_HEATER_TAG, "Temperature IN: %2.1fºC", temperature);
      if (frame.get_source() == SOURCE_HEATER) {
        set_temperature_in(temperature);
        this->last_heater_frame_ = millis();
      }

      break;
    case FRAME_TEMP_PROG:
      this->command_frame_.save_previous(frame);
      // Handle the packet based on the source
      if (frame.get_source() == SOURCE_HEATER) {
        // this->set_temperature_sensor_1(BaseFrame::parse_temperature(frame.packet.tempprog.set_point_2));
        // Source is the heater, this is a state report
        this->last_heater_frame_ = millis();

        // Determine the current action based on the heater state
        if (!frame.packet.tempprog.mode.power) {
          ESP_LOGV(POOL_HEATER_TAG, "Power: OFF");
          action = climate::CLIMATE_ACTION_IDLE;
        } else {
          // The heater is on, determine if it's heating or cooling
          action = frame.packet.tempprog.mode.heat ? climate::CLIMATE_ACTION_HEATING : climate::CLIMATE_ACTION_COOLING;
        }

        // Set the mode based on the heater's reported mode
        if (frame.packet.tempprog.mode.auto_mode) {
          ESP_LOGV(POOL_HEATER_TAG, "Mode: AUTO");
          set_mode(climate::CLIMATE_MODE_AUTO);
          ESP_LOGW(POOL_HEATER_TAG, "AUTO mode needs to be investigated to handle climate action reporting");
        } else {
          ESP_LOGV(POOL_HEATER_TAG, "Mode: %s", frame.packet.tempprog.mode.heat ? "HEAT" : "COOL");
          set_mode(frame.packet.tempprog.mode.heat ? climate::CLIMATE_MODE_HEAT : climate::CLIMATE_MODE_COOL);
        }
        if (frame.packet.tempprog.mode.auto_mode) {
          temperature = BaseFrame::parse_temperature(frame.packet.tempprog.set_point_1);
        } else {
          temperature = BaseFrame::parse_temperature(frame.packet.tempprog.set_point_2);
        }

        ESP_LOGV(POOL_HEATER_TAG, "Temperature PROG : %2.1fºC", temperature);
        this->set_target_temperature(temperature);
      } else if (frame.get_source() == SOURCE_CONTROLLER) {
        // Source is the controller (keypad), this is a user request

        // Update the mode based on the user's request
        if (!frame.packet.tempprog.mode.power) {
          ESP_LOGW(POOL_HEATER_TAG, "User requested Power: OFF");
          set_mode(climate::CLIMATE_MODE_OFF);
        } else {
          if (frame.packet.tempprog.mode.auto_mode) {
            ESP_LOGW(POOL_HEATER_TAG, "User requested Mode: AUTO");
            set_mode(climate::CLIMATE_MODE_AUTO);
          } else {
            ESP_LOGW(POOL_HEATER_TAG, "User requested Mode: %s", frame.packet.tempprog.mode.heat ? "HEAT" : "COOL");
            set_mode(frame.packet.tempprog.mode.heat ? climate::CLIMATE_MODE_HEAT : climate::CLIMATE_MODE_COOL);
          }
        }
      }

      break;

      // case FRAME_STATUS:
      //  todo: find the heater status from the packets and call
      //  this->heater_status_.update(status_value);
      // break;

    default:
      char frametype_msg[61] = {};
      sprintf(frametype_msg, "Unsupported frame type 0x%02X", frame.packet.frame_type);
      ESP_LOGV(POOL_HEATER_TAG, "%s", frametype_msg);
      // set_actual_status(frametype_msg);
      break;
  }
  this->publish_state();
}

void PoolHeater::set_temperature_in(float temp) {
  this->current_temperature = temp;
  this->publish_state();
}
void PoolHeater::set_target_temperature(float temp) {
  this->target_temperature = temp;
  this->publish_state();
}

void PoolHeater::set_actual_status(const std::string status, bool force) {
  if (this->actual_status_ != status || force) {
    ESP_LOGD(POOL_HEATER_TAG, "%s", status.c_str());
    this->actual_status_ = status;
    this->actual_status_sensor_->publish_state(this->actual_status_);
  }
}

void PoolHeater::process_state_packets() {
  DecodingStateFrame frame;

  if (this->driver_.get_next_frame(&frame)) {
    if (!frame.is_valid()) {
      ESP_LOGW(POOL_HEATER_TAG, "Invalid frame");
    }
    parse_heater_packet(frame);
  }
}

void PoolHeater::set_mode(climate::ClimateMode mode) {
  this->mode = mode;
  this->publish_state();
}
void PoolHeater::set_temperature_out(float temp) {
  this->temperature_out_ = temp;
  this->publish_state();
}

void PoolHeater::set_passive_mode(bool passive) {
  this->passive_mode_ = passive;
  ESP_LOGD(POOL_HEATER_TAG, "Setting passive mode: %s", ONOFF(passive));
}
bool PoolHeater::get_passive_mode() { return this->passive_mode_; }
/**
 * @brief Get the climate traits.
 * @return The climate traits.
 */
climate::ClimateTraits PoolHeater::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_supports_action(true);
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_COOL,
      climate::CLIMATE_MODE_AUTO,
  });
  traits.set_supports_two_point_target_temperature(false);
  traits.set_visual_min_temperature(POOLHEATER_TEMP_MIN);
  traits.set_visual_max_temperature(POOLHEATER_TEMP_MAX);
  traits.set_visual_temperature_step(1);
  traits.set_supports_current_humidity(false);
  return traits;
}
}  // namespace hayward_pool_heater
}  // namespace esphome
