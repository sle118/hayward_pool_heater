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

#include "FrameConditions1B.h"
#include "CS.h"
#include "FrameConditions1.h"
#include "Schema.h"
namespace esphome {
namespace hwp {
CLASS_ID_DECLARATION(esphome::hwp::FrameConditions1B);

/**
 * @brief Factory method to create a new instance of FrameConditions1B.
 *
 * @return A shared pointer to a newly created FrameConditions1B instance.
 */
std::shared_ptr<BaseFrame> FrameConditions1B::create() {
    return std::make_shared<FrameConditions1B>(); // Create a FrameTemperature if type matches
}
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
bool FrameConditions1B::matches(BaseFrame& secialized, BaseFrame& base) {
    conditions_1b_t data_check = base.packet;
    return base.packet.get_type() == FrameConditions1::FRAME_ID_CONDITIONS_1 &&
           data_check.reserved_1.raw != 0x05;
}
/**
 * @brief Returns the frame type as a string.
 *
 * This method returns the frame type as a string, which is used for logging and
 * debugging purposes. The returned string is "COND_1B   ".
 *
 * @return The frame type as a string.
 */
const char* FrameConditions1B::type_string() const { return "COND_1B   "; }

std::string FrameConditions1B::format(bool no_diff) const {
    if (!this->data_.has_value()) return "N/A";
    return this->format(*data_, (no_diff ? *data_ : prev_data_.value_or(*data_)));
}
optional<std::shared_ptr<BaseFrame>> FrameConditions1B::control(const HWPCall& call) {
    // Not supported yet.
    return nullopt;
}
/**
 * @brief Format the previous frame data.
 *
 * @return A string containing the formatted frame data
 */
std::string FrameConditions1B::format_prev() const {
    if (!this->prev_data_.has_value()) return "N/A";
    return this->format(this->prev_data_.value(), this->prev_data_.value());
}

/**
 * @brief Format the frame data and difference with previous frame (if `no_diff` is false) into a
 * string.
 *
 * @param val The frame data to format.
 * @param prev The previous frame data to compare with.
 *
 * @return A string containing the formatted frame data and difference with the previous frame.
 *
 * @note The previous frame is only computed if `no_diff` is false.
 */
std::string FrameConditions1B::format(
    const conditions_1b_t& val, const conditions_1b_t& prev) const {
    CS oss;
    bool changed = (val != prev);
    oss.set_changed_base_color(changed);

    const char* S02_flow = get_flow_string(val.S02_water_flow);
    const char* prev_S02_flow = get_flow_string(prev.S02_water_flow);
    oss << "t02_inlet:" << val.t02_temperature.diff(prev.t02_temperature, ", ") << "R16["
        << val.reserved_1.diff(prev.reserved_1, ", ")
        << val.reserved_2.diff(prev.reserved_2, 0, 1, ", ") << std::setw(strlen(S02_flow))
        << format_diff(S02_flow, prev_S02_flow, ", ")
        << val.reserved_2.diff(prev.reserved_2, 2, 6, ", ")
        << val.reserved_4.diff(prev.reserved_4, ", ") << val.reserved_5.diff(prev.reserved_5, ", ")
        << val.reserved_6.diff(prev.reserved_6, "], R8[")
        << val.reserved_8.diff(prev.reserved_8, "]");
    return oss.str();
}
/**
 * @brief Returns a string representation of the water flow state.
 *
 * This method provides a human-readable string indicating whether water
 * is flowing based on the input boolean value.
 *
 * @param flow A boolean indicating the water flow state.
 * @return "FLOWING" if water flow is detected, "NOT FLOWING" otherwise.
 */
const char* FrameConditions1B::get_flow_string(bool flow) {
    return flow ? "FLOWING    " : "NOT FLOWING";
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
 * @note current elements identified in this frame are water flowing state and inlet temperature
 */
void FrameConditions1B::parse(heat_pump_data_t& hp_data) {
    hp_data.S02_water_flow = data_->get_flow_meter_enable();
    hp_data.t02_temperature_inlet = data_->t02_temperature.decode();
}
} // namespace hwp
} // namespace esphome
REGISTER_FRAME_ID_DEFAULT(esphome::hwp::FrameConditions1B);
