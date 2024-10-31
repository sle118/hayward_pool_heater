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

#include "FrameConf2.h"
#include "CS.h"
#include "HPUtils.h"
#include "Schema.h"
namespace esphome {
namespace hwp {
static constexpr char TAG[] = "hwp";
CLASS_ID_DECLARATION(esphome::hwp::FrameConf2);
std::shared_ptr<BaseFrame> FrameConf2::create() {
    return std::make_shared<FrameConf2>(); // Create a FrameTemperature if type matches
}
bool FrameConf2::matches(BaseFrame& secialized, BaseFrame& base) {
    return base.packet.get_type() == FRAME_ID_CONF_2;
}
const char* FrameConf2::type_string() const { return "CONFIG_2  "; }
std::string FrameConf2::format(bool no_diff) const {
    if (!this->data_.has_value()) return "N/A";
    return this->format(*data_, (no_diff ? *data_ : prev_data_.value_or(*data_)));
}

std::string FrameConf2::format_prev() const {
    if (!this->prev_data_.has_value()) return "N/A";
    return this->format(this->prev_data_.value(), this->prev_data_.value());
}


std::string FrameConf2::format(const conf_2_t& val, const conf_2_t& ref) const {
    CS oss;
    bool changed = (val != ref);
    oss.set_changed_base_color(changed);

    oss << "F01 Fan mode "
        << format_diff(val.fan_mode.get_fan_mode().log_string(), ref.fan_mode.get_fan_mode().log_string())
        << ", Defrost: d01-start " << val.d01_defrost_start.diff(ref.d01_defrost_start)
        << " d03-time "
        << val.d03_defrosting_cycle_time_minutes.diff(ref.d03_defrosting_cycle_time_minutes)
        << " d04-max time"
        << val.d04_max_defrost_time_minutes.diff(ref.d04_max_defrost_time_minutes) << " d02-end "
        << val.d02_defrost_end.diff(ref.d02_defrost_end) << " [ "
        << val.fan_mode.raw.diff(ref.fan_mode.raw, ", ") << val.unknown_5.diff(ref.unknown_5, ", ")
        << val.unknown_6.diff(ref.unknown_6, ", ") << val.unknown_7.diff(ref.unknown_7, ", ")
        << val.unknown_8.diff(ref.unknown_8, "] ");

    return oss.str();
}
void FrameConf2::set_fan_mode(FanMode mode) { data_->fan_mode.mode = mode.to_raw(); }
optional<std::shared_ptr<BaseFrame>> FrameConf2::control(const HWPCall& call) {
    FrameConf2 fan_mode_frame(*this);
    bool has_data = this->data_.has_value();
    if (call.d01_defrost_start.has_value()) {
        ESP_LOGD(TAG, "control: setting d01-defrost-start to %.1f", call.d01_defrost_start.value());
        fan_mode_frame.data().d01_defrost_start = call.d01_defrost_start.value();
    }

    if (call.d02_defrost_end.has_value()) {
        ESP_LOGD(TAG, "control: setting d02-defrost-end to %.1f", call.d02_defrost_end.value());
        fan_mode_frame.data().d02_defrost_end = call.d02_defrost_end.value();
    }

    if (call.d03_defrosting_cycle_time_minutes.has_value()) {
        ESP_LOGD(TAG, "control: setting d03-defrosting-cycle-time to %.1f",
            call.d03_defrosting_cycle_time_minutes.value());
        fan_mode_frame.data().d03_defrosting_cycle_time_minutes =
            call.d03_defrosting_cycle_time_minutes.value();
    }

    if (call.d04_max_defrost_time_minutes.has_value()) {
        ESP_LOGD(TAG, "control: setting d04-max-defrost-time to %.1f",
            call.d04_max_defrost_time_minutes.value());
        fan_mode_frame.data().d04_max_defrost_time_minutes =
            call.d04_max_defrost_time_minutes.value();
    }
    optional<FanMode> fan_mode = FanMode::from_call(call);
    if (fan_mode.has_value()) {
        ESP_LOGD(TAG, "control: setting fan mode to %s", fan_mode->to_string());
        fan_mode_frame.set_fan_mode(fan_mode.value());
    }

    if (!fan_mode_frame.is_changed() && has_data) {
        ESP_LOGD(TAG, "control: no changes to fan mode");
        return nullopt;
    }
    if (!has_data) {
        ESP_LOGW(TAG, "Cannot control yet. Waiting for first heater fan mode packet");
        call.component.status_momentary_warning("Waiting for initial heater fan mode packet", 5000);
        return nullopt;
    }
    fan_mode_frame.finalize();
    fan_mode_frame.print("TXQ", TAG, ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
    return optional<std::shared_ptr<FrameConf2>>{std::make_shared<FrameConf2>(fan_mode_frame)};
}
void FrameConf2::traits(climate::ClimateTraits& traits, heat_pump_data_t& hp_data) {
    hp_data.fan_mode->set_supported_fan_modes(traits);
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
 * @note current elements identified in this frame are fan mode and defrost settings
 */
void FrameConf2::parse(heat_pump_data_t& hp_data) {
    hp_data.d01_defrost_start = data_->d01_defrost_start.decode();
    hp_data.d01_defrost_start = data_->d01_defrost_start.decode();
    hp_data.d03_defrosting_cycle_time_minutes = data_->d03_defrosting_cycle_time_minutes.decode();
    hp_data.d04_max_defrost_time_minutes = data_->d04_max_defrost_time_minutes.decode();
    hp_data.d02_defrost_end = data_->d02_defrost_end.decode();
    hp_data.d02_defrost_end = data_->d02_defrost_end.decode();
    hp_data.fan_mode = make_optional(data_->fan_mode.get_fan_mode());
}
} // namespace hwp
} // namespace esphome

REGISTER_FRAME_ID_DEFAULT(esphome::hwp::FrameConf2);
