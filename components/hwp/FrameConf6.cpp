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

#include "FrameConf6.h"
#include "CS.h"
#include "Schema.h"
namespace esphome {
namespace hwp {
CLASS_ID_DECLARATION(esphome::hwp::FrameConf6);
static constexpr char TAG[] = "hwp";
std::shared_ptr<BaseFrame> FrameConf6::create() {
    return std::make_shared<FrameConf6>(); // Create a FrameTemperature if type matches
}
const char* FrameConf6::type_string() const { return "CONFIG_6  "; }
bool FrameConf6::matches(BaseFrame& secialized, BaseFrame& base) {
    return base.packet.get_type() == FRAME_ID_CONF_6;
}
std::string FrameConf6::format(bool no_diff) const {
    if (!this->data_.has_value()) return "N/A";
    return this->format(*data_, (no_diff ? *data_ : prev_data_.value_or(*data_)));
}

std::string FrameConf6::format_prev() const {
    if (!this->prev_data_.has_value()) return "N/A";
    return this->format(this->prev_data_.value(), this->prev_data_.value());
}
void FrameConf6::traits(climate::ClimateTraits& traits, heat_pump_data_t& hp_data) {
    // N/A
}

std::string FrameConf6::format(const conf_6_t& val, const conf_6_t& ref) const {
    CS oss;
    bool changed = (val != ref);
    oss.set_changed_base_color(changed);

    oss << "[" << data_->unknown_1.diff(this->prev_data_->unknown_1, ", ")
        << data_->unknown_2.diff(this->prev_data_->unknown_2, ", ")
        << data_->unknown_3.diff(this->prev_data_->unknown_3, ", ")
        << data_->unknown_4.diff(this->prev_data_->unknown_4, ", ")
        << data_->unknown_5.diff(this->prev_data_->unknown_5, ", ") 
        << data_->unknown_6.diff(this->prev_data_->unknown_6, ", ")
        << data_->unknown_7.diff(this->prev_data_->unknown_7, ", ")
        << data_->unknown_8.diff(this->prev_data_->unknown_8, ", ")
        << data_->unknown_9.diff(this->prev_data_->unknown_9, "]");
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
 * @note no current elements identified in this frame
 */
void FrameConf6::parse(heat_pump_data_t& hp_data) {
    // N/A
}
optional<std::shared_ptr<BaseFrame>> FrameConf6::control(const HWPCall& call) {
    // Not supported yet.
    return nullopt;
}
} // namespace hwp
} // namespace esphome
REGISTER_FRAME_ID_DEFAULT(esphome::hwp::FrameConf6);
