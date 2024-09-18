/**
 * @file SpinLockQueue.h
 * @brief Provides a template-based thread-safe queue with spinlock and FreeRTOS synchronization.
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

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <deque>

#include "SpinLock.h"
#include "esphome/core/log.h"

namespace esphome {
namespace hayward_pool_heater {
extern const char* SPINLOCK_TAG;
/**
 * @class SpinLockQueue
 * @brief A thread-safe queue using spinlock and FreeRTOS semaphores for synchronization.
 *
 * This class provides a queue that supports concurrent access, using a spinlock
 * for mutual exclusion and FreeRTOS semaphores for signaling and blocking.
 */
template <typename T>
class SpinLockQueue {
 public:
  /**
   * @brief Constructs a new SpinLockQueue object with a default maximum length of 20.
   */
  SpinLockQueue() : max_len_(20), task_handle(nullptr) {
    this->data_available = xSemaphoreCreateBinary();
  }

  /**
   * @brief Constructs a new SpinLockQueue object with a specified maximum length.
   * @param max_len Maximum number of elements in the queue.
   */
  explicit SpinLockQueue(size_t max_len) : max_len_(max_len), task_handle(nullptr) {
    this->data_available = xSemaphoreCreateBinary();
  }

  /**
   * @brief Destroys the SpinLockQueue object and deletes the semaphore.
   */

  ~SpinLockQueue() {
    if (this->data_available != nullptr) {
      vSemaphoreDelete(this->data_available);
    }
  }

  /**
   * @brief Sets the maximum size of the queue.
   * @param max_len Maximum number of elements in the queue.
   */
  void set_max_size(size_t max_len) { this->max_len_ = max_len; }

  /**
   * @brief Sets the task handle to notify when new data is available in the queue.
   * @param handle Task handle to notify.
   */
  void set_task_handle(TaskHandle_t handle) {
    ESP_LOGV(SPINLOCK_TAG, "set_task_handle: setting task handle");
    this->task_handle = handle;
  }

  /**
   * @brief Enqueues an element to the queue.
   *
   * If the queue exceeds the maximum length, the oldest element is removed.
   *
   * @param element The element to enqueue.
   */
  void inline enqueue(const T& element) {
    this->spinlock.lock();
    while (this->queue.size() > this->max_len_) {
      this->queue.pop_front();
    }
    this->queue.push_back(element);
    this->spinlock.unlock();
    xSemaphoreGive(this->data_available);
    if (this->task_handle != nullptr) {
      xTaskNotifyGive(this->task_handle);
    }
  }

  /**
   * @brief Checks if the queue has next element.
   * @return true If the queue is not empty.
   * @return false If the queue is empty.
   */
  bool has_next() { return !this->queue.empty(); }

  /**
   * @brief Dequeues an element from the queue.
   *
   * This function blocks until an element is available.
   *
   * @param element The element to dequeue.
   * @return true If an element was successfully dequeued.
   * @return false If the queue is empty.
   */
  bool dequeue(T* element) {
    ESP_LOGV(SPINLOCK_TAG, "dequeue: taking semaphore");
    if (xSemaphoreTake(this->data_available, portMAX_DELAY) == pdTRUE) {
      ESP_LOGV(SPINLOCK_TAG, "dequeue: Locking spinlock");
      this->spinlock.lock();
      if (!this->queue.empty()) {
        *element = this->queue.front();
        this->queue.pop_front();
        this->spinlock.unlock();
        ESP_LOGV(SPINLOCK_TAG, "dequeue: data found");
        return true;
      }
      this->spinlock.unlock();
    }
    ESP_LOGV(SPINLOCK_TAG, "dequeue: data not found");
    return false;
  }

  /**
   * @brief Attempts to dequeue an element from the queue without blocking.
   * @param element The element to dequeue.
   * @return true If an element was successfully dequeued.
   * @return false If the queue is empty.
   */
  bool try_dequeue(T* element) {
    ESP_LOGV(SPINLOCK_TAG, "try_dequeue: trying semaphore");
    if (xSemaphoreTake(this->data_available, 0) == pdTRUE) {
      ESP_LOGV(SPINLOCK_TAG, "try_dequeue: Locking spinlock");
      this->spinlock.lock();
      if (!this->queue.empty()) {
        *element = this->queue.front();
        this->queue.pop_front();
        this->spinlock.unlock();
        ESP_LOGV(SPINLOCK_TAG, "try_dequeue: found data");
        return true;
      }
      this->spinlock.unlock();
    }
    ESP_LOGV(SPINLOCK_TAG, "try_dequeue: no data");
    return false;
  }

 private:
  std::deque<T> queue;               ///< Internal queue storage.
  Spinlock spinlock;                 ///< Spinlock for mutual exclusion.
  size_t max_len_;                   ///< Maximum number of elements in the queue.
  SemaphoreHandle_t data_available;  ///< Semaphore to signal availability of data.
  TaskHandle_t task_handle;          ///< Task handle to notify when new data is available.
};

}  // namespace hayward_pool_heater
}  // namespace esphome
