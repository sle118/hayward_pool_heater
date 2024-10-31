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

#include "FrameConf3.h"
#include "CS.h"
#include "Schema.h"
namespace esphome {
namespace hwp {
CLASS_ID_DECLARATION(esphome::hwp::FrameConf3);
static constexpr char TAG[] = "hwp";
std::shared_ptr<BaseFrame> FrameConf3::create() {
    return std::make_shared<FrameConf3>(); // Create a FrameTemperature if type matches
}
// FRAME_ID_t FrameConf3::get_type() const { return FRAME_SETPOINT_LIMITS; }
const char* FrameConf3::type_string() const { return "CONFIG_3  "; }
bool FrameConf3::matches(BaseFrame& secialized, BaseFrame& base) {
    return base.packet.get_type() == FRAME_ID_CONF_3;
}
std::string FrameConf3::format(bool no_diff) const {
    if (!this->data_.has_value()) return "N/A";
    return this->format(*data_, (no_diff ? *data_ : prev_data_.value_or(*data_)));
}

std::string FrameConf3::format_prev() const {
    if (!this->prev_data_.has_value()) return "N/A";
    return this->format(this->prev_data_.value(), this->prev_data_.value());
}
void FrameConf3::traits(climate::ClimateTraits& traits, heat_pump_data_t& hp_data) {
    traits.set_visual_min_temperature(hp_data.get_min_target());
    traits.set_visual_max_temperature(hp_data.get_max_target());
    traits.set_visual_temperature_step(0.5);
}

std::string FrameConf3::format(const conf_3_t& val, const conf_3_t& ref) const {
    CS oss;
    bool changed = (val != ref);
    oss.set_changed_base_color(changed);

    oss << "[" << data_->unknown_1.diff(this->prev_data_->unknown_1, ", ")
        << data_->unknown_2.diff(this->prev_data_->unknown_2, ", ")
        << data_->unknown_3.diff(this->prev_data_->unknown_3, ", ")
        << data_->unknown_4.diff(this->prev_data_->unknown_4, ", ")
        << data_->unknown_5.diff(this->prev_data_->unknown_5, "] ") << "r08_min_cool_setpoint: "
        << data_->r08_min_cool_setpoint.diff(this->prev_data_->r08_min_cool_setpoint, ", ")
        << "r09_max_cooling_setpoint: "
        << data_->r09_max_cooling_setpoint.diff(this->prev_data_->r09_max_cooling_setpoint, ", ")
        << "r10_min_heating_setpoint: "
        << data_->r10_min_heating_setpoint.diff(this->prev_data_->r10_min_heating_setpoint, ", ")
        << "r11_max_heating_setpoint: "
        << data_->r11_max_heating_setpoint.diff(this->prev_data_->r11_max_heating_setpoint);
    return oss.str();
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
 * @note current elements identified in this frame are setpoint limits
 */
void FrameConf3::parse(heat_pump_data_t& hp_data) {
    hp_data.r08_min_cool_setpoint = data_->r08_min_cool_setpoint.decode();
    hp_data.r09_max_cooling_setpoint = data_->r09_max_cooling_setpoint.decode();
    hp_data.r10_min_heating_setpoint = data_->r10_min_heating_setpoint.decode();
    hp_data.r11_max_heating_setpoint = data_->r11_max_heating_setpoint.decode();

    active_modes_t active_mode = STATE_HEATING_MODE;

    // Manually check the type and cast if it's FrameTemperature
    if (hp_data.mode.has_value() && hp_data.mode.value() == climate::CLIMATE_MODE_OFF) {
        active_mode = STATE_HEATING_MODE;
    }

    // Manually check the type and cast if it's FrameConf3
    auto min_heating_setpoint = data_->r10_min_heating_setpoint.decode();
    auto max_heating_setpoint = data_->r11_max_heating_setpoint.decode();
    auto min_cooling_setpoint = data_->r08_min_cool_setpoint.decode();
    auto max_cooling_setpoint = data_->r09_max_cooling_setpoint.decode();
    bits_details_t r08_bits;
    r08_bits.raw = data_->r08_min_cool_setpoint.raw;
    bits_details_t r09_bits;
    r09_bits.raw = data_->r09_max_cooling_setpoint.raw;
    bits_details_t r10_bits;
    r10_bits.raw = data_->r10_min_heating_setpoint.raw;
    bits_details_t r11_bits;
    r11_bits.raw = data_->r11_max_heating_setpoint.raw;

    switch (active_mode) {
    case STATE_HEATING_MODE:
        hp_data.min_target_temperature = min_heating_setpoint;
        hp_data.max_target_temperature = max_heating_setpoint;
        break;
    case STATE_COOLING_MODE:
        hp_data.min_target_temperature = min_cooling_setpoint;
        hp_data.max_target_temperature = max_cooling_setpoint;
        break;
    default:
        hp_data.min_target_temperature = min_cooling_setpoint;
        hp_data.max_target_temperature = max_heating_setpoint;
    }
}
optional<std::shared_ptr<BaseFrame>> FrameConf3::control(const HWPCall& call) {
    // Not supported yet.
    return nullopt;
}
} // namespace hwp
} // namespace esphome
REGISTER_FRAME_ID_DEFAULT(esphome::hwp::FrameConf3);
