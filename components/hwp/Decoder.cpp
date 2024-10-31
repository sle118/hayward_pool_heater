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

#include "Decoder.h"
#include "base_frame.h"
#include "esphome/core/log.h"
namespace esphome {
namespace hwp {

Decoder::Decoder()
    : BaseFrame(), passes_count(0), current_byte_value(0), bit_current_index(0), started(false) {}

Decoder::Decoder(const Decoder& other)
    : BaseFrame(other), passes_count(0), current_byte_value(other.current_byte_value),
      bit_current_index(other.bit_current_index), started(other.started) {}

Decoder& Decoder::operator=(const Decoder& other) {
    if (this != &other) {
        BaseFrame::operator=(other);
        current_byte_value = other.current_byte_value;
        bit_current_index = other.bit_current_index;
        started = other.started;
        passes_count = other.passes_count;
    }
    return *this;
}

void Decoder::reset(const char* msg) {
    bool debug_status = log_active(TAG_DECODING);

    if (this->started) {
        if (debug_status && strlen(msg) > 0) {
            debug(msg);
        }
    }
    this->passes_count = 0;
    this->bit_current_index = 0;
    this->current_byte_value = 0;
    this->started = false;
    this->finalized = false;
    this->source_ = SOURCE_UNKNOWN;
    this->packet.reset();
}

std::shared_ptr<BaseFrame> Decoder::finalize(heat_pump_data_t& hp_data) {
    bool inverted = false;
    this->source_ = SOURCE_UNKNOWN;
    this->finalized = false;
    std::shared_ptr<BaseFrame> specialized = nullptr;
    if (!started) {
        return nullptr;
    }
    if (!this->is_size_valid()) {
        return nullptr;
    }

    if (is_checksum_valid(inverted)) {
        if (inverted) {
            inverse();
            this->source_ = SOURCE_HEATER;
        } else {
            this->source_ = SOURCE_CONTROLLER;
        }
        finalized = true;
        specialized = process(hp_data);
        specialized->set_frame_time_ms(millis());
        ESP_LOGVV(TAG_DECODING, "Finalize()->frame is %s with type %s",
            this->finalized ? "FINALIZED" : "NOT FINALIZED", specialized->type_string());
        return specialized;
    }
    return nullptr;
}

bool Decoder::is_valid() const { return (finalized && BaseFrame::is_valid()); }

void Decoder::append_bit(bool long_duration) {
    if (!started) {
        ESP_LOGW(TAG_DECODING, "Frame not started. Ignoring bit");
        return;
    }
    if (long_duration) {
        set_bit(&current_byte_value, bit_current_index);
    }
    bit_current_index++;
    if (bit_current_index == 8) {
        if (this->packet.data_len < sizeof(this->packet.data)) {
            ESP_LOGVV(
                TAG_DECODING, "New byte #%u: 0X%02X", this->packet.data_len, current_byte_value);
            this->packet.data[this->packet.data_len] = current_byte_value;
        } else {
            ESP_LOGW(TAG_DECODING, "Frame overflow %u/%u. New byte: 0X%2X", this->packet.data_len,
                sizeof(this->packet.data), current_byte_value);
        }
        bit_current_index = 0;
        current_byte_value = 0;
        this->packet.data_len++;
    }
}

void Decoder::start_new_frame() {
    reset("Start of new frame");
    started = true;
}

int32_t Decoder::get_high_duration(const rmt_item32_t* item) {
    return item->level0 ? item->duration0 : item->level1 ? item->duration1 : 0;
}

uint32_t Decoder::get_low_duration(const rmt_item32_t* item) {
    return !item->level0 ? item->duration0 : !item->level1 ? item->duration1 : 0;
}

bool Decoder::matches_duration(uint32_t target_us, uint32_t actual_us) {
    return actual_us >= (target_us - pulse_duration_threshold_us) &&
           actual_us <= (target_us + pulse_duration_threshold_us);
}

bool Decoder::is_start_frame(const rmt_item32_t* item) {
    return matches_duration(get_high_duration(item), frame_heading_high_duration_ms * 1000);
}

bool Decoder::is_long_bit(const rmt_item32_t* item) {
    return matches_duration(get_high_duration(item), bit_long_high_duration_ms * 1000) &&
           matches_duration(get_low_duration(item), bit_low_duration_ms * 1000);
}

bool Decoder::is_short_bit(const rmt_item32_t* item) {
    return matches_duration(get_high_duration(item), bit_short_high_duration_ms * 1000) &&
           matches_duration(get_low_duration(item), bit_low_duration_ms * 1000);
}
bool Decoder::is_frame_end(const rmt_item32_t* item) {
    return Decoder::get_high_duration(item) == 0 || Decoder::get_low_duration(item) == 0 ||
           Decoder::matches_duration(Decoder::get_high_duration(item), frame_end_threshold_ms*1000);
}
bool Decoder::is_started() const { return started; }

void Decoder::set_started(bool value) { started = value; }

void Decoder::debug(const char* msg) {
    std::stringstream oss;
    bool debug_status = log_active(TAG_DECODING);

    if (this->packet.data_len == 0 || strlen(msg) == 0) return;

    BaseFrame inv_bf = BaseFrame(*this);
    inv_bf.inverse();
    oss << " " << (started ? "STARTED" : "NOT STARTED") << ", "
        << (finalized ? "FINALIZED" : "NOT FINALIZED") << ", "
        << " data_len: " << std::dec << static_cast<int>(this->packet.data_len)
        << " current_byte_value: " << std::setw(2) << std::hex << current_byte_value
        << " bit_current_index: " << std::dec << static_cast<int>(bit_current_index)
        << " checksum: " << std::setw(2) << std::hex
        << static_cast<int>(this->packet.calculate_checksum()) << " inv checksum: " << std::setw(2)
        << std::hex << static_cast<int>(inv_bf.packet.calculate_checksum());
    std::string status = oss.str();
    ESP_LOGV(TAG_DECODING, "%s%s", msg, status.c_str());

    if (debug_status && (is_size_valid())) {
        debug_print_hex();
        inv_bf.debug_print_hex();
    }
}

bool Decoder::is_complete() const {
    return this->started && this->is_size_valid() && this->is_checksum_valid();
}

void Decoder::is_changed(const BaseFrame& frame) {
    // Implementation for checking if the frame has changed from the given frame.
    // This part can include comparison logic or other necessary checks depending on how the system
    // works.
}

} // namespace hwp
} // namespace esphome
