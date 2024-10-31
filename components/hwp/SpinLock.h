/**
 * @file Spinlock.h
 * @brief Provides a Spinlock class for mutual exclusion using busy-waiting.
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

#include <atomic>
namespace esphome{
namespace hwp {

/**
 * @class Spinlock
 * @brief A simple spinlock implementation using std::atomic_flag.
 *
 * This class provides a mutual exclusion mechanism using busy-waiting.
 */
class Spinlock {
 public:
  /**
   * @brief Constructs a new Spinlock object.
   */
  Spinlock() : flag(ATOMIC_FLAG_INIT) {}

  /**
   * @brief Acquires the lock.
   *
   * This function will block in a busy-wait loop until the lock is acquired.
   */
  void inline lock() {
    while (flag.test_and_set(std::memory_order_acquire)) {
      // Spin-wait (busy-wait)
    }
  }

  /**
   * @brief Releases the lock.
   */
  void inline unlock() { flag.clear(std::memory_order_release); }

 private:
  std::atomic_flag flag;  ///< Atomic flag used for the spinlock.
};

}  // namespace hwp
}
