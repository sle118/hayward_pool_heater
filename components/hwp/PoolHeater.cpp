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

#include "Schema.h"
#include "base_frame.h"

#include "esphome/components/climate/climate_mode.h"
#include "esphome/core/defines.h"
#include "hwp_call.h"
#ifdef USE_LOGGER
#include "esphome/components/logger/logger.h"
#endif
#include <cmath>
namespace esphome {
namespace hwp {

const char* POOL_HEATER_TAG = "hwp";
PoolHeater::PoolHeater(InternalGPIOPin* gpio_pin) { this->driver_.set_gpio_pin(gpio_pin); }

void PoolHeater::setup() {
    ESP_LOGI(POOL_HEATER_TAG, "Restoring state");
    restore_state_();
    ESP_LOGI(POOL_HEATER_TAG, "Setting up driver");
    this->driver_.setup();
    this->driver_.set_data_model(hp_data_);
    this->current_temperature = NAN;

    // Using App.get_compilation_time() means these will get reset each time the firmware is
    // updated, but this is an easy way to prevent wierd conflicts if e.g. select options change.
    preferences_ = global_preferences->make_preference<PoolHeaterPreferences>(
        get_object_id_hash() ^ fnv1_hash(App.get_compilation_time()));
    restore_preferences_();
    set_actual_status("Ready");
    this->status_set_warning("Waiting for heater state");
    ESP_LOGI(POOL_HEATER_TAG, "Setup complete");
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

void PoolHeater::update() {
    //////////////////////////////////////////////
    // Transfer data to climate                 //
    //////////////////////////////////////////////
    ESP_LOGD(POOL_HEATER_TAG, "Transferring data to climate component");
    if (this->driver_.get_bus_mode() == BUSMODE_ERROR) {
        this->status_momentary_error("Bus Error", 5000);
    }
    if (is_heater_offline()) {
        this->status_set_warning("Heater offline");
        set_actual_status("Waiting for heater");
    }
    set_actual_status("Connected to heater");
    ESP_LOGVV(POOL_HEATER_TAG, "Setting current temperature");
    this->current_temperature = this->hp_data_.t02_temperature_inlet.value_or(this->current_temperature);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting target temperature");
    this->target_temperature = this->hp_data_.target_temperature.value_or(this->target_temperature);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting action value");
    this->action = this->hp_data_.action.value_or(this->action);

    ESP_LOGVV(POOL_HEATER_TAG, "Setting climate mode");
    if (this->hp_data_.mode == climate::CLIMATE_MODE_OFF) {
        this->action = climate::CLIMATE_ACTION_OFF;
    }
    this->mode = this->hp_data_.mode.value_or(this->mode);
    if(this->hp_data_.fan_mode.has_value() ) {
        this->custom_fan_mode = this->hp_data_.fan_mode->to_custom_fan_mode();
        this->fan_mode = this->hp_data_.fan_mode->to_climate_fan_mode();
    }

    //////////////////////////////////////////////
    // Transfer data to float sensors           //
    //////////////////////////////////////////////
    // temperatures
    ESP_LOGVV(POOL_HEATER_TAG, "Setting suction temperature");
    publish_sensor_value(this->hp_data_.t01_temperature_suction, this->t01_temperature_suction_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting outlet temperature");
    publish_sensor_value(this->hp_data_.t03_temperature_outlet, this->t03_temperature_outlet_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting coil temperature");
    publish_sensor_value(this->hp_data_.t04_temperature_coil, this->t04_temperature_coil_);

    ESP_LOGVV(POOL_HEATER_TAG, "Setting ambient temperature");
    publish_sensor_value(this->hp_data_.t05_temperature_ambient, this->t05_temperature_ambient_);


    ESP_LOGVV(POOL_HEATER_TAG, "Setting exhaust temperature");
    publish_sensor_value(this->hp_data_.t06_temperature_exhaust, this->t06_temperature_exhaust_);
    // defrost config
    ESP_LOGVV(POOL_HEATER_TAG, "Setting defrost start");
    publish_sensor_value(this->hp_data_.d01_defrost_start, this->d01_defrost_start_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting defrost end");
    publish_sensor_value(this->hp_data_.d02_defrost_end, this->d02_defrost_end_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting defrost cycle time");
    publish_sensor_value(
        this->hp_data_.d03_defrosting_cycle_time_minutes, this->d03_defrosting_cycle_time_minutes_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting max defrost time");
    publish_sensor_value(
        this->hp_data_.d04_max_defrost_time_minutes, this->d04_max_defrost_time_minutes_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting min economy defrost time");
    publish_sensor_value(this->hp_data_.d05_min_economy_defrost_time_minutes,
        this->d05_min_economy_defrost_time_minutes_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting eco mode");
    publish_sensor_value(this->hp_data_.d06_defrost_eco_mode, this->d06_defrost_eco_mode_);

    // setpoints/temperatyre limits
    ESP_LOGVV(POOL_HEATER_TAG, "Setting cooling setpoint");
    publish_sensor_value(this->hp_data_.r01_setpoint_cooling, this->r01_setpoint_cooling_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting heating setpoint");
    publish_sensor_value(this->hp_data_.r02_setpoint_heating, this->r02_setpoint_heating_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting auto setpoint");
    publish_sensor_value(this->hp_data_.r03_setpoint_auto, this->r03_setpoint_auto_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting return diff cooling");
    publish_sensor_value(this->hp_data_.r04_return_diff_cooling, this->r04_return_diff_cooling_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting shutdown temp diff cooling");
    publish_sensor_value(this->hp_data_.r05_shutdown_temp_diff_when_cooling,
        this->r05_shutdown_temp_diff_when_cooling_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting shutdown diff cooling");
    publish_sensor_value(this->hp_data_.r06_return_diff_heating, this->r06_return_diff_heating_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting shutdown temp diff heating");
    publish_sensor_value(
        this->hp_data_.r07_shutdown_diff_heating, this->r07_shutdown_diff_heating_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting min/max setpoints");
    publish_sensor_value(this->hp_data_.r08_min_cool_setpoint, this->r08_min_cool_setpoint_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting max cooling setpoint");
    publish_sensor_value(this->hp_data_.r09_max_cooling_setpoint, this->r09_max_cooling_setpoint_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting min heating setpoint");
    publish_sensor_value(this->hp_data_.r10_min_heating_setpoint, this->r10_min_heating_setpoint_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting max heating setpoint");
    publish_sensor_value(this->hp_data_.r11_max_heating_setpoint, this->r11_max_heating_setpoint_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting pulses per liter");
    publish_sensor_value(this->hp_data_.U02_pulses_per_liter, this->u02_pulses_per_liter_);

    //////////////////////////////////////////////
    // Transfer data to text sensors            //
    //////////////////////////////////////////////
    ESP_LOGVV(POOL_HEATER_TAG, "Setting actual status");
    publish_sensor_value(this->actual_status_, this->actual_status_sensor_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting heater status code");
    publish_sensor_value(this->heater_status_.get_code(), this->heater_status_code_sensor_);
    ESP_LOGVV(POOL_HEATER_TAG, "Setting heater status description");
    publish_sensor_value(
        this->heater_status_.get_description(), this->heater_status_description_sensor_);
    publish_sensor_value(this->heater_status_.get_solution(), this->heater_status_solution_sensor_);
    

    //////////////////////////////////////////////    
    // Transfer data to binary sensors          //
    //////////////////////////////////////////////
    ESP_LOGVV(POOL_HEATER_TAG, "Setting water flow");
    publish_sensor_value(this->hp_data_.S02_water_flow, this->s02_water_flow_);

    
    //////////////////////////////////////////////
    // Transfer data to select sensors          //
    //////////////////////////////////////////////
    ESP_LOGVV(POOL_HEATER_TAG, "Setting mode restrictions");
    publish_sensor_value(this->hp_data_.mode_restrictions, this->h02_mode_restrictions_);    
     ESP_LOGVV(POOL_HEATER_TAG, "Setting flow meter");
    publish_sensor_value(this->hp_data_.U01_flow_meter, this->u01_flow_meter_);
    


    //////////////////////////////////////////////
    // Publish the climate state if needed      //
    //////////////////////////////////////////////
    if (this->update_active_) {
        ESP_LOGD(POOL_HEATER_TAG, "Publishing climate state");
        save_preferences_();     
        climate::Climate::publish_state();
    }
}


void PoolHeater::dump_config() {
    ESP_LOGCONFIG(POOL_HEATER_TAG, "hwp:");
    if (this->driver_.get_gpio_pin()) {
        ESP_LOGCONFIG(
            POOL_HEATER_TAG, "      - txrx_pin: %d", this->driver_.get_gpio_pin()->get_pin());
    } else {
        ESP_LOGCONFIG(POOL_HEATER_TAG, "      - txrx_pin: NULLPTR");
    }
    ESP_LOGCONFIG(POOL_HEATER_TAG, "      - passive_mode: %s", ONOFF(this->passive_mode_));
    ESP_LOGCONFIG(POOL_HEATER_TAG, "      - update_active: %s", ONOFF(this->update_active_));
    dump_traits_(POOL_HEATER_TAG);
    this->driver_.dump_known_packets(POOL_HEATER_TAG);
}
void PoolHeater::control(const HWPCall& hwpcall) {
    auto ctrl_frames = this->driver_.control(hwpcall);
    if (this->passive_mode_) {
        ESP_LOGW(POOL_HEATER_TAG, "Passive mode. Ignoring inbound changes");
        this->status_momentary_warning("Passive mode. Ignoring changes", 5000);
        this->publish_state();
        return;
    }

    bool success = true;
    for (size_t i = 0; i < ctrl_frames.size(); i++) {
        ctrl_frames[i]->print("QUEUE", POOL_HEATER_TAG, ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
        if (!this->driver_.queue_frame_data(ctrl_frames[i])) {
            success = false;
            ESP_LOGE(POOL_HEATER_TAG, "Control queuing error for control frame %d", i);
        }
    }
    if (!success) {
        this->status_momentary_error("Control queuing error");
        this->publish_state();
    }
}
void PoolHeater::control(const climate::ClimateCall& call) {
    HWPCall hwpcall(call, *this, this->hp_data_, *this->actual_status_sensor_);
    this->control(hwpcall);
}

void PoolHeater::set_actual_status(const std::string status, bool force) {
    if (this->actual_status_ != status || force) {
        ESP_LOGD(POOL_HEATER_TAG, "%s", status.c_str());
        this->actual_status_ = status;
        this->actual_status_sensor_->publish_state(this->actual_status_);
    }
}

void PoolHeater::set_passive_mode(bool passive) {
    if (this->passive_mode_ == passive) return;
    this->passive_mode_ = passive;
    this->publish_state();
    ESP_LOGD(POOL_HEATER_TAG, "Setting passive mode: %s", ONOFF(passive));
}
void PoolHeater::set_update_active(bool active) {
    this->update_active_ = active;
    ESP_LOGD(POOL_HEATER_TAG, "Setting update sensors: %s", ONOFF(active));
}
void PoolHeater::generate_code() { BaseFrame::dump_c_code(POOL_HEATER_TAG); }
bool PoolHeater::get_passive_mode() { return this->passive_mode_; }
bool PoolHeater::is_update_active() { return this->update_active_; }

/**
 * @brief Get the climate traits.
 * @return The climate traits.
 */
climate::ClimateTraits PoolHeater::traits() {
    auto traits = climate::ClimateTraits();

    traits.set_supports_current_temperature(true);
    traits.set_supports_action(true);
    this->driver_.traits(traits, this->hp_data_);
    traits.set_supports_two_point_target_temperature(false);
    traits.set_supports_current_humidity(false);

    return traits;
}


} // namespace hwp
} // namespace esphome
