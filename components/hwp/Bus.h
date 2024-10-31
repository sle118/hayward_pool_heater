/**
 * @file Bus.h
 * @brief Defines the Bus class for handling bus communication using bit-banging technique.
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

// #include <atomic>
#include <cstddef>
#include <cstring> // for std::memcpy
// #include <deque>
#include <memory>
#include <vector>
// #include <iomanip>
#include "Schema.h"
#include "base_frame.h"
#include "esphome/components/logger/logger.h"
#include "esphome/core/optional.h"
#include <map>
#include <sstream>

#include "Decoder.h"

#include "SpinLockQueue.h"
#include "esphome/core/gpio.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

// Uncomment the line below to enable debugging the bus pulses
#define PULSE_DEBUG

namespace esphome {
namespace hwp {
#ifdef PULSE_DEBUG
static constexpr char TAG_PULSES[] = "hwp.pulses";
#endif
static constexpr char TAG_BUS[] = "hwp";

extern const uint32_t single_frame_max_duration_ms;
extern const uint8_t default_frame_transmit_count;

/**
 * @enum bus_mode_t
 * @brief Represents the bus mode (transmit or receive).
 */
typedef enum { BUSMODE_TX, BUSMODE_RX, BUSMODE_ERROR } bus_mode_t;

/**
 * @class Bus
 * @brief Handles bus communication using the bit-banging technique.
 *
 * This class is responsible for managing bus communication by transmitting and
 * receiving data using the bit-banging method. It uses queues to manage incoming
 * and outgoing data and handles synchronization using FreeRTOS.
 */
class Bus {
  public:
    /**
     * @brief Constructs a new Bus object.
     *
     * @param pin The GPIO pin used for bus communication.
     * @param maxBufferCount The maximum buffer count for the queue.
     * @param maxWriteLength The maximum write length for the queue.
     * @param transmitCount The number of times to repeat transmission.
     */
    Bus(size_t maxWriteLength = 8, size_t transmitCount = default_frame_transmit_count);

    /**
     * @brief Sets the GPIO pin used for bus communication.
     *
     * @param gpio_pin The GPIO pin used for bus communication.
     */
    void set_gpio_pin(InternalGPIOPin* gpio_pin) { this->gpio_pin_ = gpio_pin; }
    /**
     * @brief Gets the GPIO pin used for bus communication.
     *
     * @return The GPIO pin used for bus communication.
     */
    InternalGPIOPin* get_gpio_pin() { return this->gpio_pin_; }

    /**
     * @brief Initializes the bit-banging interface.
     */
    void setup();

    /**
     * @brief Queues frame data for transmission.
     *
     * @param frame The frame data to queue.
     * @return true If the frame was successfully queued.
     * @return false If the queue is full.
     */
    bool queue_frame_data(std::shared_ptr<BaseFrame> frame);

    /**
     * @brief Checks if the time since the last controller packet exceeds the allowed delay.
     *
     * @return true If the time since the last controller packet exceeds the allowed delay.
     * @return false If the time since the last controller packet is within the allowed delay or
     * if no controller packet has been received yet.
     */
    bool is_controller_timeout() {
        return this->previous_controller_packet_time_.has_value() &&
               this->previous_controller_packet_time_.value() +
                       (delay_between_controller_messages_ms * 1.5) <
                   millis();
    }

    /**
     * @brief Checks if a controller is connected.
     *
     * @return true If a controller is connected.
     * @return false If no controller is connected.
     *
     * The presence of a controller is determined by checking if the time since the last
     * controller packet exceeds the allowed delay. If it does, we consider that the
     * controller is disconnected.
     */
    bool has_controller() {
        // we need to consider that we have a controller up until enough time has passed,
        // which could indicate that it has been disconnected
        return (this->controler_packets_received_.has_value() &&
                this->controler_packets_received_.value());
    }
    /**
     * @brief Calculates the next time when a controller packet should be sent.
     *
     * This function returns the next time when a controller packet should be sent,
     * based on the time of the last controller packet and the delay between controller
     * messages. If no controller packet has been received yet, it will return the
     * delay between controller messages. The estimate is to determine if we have time
     * to send a message on the bus without colliding.
     *
     * @return The next time when a controller packet should be sent, or an empty
     *         optional if no controller packet has been received yet and the delay
     *         between controller messages has not expired yet.
     */
    optional<uint32_t> next_controller_packet() {
        if (this->previous_controller_packet_time_.has_value()) {
            return this->previous_controller_packet_time_.value() +
                   delay_between_controller_messages_ms;
        }
        if (millis() < delay_between_controller_messages_ms) {
            return delay_between_controller_messages_ms;
        }
        return {};
    }
    /**
     * @brief Checks if the time has come to send the next packet.
     *
     * This function returns true if either no packet has been sent yet or the time since the last
     * packet exceeds the delay between sending messages. Otherwise, it returns false.
     *
     * @return true If the time has come to send the next packet.
     * @return false If the time has not come to send the next packet.
     */
    bool is_time_for_next() {
        return !this->previous_sent_packet_.has_value() ||
               this->previous_sent_packet_.value() + delay_between_sending_messages_ms <= millis();
    }
    /**
     * @brief Gets the current bus mode (transmit or receive).
     *
     * @return The current bus mode.
     */
    bus_mode_t get_bus_mode() { return this->mode; }

