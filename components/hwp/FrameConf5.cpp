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

#include "FrameConf5.h"
#include "CS.h"
#include "Schema.h"
namespace esphome {
namespace hwp {
CLASS_ID_DECLARATION(esphome::hwp::FrameConf5);
static constexpr char TAG[] = "hwp";
std::shared_ptr<BaseFrame> FrameConf5::create() {
    return std::make_shared<FrameConf5>(); // Create a FrameTemperature if type matches
}
bool FrameConf5::matches(BaseFrame& secialized, BaseFrame& base) {
    return base.packet.get_type() == FRAME_ID_CONF_5;
}
const char* FrameConf5::type_string() const { return "CONFIG_5  "; }
std::string FrameConf5::format(bool no_diff) const {
    if (!this->data_.has_value()) return "N/A";
    return this->format(*data_, (no_diff ? *data_ : prev_data_.value_or(*data_)));
}

std::string FrameConf5::format_prev() const {
    if (!this->prev_data_.has_value()) return "N/A";
    return this->format(this->prev_data_.value(), this->prev_data_.value());
}

std::string FrameConf5::format(const conf_5_t& val, const conf_5_t& ref) const {
    CS cs;
    bool changed = (val != ref);
    cs.set_changed_base_color(changed);

    const uint8_t* val_bytes = reinterpret_cast<const uint8_t*>(&val);
    const uint8_t* ref_bytes = reinterpret_cast<const uint8_t*>(&ref);
    // cs << "[";
    // for (size_t i = 0; i < sizeof(conf_5_t) - 1; ++i) {
    //     cs << format_hex_diff(val_bytes[i], ref_bytes[i]) << " ";
    // }
    // cs << "]";

    cs << "U01: " << val.flags_a.format(ref.flags_a, ", ")
       << "d05_min_economy_defrost_time_minutes: "
       << val.d05_min_economy_defrost_time_minutes.diff(
              ref.d05_min_economy_defrost_time_minutes, ", [")
       << val.unknown_4.diff(ref.unknown_4, ", ") << val.unknown_5.diff(ref.unknown_5, ", ")
       << val.unknown_6.diff(ref.unknown_6, ", ") << val.unknown_7.diff(ref.unknown_7, ", ")
       << val.unknown_7.diff(ref.unknown_7)
       << "] U02 Pulses/L: " << val.U02_pulses_per_liter.diff(ref.U02_pulses_per_liter);

    return cs.str();
}
optional<std::shared_ptr<BaseFrame>> FrameConf5::control(const HWPCall& call) {
    FrameConf5 command_frame(*this);
    bool has_data = this->data_.has_value();
    if (call.d06_defrost_eco_mode.has_value()) {
        command_frame.data().flags_a.set_eco_mode(call.d06_defrost_eco_mode.value());
    }
    if (call.u01_flow_meter.has_value()) {
        command_frame.data().flags_a.set_flow_meter(call.u01_flow_meter.value());
    }
    if (call.d05_min_economy_defrost_time_minutes.has_value()) {
        command_frame.data().d05_min_economy_defrost_time_minutes =
            call.d05_min_economy_defrost_time_minutes.value();
    }

    if (call.u02_pulses_per_liter.has_value()) {
        command_frame.data().U02_pulses_per_liter = call.u02_pulses_per_liter.value();
    }
    if(!command_frame.is_changed() && has_data)  {
        ESP_LOGD(TAG, "No changes for frame ConfA");
        return nullopt;
    }

    if (!has_data) {
        ESP_LOGW(TAG, "FrameConfA: cannot publish changes yet. waiting for first packet");
        call.component.status_momentary_warning("No data available", 5000);
        return nullopt;
    }
    command_frame.finalize();
    command_frame.print("TXQ", TAG, ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
    return optional<std::shared_ptr<FrameConf5>>{std::make_shared<FrameConf5>(command_frame)};
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
 * @note current elements identified in this frame are defrost eco mode, flow meter sensor enabled
 * and min defrost time
 */
void FrameConf5::parse(heat_pump_data_t& hp_data) {
    hp_data.d06_defrost_eco_mode = data_->flags_a.get_eco_mode();
    hp_data.U01_flow_meter = data_->flags_a.get_flow_meter();
    hp_data.d05_min_economy_defrost_time_minutes =
        data_->d05_min_economy_defrost_time_minutes.decode();
    hp_data.U02_pulses_per_liter = data_->U02_pulses_per_liter.decode();
}
} // namespace hwp
} // namespace esphome
REGISTER_FRAME_ID_DEFAULT(esphome::hwp::FrameConf5);
