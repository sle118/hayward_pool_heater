/**
 * @file HPBusDriver.h
 * @brief Defines the HPBusDriver class for handling bus communication using bit-banging technique.
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
#include <cstring>  // for std::memcpy
#include <deque>
#include <vector>
#include "esphome/core/gpio.h"
#include "BusPulse.h"
#include "CommandFrame.h"
#include "DecodingStateFrame.h"
#include "SpinLockQueue.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace hayward_pool_heater {

// Timing constants (in microseconds)
// #define BIT_DURATION_US 200
// #define HEADER_LOW_US (9 * BIT_DURATION_US)
// #define HEADER_HIGH_US (5 * BIT_DURATION_US)
// #define BINARY_0_LOW_US (1 * BIT_DURATION_US)
// #define BINARY_0_HIGH_US (1 * BIT_DURATION_US)
// #define BINARY_1_LOW_US (1 * BIT_DURATION_US)
// #define BINARY_1_HIGH_US (3 * BIT_DURATION_US)
// #define FRAME_SPACING_HIGH_US (100 * BIT_DURATION_US)
// #define GROUP_SPACING_HIGH_US (500 * BIT_DURATION_US)
// #define FRAME_END_THRESHOLD_US (50000)
// #define TRANSMIT_REPEAT_COUNT 8

extern const uint32_t single_frame_max_duration_ms;
extern const uint8_t default_frame_transmit_count;

#define FRAME_SIZE 12

/**
 * @enum bus_mode_t
 * @brief Represents the bus mode (transmit or receive).
 */
typedef enum { BUSMODE_TX, BUSMODE_RX } bus_mode_t;

/**
 * @class HPBusDriver
 * @brief Handles bus communication using the bit-banging technique.
 *
 * This class is responsible for managing bus communication by transmitting and
 * receiving data using the bit-banging method. It uses queues to manage incoming
 * and outgoing data and handles synchronization using FreeRTOS.
 */
class HPBusDriver {
 public:
  /**
   * @brief Constructs a new HPBusDriver object.
   *
   * @param pin The GPIO pin used for bus communication.
   * @param maxBufferCount The maximum buffer count for the queue.
   * @param maxWriteLength The maximum write length for the queue.
   * @param transmitCount The number of times to repeat transmission.
   */
  HPBusDriver(size_t maxWriteLength = 8, size_t transmitCount = default_frame_transmit_count);
  void set_gpio_pin(InternalGPIOPin* gpio_pin) { this->gpio_pin_ = gpio_pin; }
  void set_max_buffer_count(uint8_t max_buffer_count) { this->maxBufferCount= max_buffer_count; }

  uint8_t get_max_buffer_count() { return this->maxBufferCount; }
  InternalGPIOPin* get_gpio_pin() { return this->gpio_pin_; }

  /**
   * @brief Initializes the bit-banging interface.
   */
  void setup() ;

  /**
   * @brief Queues frame data for transmission.
   *
   * @param frame The frame data to queue.
   * @return true If the frame was successfully queued.
   * @return false If the queue is full.
   */
  bool queue_frame_data(CommandFrame frame);
  
  bool has_next_frame(){
    return this->received_frames.has_next();
  }
  /**
   * @brief Gets the next frame from the received queue.
   *
   * @param frame The frame to store the received data.
   * @return true If a frame was successfully retrieved.
   * @return false If the queue is empty.
   */
  bool get_next_frame(DecodingStateFrame* frame);

  bool has_controller()  {
    // we need to consider that we have a controller up until enough time has passed
    return millis() < delay_between_controller_messages_ms  ||
      (this->controler_packets_received_.has_value() && this->controler_packets_received_.value());
  
  }
 protected:
  optional<bool> controler_packets_received_;
  volatile bus_mode_t mode;  ///< The current mode of the bus (transmit or receive).
  InternalGPIOPin * gpio_pin_{nullptr};
  TaskHandle_t ioTaskHandle;           ///< Handle to the I/O task.
  DecodingStateFrame current_frame;    ///< The current frame being processed.
  uint32_t next_controller_packet_ms;  ///< Timestamp for the next controller packet.
  uint32_t last_io_change_us;          ///< Timestamp of the last I/O change.
  uint32_t last_io_high_us;            ///< Timestamp of the last high I/O signal.
  bool last_io_change_level;           ///< The level of the last I/O change.
  size_t transmit_count;               ///< The number of times to repeat transmission.
  uint8_t maxBufferCount;              ///< Maximum buffer count for the received frames.
  size_t maxWriteLength;               ///< Maximum write length for the transmitted frames.
  SpinLockQueue<DecodingStateFrame> received_frames;  ///< Queue for received frames.
  SpinLockQueue<CommandFrame> tx_packets_queue;       ///< Queue for frames to be transmitted.
  SpinLockQueue<BusPulse> pulse_queue;                ///< Queue for incoming pulses.

