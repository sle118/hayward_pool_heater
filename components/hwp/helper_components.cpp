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
#include "helper_components.h"
#include "Schema.h"
#include "base_frame.h"
namespace esphome {

namespace hwp {
void GenerateCodeButton::press_action() {
    auto parent = this->get_parent();
    parent->generate_code();
}
void d01_defrost_start_number::control(float value) {
    auto parent=this->get_parent();
    HWPCall call_data = parent->instantiate_call();
    call_data.d01_defrost_start = value;
    this->get_parent()->control(call_data);
}
void d02_defrost_end_number::control(float value) {
    HWPCall call_data = this->get_parent()->instantiate_call();
    call_data.d02_defrost_end = value;
    this->get_parent()->control(call_data);
}
void d03_defrosting_cycle_time_minutes_number::control(float value) {
    HWPCall call_data = this->get_parent()->instantiate_call();
    call_data.d03_defrosting_cycle_time_minutes = value;
    this->get_parent()->control(call_data);
}
void d04_max_defrost_time_minutes_number::control(float value) {
    HWPCall call_data = this->get_parent()->instantiate_call();
    call_data.d04_max_defrost_time_minutes = value;
    this->get_parent()->control(call_data);
}
void d05_min_economy_defrost_time_minutes_number::control(float value) {
    HWPCall call_data = this->get_parent()->instantiate_call();
    call_data.d05_min_economy_defrost_time_minutes = value;
    this->get_parent()->control(call_data);
}

void r04_return_diff_cooling_number::control(float value) {
    HWPCall call_data = this->get_parent()->instantiate_call();
    call_data.r04_return_diff_cooling = value;
    this->get_parent()->control(call_data);
}
void r05_shutdown_temp_diff_when_cooling_number::control(float value) {
    HWPCall call_data = this->get_parent()->instantiate_call();
    call_data.r05_shutdown_temp_diff_when_cooling = value;
    this->get_parent()->control(call_data);
}
void r06_return_diff_heating_number::control(float value) {
    HWPCall call_data = this->get_parent()->instantiate_call();
    call_data.r06_return_diff_heating = value;
    this->get_parent()->control(call_data);
}
void r07_shutdown_diff_heating_number::control(float value) {
    HWPCall call_data = this->get_parent()->instantiate_call();
    call_data.r07_shutdown_diff_heating = value;
    this->get_parent()->control(call_data);
}
void u02_pulses_per_liter_number::control(float value) {
    HWPCall call_data = this->get_parent()->instantiate_call();
    call_data.u02_pulses_per_liter = value;
    this->get_parent()->control(call_data);
}

void d06_defrost_eco_mode_select::control(const std::string& value) {
    HWPCall call_data = this->get_parent()->instantiate_call();
    call_data.d06_defrost_eco_mode = make_optional(value);
    this->get_parent()->control(call_data);
}
void u01_flow_meter_select::control(const std::string& value) {
    HWPCall call_data = this->get_parent()->instantiate_call();
    call_data.u01_flow_meter = make_optional(value);
    this->get_parent()->control(call_data);
}
void h02_mode_restrictions_select::control(const std::string& value) {
    HWPCall call_data = this->get_parent()->instantiate_call();
    call_data.h02_mode_restrictions = make_optional(value);
    this->get_parent()->control(call_data);
}

} // namespace hwp

} // namespace esphome