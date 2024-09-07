/**
 * @file command_frame.h
 * @brief Defines the CommandFrame class for handling command frames in the Heat Pump Controller.
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

#pragma once

#include "base_frame.h"
#include "esphome/components/climate/climate.h"
#include "esphome/core/application.h"
namespace esphome {
namespace hayward_pool_heater {

/**
 * @class CommandFrame
 * @brief A class to handle command frames in the Heat Pump Controller.
 *
 * This class extends the BaseFrame class to include additional functionality
 * specific to command frames.
 */
class CommandFrame : public BaseFrame {
 public:
  /**
   * @brief Default constructor. Initializes the transmit bit index.
   */
  CommandFrame() : BaseFrame(), transmitBitIndex(0) { this->previous_frame_.reset(); }
  CommandFrame(const BaseFrame &base) : BaseFrame(base) {}
  template<size_t N> CommandFrame(const unsigned char (&cmdTrame)[N]) : BaseFrame(cmdTrame) {}
  void save_previous(const BaseFrame &base) { this->previous_frame_ = base; }
  BaseFrame get_base_command() {
    BaseFrame frame;
    if (!this->previous_frame_.has_value()) {
      frame = model_cmd;
    }
    frame = this->previous_frame_.value();
    frame.set_source(SOURCE_CONTROLLER);
    return frame;
  }
  bool has_previous() const { return this->previous_frame_.has_value(); }

  void reset() {
    packet.tempprog.mode.auto_mode = false;
    packet.tempprog.mode.heat = false;
    packet.tempprog.mode.power = false;
  }
  void set_target_temperature_1(float temperature) {
    BaseFrame::set_temperature(packet.tempprog.set_point_1, temperature);
  }
  void set_target_temperature_2(float temperature) {
    BaseFrame::set_temperature(packet.tempprog.set_point_2, temperature);
  }
  void set_mode(climate::ClimateMode mode) {
    packet.tempprog.mode.power = true;
    switch (mode) {
      case climate::CLIMATE_MODE_AUTO:
        packet.tempprog.mode.auto_mode = true;
        break;
      case climate::CLIMATE_MODE_HEAT:
        packet.tempprog.mode.heat = true;
        break;
      case climate::CLIMATE_MODE_COOL:
      default:
        packet.tempprog.mode.power = false;
        break;
    }
  }

  /**
   * @brief Template copy assignment operator to accept an array of unsigned char of any length.
   * @param cmdTrame The array to copy from.
   * @return Reference to the assigned CommandFrame object.
   */
  template<size_t N> CommandFrame &operator=(const unsigned char (&cmdTrame)[N]) {
    BaseFrame::operator=(cmdTrame);
    return *this;
  }
  size_t transmitBitIndex;  ///< Index of the bit to be transmitted.
  optional<BaseFrame> previous_frame_;
};

}  // namespace hayward_pool_heater
}  // namespace esphome