    /**
     * @brief Sets the heat pump data model for the bus.
     *
     * This allows the bus to access the heat pump data model and modify it
     * accordingly.
     *
     * @param[in] hp_data The heat pump data model.
     */
    void set_data_model(heat_pump_data_t& hp_data) { this->hp_data_ = &hp_data; }
    std::vector<std::shared_ptr<BaseFrame>> control(const HWPCall& call);
    void traits(climate::ClimateTraits& traits, heat_pump_data_t& hp_data);
    static void dump_known_packets(const char* CALLER_TAG);

  protected:
    
    heat_pump_data_t* hp_data_;
    optional<bool> controler_packets_received_;
    optional<uint32_t> previous_controller_packet_time_;
    optional<uint32_t> previous_sent_packet_;
    volatile bus_mode_t mode;            ///< The current mode of the bus (transmit or receive).
    InternalGPIOPin* gpio_pin_{nullptr}; ///< The GPIO pin used for bus communication.
    TaskHandle_t TxTaskHandle;           ///< Handle to the I/O task.
    TaskHandle_t RxTaskHandle;           ///< Handle to the I/O task.
    Decoder current_frame;               ///< The current frame being processed.
    size_t transmit_count;               ///< The number of times to repeat transmission.
    // uint8_t maxBufferCount;              ///< Maximum buffer count for the received frames.
    size_t maxWriteLength; ///< Maximum write length for the transmitted frames.
    SpinLockQueue<std::shared_ptr<BaseFrame>> received_frames; ///< Queue for received frames.
    SpinLockQueue<std::shared_ptr<BaseFrame>>
        tx_packets_queue; ///< Queue for frames to be transmitted.
    rmt_config_t rmt_tx_config_;
    rmt_config_t rmt_rx_config_;
    RingbufHandle_t rb_;
#ifdef PULSE_DEBUG
    std::vector<std::string> pulse_strings_; // Vector to store formatted pulse strings
#endif
    uint64_t last_change_us_;
    volatile rmt_item32_t current_pulse_;

    inline uint64_t elapsed(uint64_t now) {
        if (now >= this->last_change_us_) {
            return now - this->last_change_us_;
        } else {
            return UINT64_MAX - this->last_change_us_ + now + 1;
        }
    }

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
    static void isr_handler(Bus* instance);

    void isr_handler();
    /**
     * @brief Task function for I/O operations.
     *
     * This function represents the main task for handling I/O operations. It sets task signaling
     * handles for the pulse and transmit packet queues, waits for notifications or pulses,
     * processes send queues, and handles received pulses. It also checks for frame end signals
     * and finalizes received frames.
     *
     * @param arg A pointer to the Bus instance.
     */
    static void TxTask(void* arg);
    static void RxTask(void* arg);

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

    void process_pulse(rmt_item32_t* item);
    void finalize_frame(bool timeout);

    std::string format_pulse_item(const rmt_item32_t* item) {
        if (item == nullptr) {
            return "";
        }
        std::ostringstream oss;
        if (Decoder::is_short_bit(item) || Decoder::is_long_bit(item)) {
            oss << "b";
        } else if (Decoder::is_start_frame(item)) {
            oss << "S";
        } else if (Decoder::is_frame_end(item)) {
            oss << "E";
        } else

        {
            oss << (item->level0 ? "H" : "L") << item->duration0 << ":"
                << (item->level1 ? "H" : "L") << item->duration1 << " ";
        }
        // Store the result in the vector
        return oss.str();
    }
    void log_pulse_item(const rmt_item32_t* item) {
#ifdef PULSE_DEBUG
        if (item == nullptr) {
            return;
        }

        auto* log = logger::global_logger;
        if (log == nullptr || log->level_for(TAG_PULSES) < ESPHOME_LOG_LEVEL_VERBOSE) {
            return;
        }

        // Store the result in the vector
        pulse_strings_.push_back(format_pulse_item(item));
#endif
    }

    void log_pulses() {
#ifdef PULSE_DEBUG
        if (pulse_strings_.empty()) {
            return;
        }

        auto* log = logger::global_logger;
        if (log == nullptr || log->level_for(TAG_PULSES) < ESPHOME_LOG_LEVEL_VERBOSE) {
            return;
        }

        std::string log_chunk = "PULSES:";
        uint8_t b_count = 0;
        uint8_t B_count = 0;
        uint8_t F_count = 0;
        std::string processed_str;
        const size_t max_chunk_length = 125;

        for (const auto& str : pulse_strings_) {
            processed_str = "";
            if (str == "b") {
                b_count++;
                if (b_count == 8) { // If we've encountered 8 consecutive 'b's
                    B_count++;
                    b_count = 0;
                }
                if (B_count == 12) {
                    F_count++;
                    B_count = 0;
                }
            } else {
                processed_str.append(F_count, 'F');
                processed_str.append(B_count, 'B');
                processed_str.append(b_count, '.');
                b_count = 0;
                B_count = 0;
                F_count = 0;
                processed_str += " " + str;
            }

            if (log_chunk.length() + processed_str.length() + 1 > max_chunk_length) {
                ESP_LOGV(TAG_PULSES, "%s", log_chunk.c_str());
                log_chunk.clear();
            }

            log_chunk += processed_str;
        }
        log_chunk.append(F_count, 'F');
        log_chunk.append(B_count, 'B');
        log_chunk.append(b_count, 'b');
        log_chunk += " END.";
        ESP_LOGV(TAG_PULSES, "%s", log_chunk.c_str());
        reset_pulse_log();
#endif
    }

    void reset_pulse_log() {
#ifdef PULSE_DEBUG
        pulse_strings_.clear();
#endif
    }
    void store_frame(std::shared_ptr<BaseFrame> frame);
};

} // namespace hwp
} // namespace esphome
