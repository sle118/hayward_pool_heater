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

#include "FrameConditions1.h"
#include "CS.h"
#include "Schema.h"
namespace esphome {
namespace hwp {
CLASS_ID_DECLARATION(esphome::hwp::FrameConditions1);

/**
 * @brief Factory method to create a new instance of FrameConditions1.
 *
 * @return A shared pointer to a newly created FrameConditions1 instance.
 */
std::shared_ptr<BaseFrame> FrameConditions1::create() {
    return std::make_shared<FrameConditions1>(); // Create a FrameTemperature if type matches
}

const char* FrameConditions1::type_string() const { return "COND_1    "; }
/**
 * @brief Checks if the given frame matches the current frame class.
 *
 * This method will be called by the frame registry to determine which frame class to use
 * for the given frame.
 *
 * @param secialized The frame to check.
 * @param base The frame to check against.
 *
 * @return true if the frame matches, false otherwise.
 */
bool FrameConditions1::matches(BaseFrame& secialized, BaseFrame& base) {
    conditions_1_t data_check = base.packet;
    return base.packet.get_type() == FRAME_ID_CONDITIONS_1 && data_check.reserved_1.raw == 0x05;
}

/**
 * @brief Format the frame data and difference with previous frame (if `no_diff` is false) into a string.
 *
 * @param no_diff If true, the difference with the previous frame is not computed.
 *
 * @return A string containing the formatted frame data and difference with the previous frame.
 *
 * @note The previous frame is only computed if `no_diff` is false.
 */
std::string FrameConditions1::format(bool no_diff) const {
    if (!this->data_.has_value()) return "N/A";
    return this->format(*data_, (no_diff ? *data_ : prev_data_.value_or(*data_)));
}

/**
 * @brief Format the previous frame data.
 *
 * @return A string containing the formatted frame data
 */
std::string FrameConditions1::format_prev() const {
    if (!this->prev_data_.has_value()) return "N/A";
    return this->format(this->prev_data_.value(), this->prev_data_.value());
}
/**
 * @brief Controls the heat pump based on the call.
 *
 * @param call The call that describes the action to take on the heat pump.
 *
 * @return An optional specialized BaseFrame that represents the new frame to send to the heat pump
 *         if the call resulted in a change to the heat pump configuration, or an empty
 *         optional if no change was made.
 *
 * @note This is currently not supported.
 */
optional<std::shared_ptr<BaseFrame>> FrameConditions1::control(const HWPCall& call) {
    // Not supported yet.
    return nullopt;
}
/**
 * @brief Format the frame data and difference with previous frame
 *        (if `ref` is given) into a string.
 *
 * @param val The current frame data
 * @param ref The previous frame data (optional)
 * @return A string containing the formatted frame data
 */
std::string FrameConditions1::format(const conditions_1_t& val, const conditions_1_t& ref) const {
    CS oss;
    bool changed = (val != ref);
    oss.set_changed_base_color(changed);

    oss << val.t02_temperature.diff(ref.t02_temperature, ", ") << "R16["
        << val.reserved_1.diff(ref.reserved_1, ", ") << val.reserved_2.diff(ref.reserved_2, ", ")
        << val.reserved_3.diff(ref.reserved_3, ", ") << val.reserved_4.diff(ref.reserved_4, ", ")
        << val.reserved_5.diff(ref.reserved_5, ", ") << val.reserved_6.diff(ref.reserved_6, ", ")
        << val.reserved_7.diff(ref.reserved_7, "], ") << "R8"
        << val.reserved_8.diff(ref.reserved_8);
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
 * @note current elements identified in this frame are inlet temperature
 */
void FrameConditions1::parse(heat_pump_data_t& hp_data) {
    hp_data.t02_temperature_inlet = data_->t02_temperature.decode();
}

} // namespace hwp
} // namespace esphome
REGISTER_FRAME_ID_DEFAULT(esphome::hwp::FrameConditions1);
