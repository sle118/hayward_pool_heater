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

#include "FrameConf1.h"
#include "CS.h"
#include "Schema.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/climate/climate_mode.h"
namespace esphome {
namespace hwp {
constexpr char TAG[] = "hwp";
CLASS_ID_DECLARATION(esphome::hwp::FrameConf1);
std::shared_ptr<BaseFrame> FrameConf1::create() {
    return std::make_shared<FrameConf1>(); //  Create a FrameConf1 if type matches
}
/**
 * @class FrameConf1
 * @brief A class to handle command frames in the Heat Pump Controller.
 *
 * This class extends the BaseFrame class to include additional functionality
 * specific to command frames.
 */

void FrameConf1::reset() {
    data_->mode.auto_mode = false;
    data_->mode.heat = false;
    data_->mode.power = false;
}

optional<std::shared_ptr<BaseFrame>> FrameConf1::control(const HWPCall& call) {

    // ESP_LOGD(TAG, "Base command : ");
    FrameConf1 command_frame(*this);
    auto has_value = this->data_.has_value();

    if (call.get_mode().has_value()) {
        call.hp_data.mode = *call.get_mode();
        ESP_LOGI(TAG, "FrameConf1 control: request for mode %s",
            LOG_STR_ARG(climate_mode_to_string(*call.hp_data.mode)));
        command_frame.set_mode(*call.hp_data.mode);
    }
    if(call.h02_mode_restrictions.has_value()) {
        ESP_LOGI(TAG, "FrameConf1 control: request for mode restrictions %s",
        call.h02_mode_restrictions.value().to_string().c_str());
        command_frame.data().mode.set_mode_restriction(call.h02_mode_restrictions.value());
    }
    if (call.get_target_temperature().has_value()) {
        ESP_LOGI(TAG, "FrameConf1 control: request for target temperature %.1f",
            call.get_target_temperature().value());
        if (!call.hp_data.is_temperature_valid(call.get_target_temperature().value())) {
            char error_msg[101] = {};
            sprintf(error_msg, "Invalid temperature %.1f. Must be between %.1fC and %.1fC.",
                call.get_target_temperature().value(), call.hp_data.get_min_target(),
                call.hp_data.get_max_target());
            ESP_LOGE(TAG, "Error setTemp:  %s", error_msg);
        } else {
            call.hp_data.target_temperature = *call.get_target_temperature();
        }

        switch (command_frame.get_active_mode()) {
        case STATE_COOLING_MODE:
            ESP_LOGD(TAG,
                "FrameConf1 control: request for cooling temperature %.1f, changing from "
                "%.1f",
                call.hp_data.target_temperature.value(),
                command_frame.data().r01_setpoint_cooling.decode());
            command_frame.set_target_cooling(*call.hp_data.target_temperature);
            break;
        case STATE_HEATING_MODE:
            command_frame.set_target_heating(*call.hp_data.target_temperature);
            ESP_LOGD(TAG,
                "FrameConf1 control: request for heating temperature %.1f, changing from "
                "%.1f",
                call.hp_data.target_temperature.value(),
                command_frame.data().r02_setpoint_heating.decode());
            break;
        case STATE_AUTO_MODE:
            ESP_LOGD(TAG,
                "FrameConf1 control: request for auto temperature %.1f, changing from %.1f",
                call.hp_data.target_temperature.value(),
                command_frame.data().r03_setpoint_auto.decode());
            command_frame.set_target_auto(*call.hp_data.target_temperature);
            break;
        case STATE_OFF: {
            ESP_LOGD(TAG, "FrameConf1 control: request for temperature %.1f while off",
                call.hp_data.target_temperature.value());
            auto restrictions = command_frame.data().mode.get_mode_restriction();
            if (restrictions == HeatPumpRestrict::Cooling) {
                ESP_LOGW(TAG, "Heater is off, restricted to cooling. Setting cooling target");
                command_frame.set_target_cooling(*call.hp_data.target_temperature);
            } else if (restrictions == HeatPumpRestrict::Heating) {
                ESP_LOGW(TAG, "Heater is off, restricted to heating. Setting heating target");
                command_frame.set_target_heating(*call.hp_data.target_temperature);
            } else {
                ESP_LOGW(TAG, "Heater is off, unknown mode for setpoint. Setting heating target");
                command_frame.set_target_heating(*call.hp_data.target_temperature);
            }
        } break;
        }
    }
    if(call.r04_return_diff_cooling.has_value()) {
        ESP_LOGD(TAG, "FrameConf1 control: request for return_diff_cooling %.1f",
            *call.r04_return_diff_cooling);
        command_frame.data().r04_return_diff_cooling = *call.r04_return_diff_cooling;
    }
    if (call.r05_shutdown_temp_diff_when_cooling.has_value()) {
        ESP_LOGD(TAG, "FrameConf1 control: request for shutdown_temp_diff_when_cooling %.1f",
            *call.r05_shutdown_temp_diff_when_cooling);
        command_frame.data().r05_shutdown_temp_diff_when_cooling =
            *call.r05_shutdown_temp_diff_when_cooling;
    }

    if (call.r06_return_diff_heating.has_value()) {
        ESP_LOGD(TAG, "FrameConf1 control: request for return_diff_heating %.1f",
            *call.r06_return_diff_heating);
        command_frame.data().r06_return_diff_heating = *call.r06_return_diff_heating;
    }

    if (call.r07_shutdown_diff_heating.has_value()) {
        ESP_LOGD(TAG, "FrameConf1 control: request for return_diff_cooling %.1f",
            *call.r07_shutdown_diff_heating);
        command_frame.data().r07_shutdown_diff_heating = *call.r07_shutdown_diff_heating;
    }

    if (!command_frame.is_changed() && has_value) {
        ESP_LOGD(TAG, "control: no changes to send for temperature control frame");
        return nullopt;
    }
    if (!has_value) {
        ESP_LOGW(TAG, "Cannot control yet. Waiting for first heater state packet");
        call.component.status_momentary_warning("Waiting for initial heater state", 5000);
        return nullopt;
    }

    command_frame.finalize();
    command_frame.print("TXQ", TAG, ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
    return optional<std::shared_ptr<FrameConf1>>{
        std::make_shared<FrameConf1>(command_frame)};
}
void FrameConf1::traits(climate::ClimateTraits& traits, heat_pump_data_t& hp_data) {
    const std::set<climate::ClimateMode> any_mode = {
        climate::CLIMATE_MODE_OFF,
        climate::CLIMATE_MODE_HEAT,
        climate::CLIMATE_MODE_COOL,
        climate::CLIMATE_MODE_AUTO,
    };
    if (this->data_.has_value()) {
        traits.add_supported_mode(climate::CLIMATE_MODE_OFF);
        auto mode_restrictions = this->data_->mode.get_mode_restriction();
        if (mode_restrictions == HeatPumpRestrict::Any) {
            traits.set_supported_modes(any_mode);
        } else if (mode_restrictions == HeatPumpRestrict::Heating) {
            traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
        } else if (mode_restrictions == HeatPumpRestrict::Cooling) {
            traits.add_supported_mode(climate::CLIMATE_MODE_COOL);
        } else {
            // ESP_LOGW(TAG_TEMP, "Restriction mode %s needs traits update",
            // temp_frame->get_mode_restriction_desc());
        }
    } else {
        traits.set_supported_modes(any_mode);
    }
}
bool FrameConf1::matches(BaseFrame& specialized, BaseFrame& base) {
    return base.packet.get_type() == FRAME_ID_CONF_1;
}
void FrameConf1::set_target_cooling(float temperature) {
    ESP_LOGD(TAG_TEMP,
        "Setting cooling target temperature to %.1fC With return differential temp %.0fC",
        temperature, data_->r04_return_diff_cooling.decode());
    data_->r01_setpoint_cooling.encode(temperature);
}
void FrameConf1::set_target_heating(float temperature) {
    ESP_LOGD(TAG_TEMP,
        "Setting heating target temperature to %.1fC. With return differential temp %.0fC",
        temperature, data_->r06_return_diff_heating.decode());
    data_->r02_setpoint_heating.encode(temperature);
}
void FrameConf1::set_target_auto(float temperature) {
    ESP_LOGD(TAG_TEMP, "Setting automatic target temperature to %.1fC.", temperature);
    data_->r03_setpoint_auto.encode(temperature);
}
climate::ClimateMode FrameConf1::get_climate_mode() const {
    switch (get_active_mode()) {
    case STATE_AUTO_MODE:
        ESP_LOGW(TAG_TEMP, "Need more analysis to figure out what the heat pump is doing");
        return climate::CLIMATE_MODE_AUTO;
        break;
    case STATE_COOLING_MODE:
        ESP_LOGW(TAG_TEMP, "Need more analysis to figure out what the heat pump is doing");
        return climate::CLIMATE_MODE_COOL;
        break;
    case STATE_HEATING_MODE:
        ESP_LOGW(TAG_TEMP, "Need more analysis to figure out what the heat pump is doing");
        return climate::CLIMATE_MODE_HEAT;
        break;
    default:
        return climate::CLIMATE_MODE_OFF;
        break;
    }
}
float FrameConf1::get_target_temperature() const {
    switch (this->get_active_mode()) {
    case STATE_AUTO_MODE:
        return data_->r03_setpoint_auto.decode();
        break;
    case STATE_COOLING_MODE:
        return data_->r01_setpoint_cooling.decode();
        break;
    case STATE_HEATING_MODE:
        return data_->r02_setpoint_heating.decode();
        break;
    default:
        return data_->r02_setpoint_heating.decode();
        break;
    }
}
active_modes_t FrameConf1::get_active_mode() const { return get_active_mode(data_->mode); }
active_modes_t FrameConf1::get_active_mode(const FrameConf1& frame) {
    return frame.get_active_mode();
}
bool FrameConf1::is_power_on() const { return is_power_on(data_->mode); }

bool FrameConf1::is_power_on(hp_mode_t mode) { return mode.power; }

active_modes_t FrameConf1::get_active_mode(hp_mode_t mode) {
    if (!mode.power) {
        return STATE_OFF;
    }
    if (mode.auto_mode) {
        return STATE_AUTO_MODE;
    }
    if (mode.heat) {
        return STATE_HEATING_MODE;
    }
    return STATE_COOLING_MODE;
}
const char* FrameConf1::get_active_mode_desc() const {
    return get_active_mode_desc(this->get_active_mode());
}
const char* FrameConf1::get_active_mode_desc(active_modes_t state) {
    switch (state) {
    case STATE_OFF:
        return "OFF    ";
    case STATE_HEATING_MODE:
        return "HEATING";
    case STATE_COOLING_MODE:
        return "COOLING";
    case STATE_AUTO_MODE:
        return "AUTO   ";
    }
    return "UNKN   ";
}

const char* FrameConf1::type_string() const { return "CONFIG_1  "; }

const char* FrameConf1::get_power_mode_desc(bool power) { return (power ? "ON " : "OFF"); }

std::string FrameConf1::format(bool no_diff) const {
    if (!this->data_.has_value()) return "N/A";
    return this->format(*data_, (no_diff ? *data_ : prev_data_.value_or(*data_)));
}

std::string FrameConf1::format_prev() const {
    if (!this->prev_data_.has_value()) return "N/A";
    return this->format(this->prev_data_.value(), this->prev_data_.value());
}

std::string FrameConf1::format(const conf_1_t& val, const conf_1_t& prev) const {
    CS oss;
    bool changed = (val != prev);
    oss.set_changed_base_color(changed);

    const char* restrictions = val.mode.log_format();
    const char* ref_restrictions = prev.mode.log_format();

    const char* modestr = get_active_mode_desc(get_active_mode(val.mode));
    const char* ref_modestr = get_active_mode_desc(get_active_mode(prev.mode));

    oss << "cool:" << val.r01_setpoint_cooling.diff(prev.r01_setpoint_cooling)
        << "heat:" << val.r02_setpoint_heating.diff(prev.r02_setpoint_heating)
        << "auto: " << val.r03_setpoint_auto.diff(prev.r03_setpoint_auto)
        << ", r04_cool_ret_dif:" << val.r04_return_diff_cooling.diff(prev.r04_return_diff_cooling)
        << ", r05_cool_shutdown_diff:"
        << val.r05_shutdown_temp_diff_when_cooling.diff(prev.r05_shutdown_temp_diff_when_cooling)
        << ", r06_heat_ret_dif:"
        << val.r06_return_diff_heating.diff(prev.r06_return_diff_heating, "") << ", Mode: ("
        << "[" << val.mode.raw.diff(prev.mode.raw) << "] "
        << format_diff(
               get_power_mode_desc(val.mode.power), get_power_mode_desc(prev.mode.power), "/")
        << std::setw(strlen(modestr)) << format_diff(modestr, ref_modestr) << "/"
        << std::setw(strlen(restrictions)) << format_diff(restrictions, ref_restrictions) << ", ["
        << val.mode.raw.diff(prev.mode.raw, 6, 2, "]") << ") r07_heat_shutdown_diff:"
        << val.r07_shutdown_diff_heating.diff(prev.r07_shutdown_diff_heating) << " ["
        << val.reserved_7.diff(prev.reserved_7) << "]";
    return oss.str();
}
void FrameConf1::set_mode(climate::ClimateMode mode) {
    data_->mode.power = false;
    data_->mode.heat = false;
    data_->mode.auto_mode = false;
    switch (mode) {
    case climate::CLIMATE_MODE_AUTO:
        data_->mode.auto_mode = true;
        data_->mode.power = true;
        break;
    case climate::CLIMATE_MODE_HEAT:
        data_->mode.heat = true;
        data_->mode.power = true;
        break;
    case climate::CLIMATE_MODE_COOL:
        data_->mode.power = true;
        break;
    default:
        data_->mode.power = false;
        break;
    }
}
/**
 * @brief Parses the frame data and places it into the canonical
 *        heat_pump_data_t structure.
 *
 * The heat_pump_data_t is a canonical representation between the heat pump bus
 * data and the esphome heat pump component.
 * 
 * @param hp_data The heat_pump_data_t structure to fill
 * @see heat_pump_data_t
 * @note current elements identified in this frame are climate mode, target temperature, restrictions
 * and setpoints
 */
void FrameConf1::parse(heat_pump_data_t& hp_data) {
    if (this->source_ != SOURCE_HEATER) {
        ESP_LOGD(TAG,"Ignoring temperature config data when not originating from the heat pump");
        return;
    };
    // only parse if the source is heater
    hp_data.mode = get_climate_mode();
    hp_data.target_temperature = this->get_target_temperature();
    hp_data.mode_restrictions = data_->mode.get_mode_restriction();
    hp_data.r01_setpoint_cooling = data_->r01_setpoint_cooling.decode();
    hp_data.r02_setpoint_heating = data_->r02_setpoint_heating.decode();
    hp_data.r03_setpoint_auto = data_->r03_setpoint_auto.decode();
    hp_data.r04_return_diff_cooling = data_->r04_return_diff_cooling.decode();
    hp_data.r05_shutdown_temp_diff_when_cooling =
        data_->r05_shutdown_temp_diff_when_cooling.decode();
    hp_data.r06_return_diff_heating = data_->r06_return_diff_heating.decode();
    hp_data.r07_shutdown_diff_heating = data_->r07_shutdown_diff_heating.decode();
}

} // namespace hwp
} // namespace esphome
REGISTER_FRAME_ID_DEFAULT(esphome::hwp::FrameConf1);
