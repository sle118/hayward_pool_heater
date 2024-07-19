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
  CommandFrame() : BaseFrame(), transmitBitIndex(0) {}

  /**
   * @brief Template copy assignment operator to accept an array of unsigned char of any length.
   * @param cmdTrame The array to copy from.
   * @return Reference to the assigned CommandFrame object.
   */
  template <size_t N>
  CommandFrame& operator=(const unsigned char (&cmdTrame)[N]) {
    BaseFrame::operator=(cmdTrame);
    return *this;
  }

  size_t transmitBitIndex;  ///< Index of the bit to be transmitted.
};

}  // namespace hayward_pool_heater
}