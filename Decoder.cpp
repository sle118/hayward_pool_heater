/**
 * @file DecodingStateFrame.cpp
 * @brief Implements the DecodingStateFrame class for handling the state of decoding while processing
 * pulses.
 *
 * Copyright (c) 2024 S. Leclerc (sle118@hotmail.com)
 *
 * @project Pool Heater Controller Component
 * @developer S. Leclerc (sle118@hotmail.com)
 *
 * @license MIT License
 */

#include "Decoder.h"

namespace esphome {
namespace hayward_pool_heater {

static constexpr char TAG_DECODING[] = "hayward_pool_heater.decoder";

Decoder::Decoder()
    : BaseFrame(), passes_count(0), currentByte(0), bit_current_index(0), started(false), finalized(false) {}

Decoder::Decoder(const Decoder &other)
    : BaseFrame(other),
      passes_count(0),
      currentByte(other.currentByte),
      bit_current_index(other.bit_current_index),
      started(other.started),
      finalized(other.finalized) {}

Decoder &Decoder::operator=(const Decoder &other) {
  if (this != &other) {
    BaseFrame::operator=(other);
    currentByte = other.currentByte;
    bit_current_index = other.bit_current_index;
    started = other.started;
    finalized = other.finalized;
    passes_count = other.passes_count;
  }
  return *this;
}

void Decoder::reset(const char *msg) {
#ifdef USE_LOGGER
  auto *log = logger::global_logger;
  bool debug_status = (log != nullptr && log->level_for(TAG_DECODING) >= ESPHOME_LOG_LEVEL_VERY_VERBOSE);
#else
  bool debug_status = false;
#endif

  if (this->started) {
    ESP_LOGVV(TAG_DECODING, "Resetting frame");
    if (debug_status && strlen(msg) > 0) {
      debug(msg);
    }
  }
  passes_count = 0;
  bit_current_index = 0;
  data_len_ = 0;
  currentByte = 0;
  started = false;
  finalized = false;
  source_ = SOURCE_UNKNOWN;
  memset(&packet, 0x00, sizeof(packet));
}

bool Decoder::finalize() {
  bool inverted = false;
  this->source_ = SOURCE_UNKNOWN;
  this->finalized = false;
  if (!started) {
    return false;
  }
  if (!this->is_size_valid()) {
    return false;
  }

  if (is_checksum_valid(inverted)) {
    if (inverted) {
      inverseLevels();
      this->source_ = SOURCE_HEATER;
    } else {
      this->source_ = SOURCE_CONTROLLER;
    }
    finalized = true;
    frame_time_ms_ = millis();
  }
  ESP_LOGVV(TAG_DECODING, "Finalize()->frame is %s", this->finalized ? "FINALIZED" : "NOT FINALIZED");
  return this->finalized;
}

bool Decoder::is_valid() const { return (finalized && BaseFrame::is_valid()); }

void Decoder::append_bit(bool long_duration) {
  if (!started) {
    ESP_LOGW(TAG_DECODING, "Frame not started. Ignoring bit");
    return;
  }
  if (long_duration) {
    set_bit(&currentByte, bit_current_index);
  }
  bit_current_index++;
  if (bit_current_index == 8) {
    if (this->data_len_ < sizeof(this->packet.data)) {
      ESP_LOGVV(TAG_DECODING, "New byte #%u: 0X%02X", this->data_len_, currentByte);
      this->packet.data[this->data_len_] = currentByte;
    } else {
      ESP_LOGW(TAG_DECODING, "Frame overflow %u/%u. New byte: 0X%2X", this->data_len_, sizeof(this->packet.data),
               currentByte);
    }
    bit_current_index = 0;
    currentByte = 0;
    this->data_len_++;
  }
}

void Decoder::start_new_frame() {
  reset("Start of new frame");
  started = true;
}

int32_t Decoder::get_high_duration(const rmt_item32_t *item) {
  return item->level0 ? item->duration0 : item->level1 ? item->duration1 : 0;
}

uint32_t Decoder::get_low_duration(const rmt_item32_t *item) {
  return !item->level0 ? item->duration0 : !item->level1 ? item->duration1 : 0;
}

bool Decoder::matches_duration(uint32_t target_us, uint32_t actual_us) {
  return actual_us >= (target_us - pulse_duration_threshold_us) &&
         actual_us <= (target_us + pulse_duration_threshold_us);
}

bool Decoder::is_start_frame(const rmt_item32_t *item) {
  return matches_duration(get_high_duration(item), frame_heading_high_duration_ms * 1000);
}

bool Decoder::is_long_bit(const rmt_item32_t *item) {
  return matches_duration(get_high_duration(item), bit_long_high_duration_ms * 1000) &&
         matches_duration(get_low_duration(item), bit_low_duration_ms * 1000);
}

bool Decoder::is_short_bit(const rmt_item32_t *item) {
  return matches_duration(get_high_duration(item), bit_short_high_duration_ms * 1000) &&
         matches_duration(get_low_duration(item), bit_low_duration_ms * 1000);
}

bool Decoder::is_started() const { return started; }

void Decoder::set_started(bool value) { started = value; }

void Decoder::debug(const char *msg) {
#ifdef USE_LOGGER
  auto *log = logger::global_logger;
  bool debug_status = (log != nullptr && log->level_for(TAG_DECODING) >= ESPHOME_LOG_LEVEL_DEBUG);
#else
  bool debug_status = false;
#endif

  if (this->data_len_ == 0 || strlen(msg) == 0)
    return;

  BaseFrame inv_bf = BaseFrame(*this);
  inv_bf.inverseLevels();

  ESP_LOGV(TAG_DECODING,
           "%s%s packet. data_len: %u, currentByte: %d, bit_current_index: %d, "
           "started: %s, finalized: %s, passes: %d, checksum: %d?=%d, inv checksum: %d?=%d",
           msg, this->source_string(), this->data_len_, currentByte, bit_current_index,
           started ? "STARTED" : "NOT STARTED", finalized ? "FINALIZED" : "NOT FINALIZED", passes_count,
           this->get_checksum_byte(), calculateChecksum(), inv_bf.get_checksum_byte(), inv_bf.calculateChecksum());

  if (debug_status && (is_size_valid())) {
    debug_print_hex();
    inv_bf.debug_print_hex();
  }
}

bool Decoder::is_complete() const { return this->started && this->is_size_valid() && is_checksum_valid(); }

void Decoder::is_changed(const BaseFrame &frame) {
  // Implementation for checking if the frame has changed from the given frame.
  // This part can include comparison logic or other necessary checks depending on how the system works.
}

}  // namespace hayward_pool_heater
}  // namespace esphome
