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

#include "FrameConditions2.h"
#include "CS.h"
#include "Schema.h"
namespace esphome {
namespace hwp {
CLASS_ID_DECLARATION(esphome::hwp::FrameConditions2);
std::shared_ptr<BaseFrame> FrameConditions2::create() {
    return std::make_shared<FrameConditions2>(); // Create a FrameTemperature if type matches
}
bool FrameConditions2::matches(BaseFrame& secialized, BaseFrame& base) {
    return base.packet.get_type() == FRAME_ID_CONDITIONS2 && (base.size() == frame_data_length);
}
const char* FrameConditions2::type_string() const { return "COND_2    "; }
float FrameConditions2::get_out_temp() const { return this->data_->t03_temperature.decode(); }
float FrameConditions2::get_exhaust_temp() const {
    return this->data_->t06_temperature_exhaust.decode();
}
float FrameConditions2::get_coil_temp() const { return this->data_->t04_temperature_coil.decode(); }

std::string FrameConditions2::format(bool no_diff) const {
    if (!this->data_.has_value()) return "N/A";
    return this->format(*data_, (no_diff ? *data_ : prev_data_.value_or(*data_)));
}

std::string FrameConditions2::format_prev() const {
    if (!this->prev_data_.has_value()) return "N/A";
    return this->format(this->prev_data_.value(), this->prev_data_.value());
}
optional<std::shared_ptr<BaseFrame>> FrameConditions2::control(const HWPCall& call) {
    // Not supported yet.
    return nullopt;
}
std::string FrameConditions2::format(const conditions2_t& val, const conditions2_t& ref) const {
    CS oss;
    bool changed = (val != ref);
    oss.set_changed_base_color(changed);

    auto cs_inv = (changed ? CS::invert : "");
    auto cs_inv_rst = (changed ? CS::invert_rst : "");
    oss << val.t03_temperature.diff(ref.t03_temperature, ", ") << "t04 Coil "
        << val.t04_temperature_coil.diff(ref.t04_temperature_coil, ", ") << "t06 Exhaust "
        << val.t06_temperature_exhaust.diff(ref.t06_temperature_exhaust, ", ") << "4?? "
        << val.temperature_4.diff(ref.temperature_4, ", ") << "R12["
        << val.reserved_1.diff(ref.reserved_1, ", ") << val.reserved_2.diff(ref.reserved_2, "], ")
        << "R578[" << val.reserved_5.diff(ref.reserved_5, ",")
        << val.reserved_7.diff(ref.reserved_7, ",") << val.reserved_8.diff(ref.reserved_8, "] ");
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
 * @note current elements identified in this frame are outlet temperature, coil temperature and
 * exhaust temperature
 */
void FrameConditions2::parse(heat_pump_data_t& hp_data) {
    hp_data.t03_temperature_outlet = data_->t03_temperature.decode();
    hp_data.t04_temperature_coil = data_->t04_temperature_coil.decode();
    hp_data.t06_temperature_exhaust = data_->t06_temperature_exhaust.decode();
}

} // namespace hwp
} // namespace esphome
REGISTER_FRAME_ID_DEFAULT(esphome::hwp::FrameConditions2);
