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
#include "HPUtils.h"
namespace esphome {

namespace hayward_pool_heater {

const char* POOL_HEATER_TAG = "hayward_pool_heater";
const unsigned char cmdTrame[12] = {
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
PoolHeater::PoolHeater(InternalGPIOPin* gpio_pin, uint8_t max_buffer_count) {
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
  preferences_ = global_preferences->make_preference<PoolHeaterPreferences>(
      get_object_id_hash() ^ fnv1_hash(App.get_compilation_time()));
  restore_preferences_();
  set_actual_status("Ready");
  ESP_LOGI(POOL_HEATER_TAG, "Setup complete");
}

void PoolHeater::set_out_temperature_sensor(sensor::Sensor* sensor) {
  this->out_temperature_sensor_ = sensor;
}
void PoolHeater::set_actual_status_sensor(text_sensor::TextSensor* sensor) {
  this->actual_status_sensor_ = sensor;
}

void PoolHeater::set_heater_status_code_sensor(text_sensor::TextSensor* sensor) {
  this->heater_status_code_sensor_ = sensor;
}
void PoolHeater::set_heater_status_description_sensor(text_sensor::TextSensor* sensor) {
  this->heater_status_description_sensor_ = sensor;
}
void PoolHeater::set_heater_status_solution_sensor(text_sensor::TextSensor* sensor) {
  this->heater_status_solution_sensor_ = sensor;
}

void PoolHeater::loop() { process_state_packets(); }
void PoolHeater::dump_config() {
  ESP_LOGCONFIG(POOL_HEATER_TAG, "hayward_pool_heater:");
  if (this->driver_.get_gpio_pin()) {
    ESP_LOGCONFIG(POOL_HEATER_TAG, "      - txrx_pin: %d", this->driver_.get_gpio_pin()->get_pin());
  } else {
    ESP_LOGCONFIG(POOL_HEATER_TAG, "      - txrx_pin: NULLPTR");
  }
  dump_traits_(POOL_HEATER_TAG);
}

void PoolHeater::control(const climate::ClimateCall& call) {
  if (call.get_mode().has_value()) {
    this->mode = *call.get_mode();
  }

  if (call.get_target_temperature().has_value()) {
    this->target_temperature = *call.get_target_temperature();
  }

  push_command(this->target_temperature, this->mode);
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

  if(this->heater_status_code_sensor_){
    this->heater_status_code_sensor_->publish_state(this->heater_status_.get_code());
  }
  if(this->heater_status_description_sensor_){
    this->heater_status_description_sensor_->publish_state(this->heater_status_.get_description());
  }
  if(this->heater_status_solution_sensor_){
    this->heater_status_solution_sensor_->publish_state(this->heater_status_.get_solution());
  }
}

void PoolHeater::parse_heater_packet(const DecodingStateFrame& frame) {
#ifdef USE_LOGGER
  auto* log = logger::global_logger;
  bool debug_status =
      (log != nullptr && log->level_for(POOL_HEATER_TAG) >= ESPHOME_LOG_LEVEL_DEBUG);
#else
  bool debug_status = false;
#endif
  frame_type_t result = frame.get_frame_type();
  switch (result) {
    case FRAME_TYPE_TEMP_OUT:
      if (debug_status) set_actual_status("Parsing TEMP_OUT frame");
      set_temperature_out(parse_temperature(frame[FRAME_POS_TEMPOUT]));
      break;
    case FRAME_TYPE_TEMP_IN:
      if (debug_status) set_actual_status("Parsing TEMP_IN frame");
      set_temperature_in(parse_temperature(frame[FRAME_POS_TEMPIN]));
      break;
    case FRAME_TEMP_PROG:
      if (debug_status) set_actual_status("Parsing TEMP_PROG frame");
      this->set_target_temperature(parse_temperature(frame[FRAME_POS_target_temperature]));
      if (!get_bit(frame[FRAME_POS_MODE], FRAME_STATE_POWER_BIT_BE)) {
        set_mode(climate::CLIMATE_MODE_OFF);
      }
      if (get_bit(frame[FRAME_POS_MODE], FRAME_MODE_AUTO_BIT_BE)) {
        set_mode(climate::CLIMATE_MODE_AUTO);
      } else {
        set_mode(get_bit(frame[FRAME_POS_MODE], FRAME_MODE_HEAT_BIT_BE)
                     ? climate::CLIMATE_MODE_HEAT
                     : climate::CLIMATE_MODE_COOL);
      }
      break;
      //case FRAME_STATUS:
      // todo: find the heater status from the packets and call
      // this->heater_status_.update(status_value);
      //break;
      
      
    default:
      char frametype_msg[61] = {};
      sprintf(frametype_msg, "Unsupported frame type %0x%02X", frame[FRAME_POS_TYPE]);
      ESP_LOGW(POOL_HEATER_TAG, "%s", frametype_msg);
      set_actual_status(frametype_msg);
      break;
  }
}

float PoolHeater::parse_temperature(uint8_t tempByte) {
  float halfDeg = get_bit(tempByte, FRAME_TEMP_HALF_DEG_BIT_BE) ? 0.5f : 0.0f;
  return ((tempByte & FRAME_TEMP_MASK_BE) >> 1) + 2 + halfDeg;
}

bool PoolHeater::serialize_command_frame(CommandFrame* frameptr, float temperature,
                                         climate::ClimateMode mode) {
  CommandFrame& frame = *frameptr;
  frame = cmdTrame;
  frame[FRAME_POS_TYPE] = FRAME_TYPE_PROG_TEMP;
  frame[1] = 0xB1;
  frame[FRAME_POS_target_temperature] = DEFAULT_TEMP_MASK_BE;
  frame[FRAME_POS_MODE] = DEFAULT_POWERMODE_MASK_BE;

  if (!is_temperature_valid(temperature)) {
    char error_msg[101] = {};
    sprintf(error_msg, "Invalid temperature %.1d. Must be between 15C and 33C.", temperature);
    ESP_LOGE(POOL_HEATER_TAG, "Error setTemp:  %s", error_msg);
    set_actual_status(error_msg);
    return false;
  }
  clear_bit(&frame[FRAME_POS_MODE], FRAME_MODE_AUTO_BIT_BE);
  clear_bit(&frame[FRAME_POS_MODE], FRAME_MODE_HEAT_BIT_BE);
  clear_bit(&frame[FRAME_POS_MODE], FRAME_STATE_POWER_BIT_BE);

  if (mode != climate::CLIMATE_MODE_OFF) {
    set_bit(&frame[FRAME_POS_MODE], FRAME_STATE_POWER_BIT_BE);

    switch (mode) {
      case climate::CLIMATE_MODE_AUTO:
        set_bit(&frame[FRAME_POS_MODE], FRAME_MODE_AUTO_BIT_BE);
        break;
      case climate::CLIMATE_MODE_HEAT:
        set_bit(&frame[FRAME_POS_MODE], FRAME_MODE_HEAT_BIT_BE);
        break;
      case climate::CLIMATE_MODE_COOL:
      default:
        break;
    }

    float target_temperature = temperature;
    bool halfDegree = ((target_temperature * 10) - (target_temperature * 10)) > 0;
    uint8_t value = target_temperature - 2;
    frame[FRAME_POS_target_temperature] = frame[FRAME_POS_target_temperature] | (value << 1);
    if (halfDegree) {
      set_bit(&frame[FRAME_POS_target_temperature], FRAME_TEMP_HALF_DEG_BIT_BE);
    }
    if (frame.size() < FRAME_SIZE) {
      frame.append_checksum();
    } else {
      frame.set_checksum();
    }
    return true;
  }

  return false;
}

void PoolHeater::set_temperature_in(float temp) {
  this->current_temperature = temp;
  this->publish_state();
}
void PoolHeater::set_target_temperature(float temp) {
  this->target_temperature = temp;
  this->publish_state();
}


void PoolHeater::set_actual_status(const std::string status) {
  this->actual_status_ = status;
  ESP_LOGD(POOL_HEATER_TAG, "%s", status.c_str());
  this->actual_status_sensor_->publish_state(this->actual_status_);
}
void PoolHeater::process_state_packets() {
  DecodingStateFrame frame;
  if (this->driver_.get_next_frame(&frame) && frame.is_valid()) {
    if (frame.get_source() == SOURCE_HEATER) {
      parse_heater_packet(frame);
    }
    frame.debug_print_hex();
  }
}
void PoolHeater::push_command(float temperature, climate::ClimateMode mode) {
  CommandFrame frame;

  if (!this->passive_mode_) {
    if (!serialize_command_frame(&frame, temperature, mode)) {
      ESP_LOGE(POOL_HEATER_TAG, "Error serializing request");
      set_actual_status("Serialize error");
      return;
    }
    if (!this->driver_.queue_frame_data(frame)) {
      ESP_LOGE(POOL_HEATER_TAG, "Error queuing data frame");
      set_actual_status("Send Queuing error");
    }
  } else {
    ESP_LOGW(POOL_HEATER_TAG, "Passive mode. Ignoring inbound changes");
    set_actual_status("Ignored passive mode command");
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
climate::ClimateTraits traits()  {
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
