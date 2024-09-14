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
#include "esp32/clk.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#ifdef USE_ARDUINO
#include <esp32-hal.h>
#endif
#else
#define millis  (uint32_t) (esp_timer_get_time() / 1000)

#endif
namespace esphome {
namespace hayward_pool_heater {
const uint8_t default_frame_transmit_count = 8;

// max duration would be all bits having their longest duration, plus the frame spacing and
// frame heading
const uint32_t single_frame_max_duration_ms =
    frame_data_length * 8 * (bit_long_high_duration_ms + bit_low_duration_ms) + controler_frame_spacing_duration_ms +
    frame_heading_total_duration_ms;

const char *TAG_BUS = "hayward_pool_heater.bus";
const char *TAG = "hayward_pool_heater.driver";
const char *TAG_PACKET = "pk";

HPBusDriver::HPBusDriver(size_t maxWriteLength, size_t transmitCount)
    : mode(BUSMODE_RX),
      TxTaskHandle(nullptr),
      RxTaskHandle(nullptr),
      transmit_count(transmitCount),
      // maxBufferCount(maxBufferCount),
      maxWriteLength(maxWriteLength),
      tx_packets_queue(maxWriteLength) {}

void HPBusDriver::setup() {
  this->current_frame.reset("From setup");
  start_receive();
}
void IRAM_ATTR HPBusDriver::process_pulse(rmt_item32_t *item) {
  if (!item)
    return;
  this->log_pulse_item(item);
  if (DecodingStateFrame::is_start_frame(item)) {
    ESP_LOGV(TAG_BUS, "Start frame detected");
    if (this->current_frame.is_complete()) {
      ESP_LOGV(TAG_BUS, "Finalizing complete frame");
      this->finalize_frame(false);

    } else if (this->current_frame.is_started()) {
      ESP_LOGV(TAG_BUS, "Resetting incomplete frame");
      if (!this->current_frame.is_started())
        ESP_LOGV(TAG_BUS, "Not started");
      if (!(this->current_frame.is_long_frame() || this->current_frame.is_short_frame()))
        ESP_LOGV(TAG_BUS, "Invalid length");
      if (!this->current_frame.is_checksum_valid())
        ESP_LOGV(TAG_BUS, "Invalid checksum");
      if (this->current_frame.is_size_valid()) {
        this->current_frame.debug("Starting new frame");
      }
    }
    this->current_frame.reset("Starting new frame");
    this->current_frame.start_new_frame();
  } else {
    if (this->current_frame.is_started()) {
      bool is_long = DecodingStateFrame::is_long_bit(item);
      bool is_short = DecodingStateFrame::is_short_bit(item);
      if (is_long || is_short) {
        // Long bit detected
        this->current_frame.append_bit(is_long);
      } else {
        if (DecodingStateFrame::get_high_duration(item) == 0 || DecodingStateFrame::get_low_duration(item) == 0 ||
            DecodingStateFrame::matches_duration(DecodingStateFrame::get_high_duration(item), frame_end_threshold_ms)) {
          if (this->current_frame.is_complete()) {
            this->finalize_frame(true);
          } else {
            ESP_LOGVV(TAG_BUS, "Bus idle state detected (1:%dus/0:%d)", DecodingStateFrame::get_high_duration(item),
                      DecodingStateFrame::get_low_duration(item));
          }
        } else {
          if (this->current_frame.is_complete()) {
            this->finalize_frame(true);
          } else {
            // Invalid length, possibly due to collisions
            ESP_LOGD(TAG_BUS, "Invalid pulse length detected (1:%dus/0:%d)", DecodingStateFrame::get_high_duration(item),
                     DecodingStateFrame::get_low_duration(item));
            if (this->current_frame.is_size_valid()) {
              this->current_frame.debug("Invalid frame - ");
            }
            this->current_frame.reset("Invalid pulse and invalid frame");
          }
        }
      }
    }
  }
}
bool HPBusDriver::queue_frame_data(const CommandFrame &frame) {
  ESP_LOGD(TAG, "Queueing frame data for transmission");
  tx_packets_queue.enqueue(frame);
  return true;
}
void HPBusDriver::start_receive() {
  this->current_frame.reset("Starting receive");
  if (this->gpio_pin_ == nullptr) {
    ESP_LOGE(TAG, "Invalid pin. Cannot start receive");

  } else {
    ESP_LOGI(TAG, "Starting reception on pin %d", this->gpio_pin_->get_pin());
    this->mode = BUSMODE_RX;
    this->gpio_pin_->pin_mode(gpio::Flags::FLAG_PULLUP | gpio::Flags::FLAG_INPUT);
    this->gpio_pin_->attach_interrupt(&HPBusDriver::isr_handler, this, gpio::INTERRUPT_ANY_EDGE);
    // reset the change detection to what's now on the bus

    if (this->RxTaskHandle == nullptr) {
      size_t ring_buf_size = 12 * frame_data_length * (8 + 2) * sizeof(rmt_item32_t);
      this->rb_ = xRingbufferCreate(ring_buf_size, RINGBUF_TYPE_NOSPLIT);

      ESP_LOGD(TAG, "Creating io Tasks");
      xTaskCreate(RxTask, "RX", 1024 * 4, this, 1, &this->RxTaskHandle);
      xTaskCreate(TxTask, "TX", 1024 * 4, this, 1, &this->TxTaskHandle);
    }
  }
}

void IRAM_ATTR HPBusDriver::isr_handler(HPBusDriver *instance) { instance->isr_handler(); }

void IRAM_ATTR HPBusDriver::isr_handler() {
  portBASE_TYPE HPTaskAwoken = pdFALSE;
  if (this->mode == BUSMODE_TX) {
    return;
  }

  uint64_t now = esp_timer_get_time();
  bool level = this->gpio_pin_->digital_read();

  if (this->current_pulse_.duration0 == 0 && level) {
    this->current_pulse_.level0 = !level;
    this->current_pulse_.duration0 = this->elapsed(now);
  } else if (this->current_pulse_.duration0 > 0) {
    this->current_pulse_.level1 = !level;
    this->current_pulse_.duration1 = this->elapsed(now);
    BaseType_t res =
        xRingbufferSendFromISR(this->rb_, (void *) &this->current_pulse_, sizeof(this->current_pulse_), &HPTaskAwoken);
    // reset for next pass
    memset((void *) &this->current_pulse_, 0x00, sizeof(this->current_pulse_));
  }

  if (HPTaskAwoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
  this->last_change_us_ = now;
}

bool HPBusDriver::get_next_frame(DecodingStateFrame *frame) {
  if (!has_next_frame())
    return false;
  if (!frame) {
    ESP_LOGE(TAG, "Invalid frame pointer!");
    return false;
  }
  return this->received_frames.try_dequeue(frame);
}

bool HPBusDriver::has_time_to_send() {
  bool result = false;
  static unsigned long last_send_time = 0;  // Tracks the last time a message was sent

  unsigned long current_time = millis();
  uint32_t end_of_transmit = current_time + this->transmit_count * single_frame_max_duration_ms;

  if (this->has_controller() && this->is_controller_timeout()) {
    return true;  // assume controller was disconnected
  }

  if (this->next_controller_packet().has_value()) {
    auto next = this->next_controller_packet().value();
    result = (end_of_transmit < next);
  } else {
    result = true;
  }

  // Check if at least 1 second has passed since the last message
  if (current_time - last_send_time < 1000) {
    ESP_LOGVV(TAG_BUS, "Has time to send: %s. (next: %d, needed: %d)", result ? "true" : "false",
              this->next_controller_packet().has_value() ? (this->next_controller_packet().value() / 1000) : 0,
              end_of_transmit / 1000);
    last_send_time = current_time;
  }

  return result;
}

void HPBusDriver::process_send_queue() {
  CommandFrame packet;
  if (!this->tx_packets_queue.has_next())
    return;
  if (this->current_frame.is_started()) {
    ESP_LOGV(TAG_BUS, "Packet being received. waiting");
    return;
  }
  if (!this->is_time_for_next()) {
    ESP_LOGD(TAG_BUS, "Queue has next but need to throttle. Waiting.");
    return;
  }

  if (!this->has_time_to_send()) {
    ESP_LOGW(TAG_BUS, "No time to send before next panel message. Waiting.");
    return;
  }
  ESP_LOGI(TAG_BUS, "Retrieving packet from queue");
  if (this->tx_packets_queue.try_dequeue(&packet)) {
    ESP_LOGI(TAG_BUS, "Packet received, type: %s", packet.type_string());
    this->mode = BUSMODE_TX;
    ESP_LOGD(TAG_BUS, "Resetting existing packet (if any)");
    this->current_frame.reset("TX Start");
    ESP_LOGI(TAG_BUS, "Processing send queue: %s", packet.get_format().c_str());
    auto transmitCount = this->transmit_count;
    this->gpio_pin_->pin_mode(gpio::Flags::FLAG_OUTPUT | gpio::Flags::FLAG_PULLUP);
    while (transmitCount > 0) {
      sendHeader();
      size_t transmitIndex = 0;
      while (transmitIndex < packet.size()) {
        for (int bitIndex = 0; bitIndex <= 7; bitIndex++) {
          _sendLow(bit_low_duration_ms);
          if (get_bit(packet[transmitIndex], bitIndex)) {
            _sendHigh(bit_long_high_duration_ms);
          } else {
            _sendHigh(bit_low_duration_ms);
          }
        }
        transmitIndex++;
      }

      if (--transmitCount > 0) {
        _sendLow(bit_low_duration_ms);
        _sendHigh(controler_frame_spacing_duration_ms);
      }
    }
    _sendLow(bit_low_duration_ms);
    _sendHigh(controler_group_spacing_ms);
    this->previous_sent_packet_ = millis();
    start_receive();
  }
}
void IRAM_ATTR HPBusDriver::finalize_frame(bool timeout) {
  if (this->current_frame.finalize()) {
    ESP_LOGVV(TAG_BUS, "New Frame finalized %s", timeout ? "after timeout" : "");
    if (this->current_frame.get_source() == SOURCE_CONTROLLER) {
      this->controler_packets_received_ = true;
      this->previous_controller_packet_time_ = millis();
    }

    // Check if the frame has changed compared to the previous one
    BaseFrame *previous_frame = this->get_previous(this->current_frame);
    if (this->is_changed(this->current_frame)) {
      // Retrieve the previous frame for comparison
      ESP_LOGVV(TAG, "Frame type has changed values. Printing");

      if (previous_frame) {
        // Print the differences between the current and previous frames
        ESP_LOGVV(TAG, "Previous frame found");

        if (this->current_frame.get_frame_type() != FRAME_CLOCK) {
          // don't show differences for clock packets as
          BaseFrame::print_diff("Diff", this->current_frame, *previous_frame, TAG_PACKET, ESPHOME_LOG_LEVEL_DEBUG,
                                __LINE__);
        }
        BaseFrame::print("Chg", this->current_frame, TAG_PACKET, ESPHOME_LOG_LEVEL_DEBUG, __LINE__);
      } else {
        // If no previous frame is found, print the current frame
        ESP_LOGVV(TAG, "First frame of its kind. ");
        BaseFrame::print("New", this->current_frame, TAG_PACKET, ESPHOME_LOG_LEVEL_DEBUG, __LINE__);
      }
      this->received_frames.enqueue(this->current_frame);
      this->store_frame(this->current_frame);

    } else {
      if (previous_frame && previous_frame->has_expired()) {
        ESP_LOGV(TAG, "Frame did not change: %s", previous_frame->type_string());
        if (BaseFrame::log_active(TAG, ESPHOME_LOG_LEVEL_DEBUG)) {
          ESP_LOGV(TAG, "now: %d, Age: %d, time: %d,  current age: %d time %d ", millis(),
                   previous_frame->get_frame_age_ms(), previous_frame->get_frame_time_ms(),
                   this->current_frame.get_frame_age_ms(), this->current_frame.get_frame_time_ms());
          BaseFrame::print("Ping", this->current_frame, TAG_PACKET, ESPHOME_LOG_LEVEL_DEBUG, __LINE__);
        }
        this->store_frame(this->current_frame);
        this->received_frames.enqueue(this->current_frame);
      }

      else {
        ESP_LOGVV(TAG, "No change detected. Not queuing. ");
      }
    }
    // Reset the current frame for the next sequence
    this->current_frame.reset("Resetting queued frame - ");
  } else if (timeout) {
    // Reset the frame in case of a timeout
    this->current_frame.reset("Timeout - ");
  }
}

void HPBusDriver::sendHeader() {
  if (this->gpio_pin_ == nullptr)
    return;

  // ESP_LOGV(TAG, "Sending frame header");

  _sendLow(frame_heading_low_duration_ms);
  _sendHigh(frame_heading_high_duration_ms);
}

void HPBusDriver::TxTask(void *arg) {
  HPBusDriver *instance = static_cast<HPBusDriver *>(arg);
  ESP_LOGD(TAG_BUS, "Starting TxTask for bus on GPIO%d", instance->gpio_pin_->get_pin());
  instance->tx_packets_queue.set_task_handle(xTaskGetCurrentTaskHandle());
  while (true) {
    // ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(frame_end_threshold_ms));
    instance->process_send_queue();
    delay(200);
  }
}
void HPBusDriver::RxTask(void *arg) {
  HPBusDriver *instance = static_cast<HPBusDriver *>(arg);
  size_t rx_size = 0;
  bool received_msg = true;  // force display at least once
  instance->mode = BUSMODE_RX;

  while (true) {
    rmt_item32_t *items = (rmt_item32_t *) xRingbufferReceive(instance->rb_, &rx_size, 120 * portTICK_PERIOD_MS);
    if (items && rx_size > 0) {
      instance->current_frame.passes_count++;
      size_t count = static_cast<size_t>(rx_size / sizeof(rmt_item32_t));
      // ESP_LOGVV(TAG_BUS, "Received %d bytes from the ring buffer (%d items)", rx_size, count);
      for (size_t i = 0; i < count; i++) {
        instance->process_pulse(&items[i]);
      }
      if (instance->current_frame.is_complete()) {
        instance->finalize_frame(false);
      }

      received_msg = true;
      // Return the items to the ring buffer
      vRingbufferReturnItem(instance->rb_, (void *) items);
    } else {
      if (instance->current_frame.is_started() && instance->current_pulse_.duration0 > 0 &&
          instance->elapsed(esp_timer_get_time()) > (frame_end_threshold_ms * 1000)) {
        rmt_item32_t pulse;
        memcpy((void *) &pulse, (void *) &instance->current_pulse_, sizeof(instance->current_pulse_));
        memset((void *) &instance->current_pulse_, 0, sizeof(instance->current_pulse_));
        ESP_LOGD(TAG_BUS, "Bus TIMEOUT. %s", instance->format_pulse_item(&pulse).c_str());
        received_msg = true;
        instance->process_pulse(&pulse);
        if (instance->current_frame.is_complete()) {
          instance->finalize_frame(true);
        } else {
          ESP_LOGD(TAG_BUS, "Frame not complete.");
          instance->current_frame.debug();
        }
      }
      if (received_msg) {
        ESP_LOGVV(TAG_BUS, "No item received from the ring buffer");
        // only display once
        received_msg = false;
      }
      instance->log_pulses();
    }
  }

  vTaskDelete(NULL);
}

bool HPBusDriver::is_changed(const BaseFrame &frame) {
  auto it = packet_map_.find(frame.get_frame_type());

  if (it != packet_map_.end()) {
    // If an entry exists, compare the frames
    return !(frame == it->second);
  }
  // no existing entry found, change detected
  return true; 
}

BaseFrame *HPBusDriver::get_previous(const BaseFrame &frame) {
  auto it = this->packet_map_.find(frame.get_frame_type());
  if (it != packet_map_.end()) {
    return &(it->second);  // Return a pointer to the found frame
  }
  ESP_LOGVV(TAG, "No previous frame found for frame type 0x%02X", static_cast<uint8_t>(frame.packet.frame_type));
  return nullptr;  // Return nullptr if no frame is found
}

void HPBusDriver::store_frame(const BaseFrame &frame) {
  auto ts = frame.get_frame_time_ms();
  auto it = packet_map_.find(frame.get_frame_type());
  if (it != packet_map_.end()) {
    // update the data
    ESP_LOGVV(TAG, "Updating the data.");
    it->second = std::move(frame);
  } else {
    // If no entry exists, simply insert the new frame
    ESP_LOGVV(TAG, "Creating new packet list entry.");
    packet_map_.emplace(frame.get_frame_type(), frame);
  }
  auto prev = get_previous(frame);
  if (!prev) {
    ESP_LOGE(TAG, "Frame not stored properly!");
  } else if (ts != prev->get_frame_time_ms()) {
    ESP_LOGW(TAG, "Received ts: %d, updated ts: %d", ts, prev->get_frame_time_ms());
  }
}
}  // namespace hayward_pool_heater
}  // namespace esphome
