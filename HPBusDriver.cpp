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
#include "HPBusDriver.h"

#include "HPUtils.h"

#ifdef USE_ESP32
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#ifdef USE_ARDUINO
#include <esp32-hal.h>
#endif
#else
#error("Not using ESP32")
#endif
namespace esphome {
namespace hayward_pool_heater {
const uint8_t default_frame_transmit_count = 8;

// max duration would be all bits having their longest duration, plus the frame spacing and
// frame heading
const uint32_t single_frame_max_duration_ms =
    heater_packet_length_bytes * 8 * (bit_long_high_duration_ms + bit_low_duration_ms) +
    controler_frame_spacing_duration_ms + frame_heading_total_duration_ms;

const char* TAG = "hayward_pool_heater.climate";

HPBusDriver::HPBusDriver(size_t maxWriteLength, size_t transmitCount)
    : mode(BUSMODE_RX),
      ioTaskHandle(nullptr),
      maxBufferCount(maxBufferCount),
      maxWriteLength(maxWriteLength),
      next_controller_packet_ms(0),
      transmit_count(transmitCount),
      last_io_change_level(false),
      last_io_change_us(0),
      tx_packets_queue(maxWriteLength) {}

void HPBusDriver::setup() {
  this->current_frame.reset();
  start_receive();
}

bool HPBusDriver::queue_frame_data(CommandFrame frame) {
  ESP_LOGD(TAG, "Queueing frame data for transmission");
  frame.print();
  tx_packets_queue.enqueue(frame);
  return true;
}

void HPBusDriver::start_receive() {
  this->current_frame.reset();
  if (this->gpio_pin_ == nullptr) {
    ESP_LOGE(TAG, "Invalid pin. Cannot start receive");

  } else {
    ESP_LOGI(TAG, "Starting reception on pin %d", this->gpio_pin_->get_pin());
    this->mode = BUSMODE_RX;
    this->gpio_pin_->pin_mode(gpio::Flags::FLAG_PULLUP | gpio::Flags::FLAG_INPUT);
    this->gpio_pin_->attach_interrupt(&HPBusDriver::handlePulse, this, gpio::INTERRUPT_ANY_EDGE);
    // reset the change detection to what's now on the bus
    this->last_io_change_us = micros();
    this->last_io_change_level = this->gpio_pin_->digital_read();
    if (this->last_io_change_level) {
      this->last_io_high_us = this->last_io_change_us;
    }
    if (this->ioTaskHandle == nullptr) {
      ESP_LOGI(TAG, "Creating ioTask");
      xTaskCreate(ioTask, "ioTask", 2048, this, 1, &this->ioTaskHandle);
    }
  }
}

bool HPBusDriver::get_next_frame(DecodingStateFrame* frame) {
  if (!has_next_frame()) return false;
  if (!frame) {
    ESP_LOGE(TAG, "Invalid frame pointer!");
    return false;
  }
  ESP_LOGD(TAG, "Getting next frame from the queue");
  return this->received_frames.try_dequeue(frame);
}

bool HPBusDriver::has_time_to_send() {
  auto end_of_transmit = millis() + this->transmit_count * single_frame_max_duration_ms;
  bool result = !this->has_controller() ||
      (this->next_controller_packet_ms > 0 && end_of_transmit < this->next_controller_packet_ms) ;
  ESP_LOGV(TAG, "Has time to send: %s", result ? "true" : "false");
  return result;
}

void HPBusDriver::process_send_queue() {
  CommandFrame packet;
  if (this->has_time_to_send()) {
    if (this->tx_packets_queue.try_dequeue(&packet)) {
      this->mode = BUSMODE_TX;
      this->current_frame.reset();
      ESP_LOGI(TAG, "Processing send queue, type: %s", packet.type_string());
      auto transmitCount = this->transmit_count;
      this->gpio_pin_->pin_mode(gpio::Flags::FLAG_OUTPUT);
      while (transmitCount > 0) {
        sendHeader();
        size_t transmitIndex = 0;
        while (transmitIndex < packet.size()) {
          for (int bitIndex = 0; bitIndex <= 7; bitIndex++) {
            if (get_bit(packet[transmitIndex], bitIndex)) {
              sendBinary1();
            } else {
              sendBinary0();
            }
          }
          transmitIndex++;
        }
        sendGroupSpacing();

        transmitCount--;
        if (transmitCount > 0) {
          sendFrameSpacing();
        } else {
          sendGroupSpacing();
        }
      }
      start_receive();
    } else {
      ESP_LOGV(TAG, "Nothing in the send queue");
    }

  } else {
    if(this->controler_packets_received_.has_value()){
      ESP_LOGW(TAG, "No time to send before next panel message. Waiting.");
    }
  }
}

void HPBusDriver::process_pulse(const BusPulse& pulse) {
  if (pulse.level) {
    this->current_frame.set_last_high_duration(pulse.duration_us);
  } else {
    this->current_frame.set_last_low_duration(pulse.duration_us);
  }
  // process on level going down.
  if (!pulse.level && this->current_frame.get_last_high_duration() > 0) {
    this->current_frame.set_last_high_start(0);
    if (current_frame.is_start_frame()) {
      ESP_LOGI(TAG, "Start frame detected");
      current_frame.start_new_frame();
    } else {
      if (current_frame.is_long_bit()) {
        // Long bit detected
        current_frame.append_bit(true);
      } else if (current_frame.is_short_bit()) {
        // Short bit detected
        current_frame.append_bit(false);
      } else {
        // invalid length. This could happen
        // during collisions of frame when both the
        // controller and heater transmit at the same time
        ESP_LOGW(TAG, "Invalid pulse length detected, resetting frame");
        current_frame.reset();
      }
    }
  }
}

void HPBusDriver::ioTask(void* arg) {
  HPBusDriver* instance = static_cast<HPBusDriver*>(arg);
  BusPulse pulse;
  // set task signalling handles on both queues so we can
  // block the task when no data is available
  instance->pulse_queue.set_task_handle(xTaskGetCurrentTaskHandle());
  instance->tx_packets_queue.set_task_handle(xTaskGetCurrentTaskHandle());

  ESP_LOGI(TAG, "Starting ioTask");
  while (true) {
    // wait for frames to send, pulses up to the duration of the
    // threshold that signals the end of a frame
    ulTaskNotifyTake(pdTRUE,
                     (instance->current_frame.is_started() || instance->tx_packets_queue.has_next())
                         ? pdMS_TO_TICKS(frame_end_threshold_ms)
                         : pdMS_TO_TICKS(1000));
    ESP_LOGV(TAG, "ioTask waking up");
    if (instance->pulse_queue.has_next()) {
      ESP_LOGV(TAG, "Pulses decoding needed");
      while (instance->pulse_queue.dequeue(&pulse)) {
        // prioritize decoding of pulses
        instance->process_pulse(pulse);
      }

    } else {
      // no more pulse, check if bus has end frame signal
      if (instance->gpio_pin_ != nullptr && instance->gpio_pin_->digital_read() &&
          instance->last_high_duration_matches(frame_end_threshold_ms)) {
        // Frame is hard ended
        if (instance->current_frame.finalize()) {
          ESP_LOGD(TAG, "New Frame finalized");
          if (instance->current_frame.get_source() == SOURCE_CONTROLLER) {
            instance->controler_packets_received_ = true;
            // next controller packet expected
            instance->next_controller_packet_ms = delay_between_controller_messages_ms + millis();
          }
          instance->addReceivedFrame();
        }
        instance->current_frame.reset();
      }
    }
    // process send queue if needed
    instance->process_send_queue();
  }
}

void IRAM_ATTR HPBusDriver::handlePulse(HPBusDriver* instance) { instance->handlePulseInternal(); }

void IRAM_ATTR HPBusDriver::handlePulseInternal() {
  if (this->mode == BUSMODE_TX) {
    return;
  }

  uint32_t currentTimestamp = micros();
  if (this->last_io_change_us > 0) {
    pulse_queue.enqueue(
        BusPulse(this->last_io_change_level, currentTimestamp - this->last_io_change_us));
  }
  this->last_io_change_us = currentTimestamp;
  this->last_io_change_level = this->gpio_pin_->digital_read();
  if (this->last_io_change_level) {
    this->last_io_high_us = this->last_io_change_us;
  }
}

void HPBusDriver::addReceivedFrame() {
  ESP_LOGI(TAG, "Adding received frame to queue. Source: %s", this->current_frame.source_string());
  this->received_frames.enqueue(this->current_frame);
}

void HPBusDriver::sendBinary0() {
  if (this->gpio_pin_ == nullptr) return;

  _sendLow(bit_low_duration_ms);
  _sendHigh(bit_short_high_duration_ms);
}

void HPBusDriver::sendBinary1() {
  if (this->gpio_pin_ == nullptr) return;

  _sendLow(bit_low_duration_ms);
  _sendHigh(bit_long_high_duration_ms);
}

void HPBusDriver::sendHeader() {
  if (this->gpio_pin_ == nullptr) return;

  ESP_LOGV(TAG, "Sending frame header");

  _sendLow(frame_heading_low_duration_ms);
  _sendHigh(frame_heading_high_duration_ms);
}

void HPBusDriver::sendFrameSpacing() {
  if (this->gpio_pin_ == nullptr) return;

  ESP_LOGV(TAG, "Sending frame spacing");

  _sendLow(bit_low_duration_ms);
  _sendHigh(controler_frame_spacing_duration_ms);
}

void HPBusDriver::sendGroupSpacing() {
  if (this->gpio_pin_ == nullptr) return;

  ESP_LOGV(TAG, "Sending group spacing");

  _sendLow(bit_low_duration_ms);
  _sendHigh(controler_group_spacing_ms);
}
}  // namespace hayward_pool_heater
}  // namespace esphome