 private:
  void start_receive();
  /**
   * @brief Processes the send queue.
   *
   * This function processes the send queue by dequeuing packets and transmitting them
   * using the bit-banging technique. It handles sending headers, individual bits, and
   * spacing between frames and groups. It also manages the transmission count.
   */
  void process_send_queue();
  /**
   * @brief Processes a received pulse.
   *
   * This function processes a received pulse by updating the current frame's high or low
   * duration based on the pulse level. It detects start frames, long bits, and short bits,
   * and handles invalid pulse lengths.
   *
   * @param pulse The received pulse to process.
   */
  void process_pulse(const BusPulse& pulse);

  /**
   * @brief Checks if the last high duration matches the target duration.
   *
   * @param target_duration_ms The target duration in milliseconds.
   * @return true If the last high duration matches the target duration.
   * @return false Otherwise.
   */
  inline bool last_high_duration_matches(uint32_t target_duration_ms) const {
    auto target_duration_us = target_duration_ms * 1000;
    return this->last_io_high_us >= (target_duration_us - pulse_duration_threshold_us) &&
           this->last_io_high_us <= (target_duration_us + pulse_duration_threshold_us);
  }
  
  /**
   * @brief Task function for I/O operations.
   *
   * This function represents the main task for handling I/O operations. It sets task signaling
   * handles for the pulse and transmit packet queues, waits for notifications or pulses,
   * processes send queues, and handles received pulses. It also checks for frame end signals
   * and finalizes received frames.
   *
   * @param arg A pointer to the HPBusDriver instance.
   */
  static void ioTask(void* arg);

  /**
   * @brief Checks if there is enough time to send all packets before the next controller packet.
   *
   * This function determines if there is sufficient time to transmit all packets before
   * the arrival of the next controller packet. It checks the estimated end of transmission
   * time against the next controller packet time or if the controller message delay has passed.
   *
   * @return true If there is enough time to send all packets.
   * @return false Otherwise.
   */
  bool has_time_to_send();

  /**
   * @brief Interrupt Service Routine (ISR) handler for processing pulses.
   *
   * This function is called by the ISR to handle GPIO pin changes. It pushes the changed frames
   * to a queue that will be processed by the I/O task.
   *
   * @param arg A pointer to the HPBusDriver instance.
   */
  static void IRAM_ATTR handlePulse(HPBusDriver* arg);

  /**
   * @brief Internal handler for processing pulses.
   *
   * This function processes the pulses detected by the ISR. It records the timestamp and level
   * of the pulse and enqueues it for processing by the I/O task. If the mode is BUSMODE_TX, it
   * returns immediately.
   */
  void IRAM_ATTR handlePulseInternal();

  /**
   * @brief Adds the current frame to the received frames queue.
   *
   * This function enqueues the current frame into the queue of received frames.
   */
  void addReceivedFrame();

  /**
   * @brief Sends a binary 0 on the bus.
   *
   * This function sends a binary 0 signal by setting the bus low for the binary 0 low duration
   * followed by a short high duration.
   */
  void sendBinary0();

  /**
   * @brief Sends a binary 1 on the bus.
   *
   * This function sends a binary 1 signal by setting the bus low for the binary 0 low duration
   * followed by a long high duration.
   */
  void sendBinary1();

  /**
   * @brief Sends the start of frame header on the bus.
   *
   * This function sends the start of frame header by setting the bus low for the frame heading
   * low duration followed by a high duration.
   */
  void sendHeader();

  /**
   * @brief Sends the frame spacing signal on the bus.
   *
   * This function sends the frame spacing signal, which is the space between each set of bytes
   * that are sent in groups. It sets the bus low for a short duration followed by the controller
   * frame spacing high duration.
   */
  void sendFrameSpacing();

  /**
   * @brief Sends the group spacing signal on the bus.
   *
   * This function sends the group spacing signal, which is the space between groups of frames.
   * It sets the bus low for a short duration followed by the controller group spacing high
   * duration.
   */
  void sendGroupSpacing();

  /**
   * @brief Sends a high signal for a specified duration.
   *
   * @param ms The duration in milliseconds.
   */
  void _sendHigh(uint32_t ms) {
    if (this->gpio_pin_ == nullptr) return;
    this->gpio_pin_->digital_write(true);
    delayMicroseconds(ms * 1000);
  }

  /**
   * @brief Sends a low signal for a specified duration.
   *
   * @param ms The duration in milliseconds.
   */
  void _sendLow(uint32_t ms) {
    if (this->gpio_pin_ == nullptr) return;
    this->gpio_pin_->digital_write(false);
    delayMicroseconds(ms * 1000);
  }

};

}  // namespace hayward_pool_heater
}  // namespace esphome
