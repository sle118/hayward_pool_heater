/**
 * @file BusPulse.h
 * @brief Defines the BusPulse class used to represent a pulse on the bus.
 *
 * Copyright (c) 2024 S. Leclerc (sle118@hotmail.com)
 *
 * * This file is part of the Pool Heater Controller component project.
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
namespace esphome {
namespace hayward_pool_heater {

/**
 * @class BusPulse
 * @brief Represents a pulse on the bus.
 *
 * This class encapsulates the level and duration of a pulse detected on the bus.
 */
class BusPulse {
 public:
  bool level;            ///< The level of the pulse (true for high, false for low).
  uint32_t duration_us;  ///< The duration of the pulse in microseconds.

  /**
   * @brief Constructs a new BusPulse object with specified level and duration.
   *
   * @param level The level of the pulse.
   * @param duration_us The duration of the pulse in microseconds.
   */
  BusPulse(bool level, uint32_t duration_us) : level(level), duration_us(duration_us) {}

  /**
   * @brief Constructs a new BusPulse object with default values.
   *
   * The default level is false (low) and the default duration is 0 microseconds.
   */
  BusPulse() : level(false), duration_us(0) {}
  
};

}  // namespace hayward_pool_heater
}