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
#include "base_frame.h"

namespace esphome {
namespace hwp {
const char* TAG_PACKET = "hwp.pk";
// Static member definition.
std::vector<BaseFrame::frame_registry_t> BaseFrame::registry_;

// Constructors.
BaseFrame::BaseFrame()
    : packet(), transmitBitIndex(0), finalized(false), source_(SOURCE_UNKNOWN),
      frame_time_ms_(millis()) {}

BaseFrame::BaseFrame(const BaseFrame& other)
    : packet(other.packet), transmitBitIndex(0), finalized(other.finalized), source_(other.source_),
      frame_time_ms_(other.frame_time_ms_) {}

BaseFrame::BaseFrame(BaseFrame&& other) noexcept
    : packet(std::move(other.packet)), transmitBitIndex(0), finalized(std::move(other.finalized)),
      source_(std::move(other.source_)), frame_time_ms_(std::move(other.frame_time_ms_)) {}

BaseFrame::BaseFrame(const BaseFrame* other)
    : packet(), transmitBitIndex(0), source_(SOURCE_UNKNOWN), frame_time_ms_(0) {
    if (other != nullptr) {
        packet = other->packet;
        transmitBitIndex = 0;
        source_ = other->source_;
        frame_time_ms_ = other->frame_time_ms_;
        finalized = other->finalized;
    }
}

uint8_t& BaseFrame::operator[](size_t index) { return this->packet.data[index]; }
const uint8_t& BaseFrame::operator[](size_t index) const { return this->packet.data[index]; }
BaseFrame& BaseFrame::operator=(const BaseFrame& other) {
    if (this != &other) {
        packet = other.packet;
        source_ = other.source_;
        frame_time_ms_ = other.frame_time_ms_;
        finalized = other.finalized;
    }
    return *this;
}

// Static methods.
std::vector<BaseFrame::frame_registry_t>& BaseFrame::get_registry() { return registry_; }

std::shared_ptr<BaseFrame> BaseFrame::base_create() { return std::make_shared<BaseFrame>(); }

bool BaseFrame::base_matches(BaseFrame& specialized, BaseFrame& base) {
    return *specialized.byte_signature_ == base.packet.get_type();
}

size_t BaseFrame::register_frame_class(
    FrameFactoryMethod factory, FrameMatchesMethod match_method) {
    auto& registry = BaseFrame::get_registry();
    size_t index = registry.size();
    registry.push_back({factory, match_method, factory()});
    return index;
}
optional<std::shared_ptr<BaseFrame>> BaseFrame::control(const HWPCall& call) { return nullopt; }


BaseFrame::frame_registry_t* BaseFrame::get_registry_by_id(size_t type_id) {
    auto& registry = BaseFrame::get_registry();
    if (type_id < registry.size()) {
        return &registry[type_id];
    }
    return nullptr;
}

void BaseFrame::dump_known_packets(const char* caller_tag) {
    auto& registry = BaseFrame::get_registry();
    size_t count = 0;
    for (size_t i = 0; i < registry.size(); i++) {
        if (registry[i].instance->has_previous_data()) {
            count++;
        }
    }
    if (count > 0) {
        ESP_LOGCONFIG(caller_tag, "Known packets:");
    }
    for (size_t i = 0; i < registry.size(); i++) {
        if (registry[i].instance->has_previous_data()) {
            std::string formatted_frame = registry[i].instance->format(true);
            ESP_LOGCONFIG(caller_tag, "%s%s",
                registry[i].instance->header_format("   - ", true).c_str(),
                formatted_frame.c_str());
        }
    }
}


std::shared_ptr<BaseFrame> BaseFrame::get_specialized() {
    auto& registry = BaseFrame::get_registry();
    for (size_t i = 0; i < registry.size(); i++) {
        if (registry[i].matches(*registry[i].instance.get(), *this)) {
            return registry[i].instance;
        }
    }
    auto new_type_id = register_frame_class(&BaseFrame::base_create, &BaseFrame::base_matches);
    frame_registry_t* new_registry_entry = get_registry_by_id(new_type_id);
    if (!new_registry_entry) {
        ESP_LOGE(TAG_BF, "Failed to register frame type %d", new_type_id);
        return nullptr;
    }
    new_registry_entry->instance->type_id_ = new_type_id;
    new_registry_entry->instance->byte_signature_ = this->packet.get_type();
    return new_registry_entry->instance;
}

// Other member functions.
void BaseFrame::initialize() {}

void BaseFrame::parse(heat_pump_data_t& data) {}
size_t BaseFrame::get_type_id() const { return this->type_id_; }
bool BaseFrame::is_changed() const {
    return !this->has_previous_data() || this->packet != this->prev_.value();
};

bool BaseFrame::is_short_frame() const { return this->packet.data_len == frame_data_length_short; }
bool BaseFrame::is_long_frame() const { return this->packet.data_len == frame_data_length; }
size_t BaseFrame::get_data_len() const { return this->packet.data_len; }

void BaseFrame::debug_print_hex() const {
    debug_print_hex(this->packet.data, this->packet.data_len, this->source_);
}

template <size_t N>
void BaseFrame::debug_print_hex(
    const uint8_t (&buffer)[N], const size_t length, const frame_source_t source) {
    std::string hexString;
    for (size_t i = 0; i < sizeof(buffer) && i < length; ++i) {
        char hex[10];
        snprintf(hex, sizeof(hex), "0x%02X", buffer[i]);
        hexString += hex;
        if (i < length - 1) {
            hexString += ", ";
        }
    }
    ESP_LOGV(TAG_BF, "%s(%zu): %s", source_string(source), length, hexString.c_str());
}


const char* BaseFrame::format_hex(uint8_t val) {
    static char buffer[4];
    snprintf(buffer, sizeof(buffer), "%02X", val);
    return buffer;
}

const char* BaseFrame::format_hex_diff(uint8_t val, uint8_t ref) {
    bool changed = (val != ref);
    auto cs_inv = changed ? CS::invert : "";
    auto cs_inv_rst = changed ? CS::invert_rst : "";
    static char buffer[20 + sizeof(CS::invert) + sizeof(CS::invert_rst)];
    snprintf(buffer, sizeof(buffer), "%s%02X%s", cs_inv, val, cs_inv_rst);
    return buffer;
}

bool BaseFrame::is_checksum_valid() const {
    bool inverted = false;
    return is_checksum_valid(inverted);
}
bool BaseFrame::is_checksum_valid(bool& inverted) const {
    inverted = false;
    if (this->packet.is_checksum_valid()) {
        return true;
    }
    ESP_LOGVV(TAG_BF, "Checksum is invalid: %s, %02X!=%02X",
        this->packet.explain_checksum().c_str(), this->packet.bus_checksum(),
        this->packet.calculate_checksum());
    BaseFrame inv_bf = BaseFrame(*this);
    esphome::hwp::inverse(inv_bf.packet.data, inv_bf.packet.data_len);
    if (inv_bf.packet.is_checksum_valid()) {
        inverted = true;
        return true;
    } else {
        ESP_LOGVV(TAG_BF, "Inverted checksum is invalid: %s - %02X!=%02X",
            inv_bf.packet.explain_checksum().c_str(), inv_bf.packet.bus_checksum(),
            inv_bf.packet.calculate_checksum());
    }

    return false;
}

void BaseFrame::inverse() { esphome::hwp::inverse(this->packet.data, this->packet.data_len); }
std::string BaseFrame::header_format(const std::string& prefix, bool no_diff) const {
    CS cs;

    bool changed = !no_diff && this->is_changed();
    cs.set_changed_base_color(changed);

    auto cs_inv = changed ? CS::invert : "";
    auto cs_inv_rst = changed ? CS::invert_rst : "";
    hp_packetdata_t prev = no_diff ? this->packet : this->prev_.value_or(this->packet);

    std::string normalized_prefix = prefix;
    if (normalized_prefix.length() > 5) {
        normalized_prefix = normalized_prefix.substr(0, 5);
    } else {
        normalized_prefix.append(5 - normalized_prefix.length(), ' ');
    }
    cs << normalized_prefix << " [";
    cs << format_hex_diff(this->packet.data[0], prev.data[0]);
    cs << "][";

    // Middle section of exactly 9 entries, filling with spaces if needed
    size_t middle_section_end =
        sizeof(this->packet.data) - 1; // Exclude last byte for middle section
    for (size_t i = 1; i < middle_section_end; ++i) {
        if (i < this->packet.data_len - 1) {
            cs << format_hex_diff(this->packet.data[i], prev.data[i]);
        } else {
            cs << "  "; // Add spaces if not enough data
        }

        if (i < middle_section_end - 1) {
            cs << " "; // Space between entries
        }
    }

    // Final bracket for the last byte (checksum)
    cs << "][" << format_hex(this->packet.data[this->packet.data_len - 1]) << "] ";

    // Append the type and source strings
    cs << this->type_string() << "(" << this->source_string() << ") ";
    cs << "(" << std::fixed << std::setprecision(1) << std::setw(4) << std::setfill(' ')
       << static_cast<float>(this->frame_age_ms_ / 1000) << "s) ";
    return cs.str();
}

void BaseFrame::print(
    const std::string& prefix, const BaseFrame& frame, const char* tag, int min_level, int line) {
    if (!log_active(tag, min_level)) {
        return;
    }
    CS cs;
    bool changed = frame.is_changed();
    cs.set_changed_base_color(changed);

    std::string formatted_frame = frame.format();
    esp_log_printf_(min_level, tag, line, ESPHOME_LOG_FORMAT("%s%s"),
        frame.header_format(prefix).c_str(), formatted_frame.c_str());
}
void BaseFrame::print_prev(
    const std::string& prefix, const BaseFrame& frame, const char* tag, int min_level, int line) {
    if (!log_active(tag, min_level)) {
        return;
    }
    std::string formatted_frame = frame.format_prev();
    esp_log_printf_(min_level, tag, line, ESPHOME_LOG_FORMAT("%s%s"),
        frame.header_format(prefix).c_str(), formatted_frame.c_str());
}

void BaseFrame::transfer() {}
void BaseFrame::stage(const BaseFrame& base) {
    this->packet = base.packet;
    source_ = base.source_;
    finalized = base.finalized;
}

std::string BaseFrame::format(const hp_packetdata_t& val, const hp_packetdata_t& ref) const {
    CS oss;
    bool changed = (val != ref);
    oss.set_changed_base_color(changed);

    bits_details_t data;
    bits_details_t data_ref;

    oss << "[ ";
    for (size_t i = 1; i < this->packet.data_len - 1; ++i) {
        data.raw = this->packet.data[i];
        data_ref.raw = ref.data[i];
        oss << data_ref.diff(ref, " ");
    }
    oss << "]";
    return oss.str();
}
std::string BaseFrame::format(bool no_diff) const {
    if (!this->packet.data_len) {
        return "N/A";
    }
    return this->format(
        this->packet, (no_diff ? this->packet : this->prev_.value_or(this->packet)));
}
std::string BaseFrame::format_prev() const {

    if (!this->prev_.has_value()) {
        return "N/A";
    }
    return this->format(this->prev_.value(), this->prev_.value());
}

bool BaseFrame::is_type_id(const BaseFrame& frame) const { return frame.type_id_ == type_id_; }

uint8_t BaseFrame::reverse_bits(unsigned char x) {
    x = ((x >> 1) & 0x55) | ((x << 1) & 0xaa);
    x = ((x >> 2) & 0x33) | ((x << 2) & 0xcc);
    x = ((x >> 4) & 0x0f) | ((x << 4) & 0xf0);
    return x;
}
std::size_t BaseFrame::size() const { return this->packet.data_len; }

void BaseFrame::print(const std::string& prefix, const char* tag, int min_level, int line) const {
    print(prefix, *this, tag, min_level, line);
}
void BaseFrame::print_prev(
    const std::string& prefix, const char* tag, int min_level, int line) const {
    print_prev(prefix, *this, tag, min_level, line);
}

bool BaseFrame::is_valid() const { return (this->source_ != SOURCE_UNKNOWN); }

bool BaseFrame::is_size_valid() const {
    return (this->packet.data_len == frame_data_length_short ||
            this->packet.data_len == frame_data_length);
}

frame_source_t BaseFrame::get_source() const { return this->source_; }

void BaseFrame::set_source(frame_source_t source) { this->source_ = source; }

std::string BaseFrame::to_string(const std::string& prefix) const {
    CS cs;
    std::string normalized_prefix = prefix;
    if (normalized_prefix.length() > 5) {
        normalized_prefix = normalized_prefix.substr(0, 5);
    } else {
        normalized_prefix.append(5 - normalized_prefix.length(), ' ');
    }
    cs << normalized_prefix << " [";
    for (size_t i = 0; i < sizeof(this->packet.data); ++i) {
        if (i == 0 || i == 1) {
            cs << format_hex(this->packet.data[i]);
            continue;
        }
        if (i == 2) {
            cs << "][";
        }
        if (i < this->packet.data_len) {
            cs << format_hex(this->packet.data[i]) << " ";
        } else {
            cs << "   ";
        }
        if (i == this->packet.data_len - 2) {
            cs << "][";
        }
    }
    cs << "] ";
    cs << this->type_string() << "(" << this->source_string() << "): ";
    cs << this->format();
    return cs.str();
}


std::shared_ptr<BaseFrame> BaseFrame::process(heat_pump_data_t& hp_data) {
    auto specialized = get_specialized();
    if (specialized) {
        auto prev_save = this->packet;
        specialized->frame_age_ms_ = millis() - specialized->frame_time_ms_;
        specialized->frame_time_ms_ = millis();
        specialized->stage(*this); // Transfer the data to the specialized frame
        ESP_LOGVV(TAG_BF, "Specialized frame found. Cur size: %d, %s", this->packet.data_len,
            specialized->type_string());
        ESP_LOGVV(TAG_BF, "(%d)%s", specialized->packet.data_len,
            specialized->header_format("Spec").c_str());

        if (specialized->is_changed()) {
            // Retrieve the previous frame for comparison
            if (specialized->has_previous_data()) {
                ESP_LOGVV(TAG_BF, "Frame has previous data");
                specialized->print("Chg", TAG_PACKET, ESPHOME_LOG_LEVEL_DEBUG, __LINE__);

            } else {
                // If no previous frame is found, print the current frame
                ESP_LOGVV(TAG_BF, "First frame of its kind. ");
                specialized->frame_age_ms_ = 0;
                specialized->print("New", TAG_PACKET, ESPHOME_LOG_LEVEL_DEBUG, __LINE__);
            }
        } else {
            specialized->print("Same", TAG_PACKET, ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
        }

        specialized->parse(hp_data);
        if (specialized->source_ == SOURCE_HEATER) {
            hp_data.last_heater_frame = specialized->get_frame_time_ms();
        } else if (specialized->source_ == SOURCE_CONTROLLER) {
            hp_data.last_controller_frame = specialized->get_frame_time_ms();
        }

        specialized->transfer(); // Transfer the data to the specialized frame
        specialized->prev_ = optional<hp_packetdata_t>(prev_save);
        return specialized;
    }

    ESP_LOGW(TAG_BF, "No specialized frame found: %s", this->type_string());
    return nullptr;
}

uint32_t BaseFrame::get_frame_age_ms() const { return millis() - frame_time_ms_; }

uint32_t BaseFrame::get_frame_time_ms() const { return frame_time_ms_; }

void BaseFrame::set_frame_time_ms(uint32_t frame_time) { this->frame_time_ms_ = frame_time; }

void BaseFrame::set_frame_time_ms() { this->frame_time_ms_ = millis(); }

const char* BaseFrame::type_string() const {
    static char buffer[15];
    // print in hex format
    snprintf(buffer, sizeof(buffer), "TYPE_%02X   ", this->packet.get_type());
    return buffer;
}

bool BaseFrame::has_previous_data() const {
    return this->prev_.has_value() && this->prev_->get_type() != 0;
}

const char* BaseFrame::source_string() const { return source_string(this->source_); }

const char* BaseFrame::source_string(frame_source_t source) {
    switch (source) {
    case SOURCE_CONTROLLER:
        return "CONT";
    case SOURCE_HEATER:
        return "HEAT";
    case SOURCE_LOCAL:
        return "LOC ";
    default:
        return "UNK ";
    }
}


void BaseFrame::dump_c_code(const char* caller_tag) {
    CS cs;

    auto& registry = BaseFrame::get_registry();
    size_t count = 0;
    for (size_t i = 0; i < registry.size(); i++) {
        if (registry[i].instance->packet.data_len > 0) {
            count++;
        }
    }
    cs << "#define NUM_BASE_FRAMES " << count << "\n";
    count = 1;
    for (size_t i = 0; i < registry.size(); i++) {
        if (registry[i].instance->packet.data_len == 0) {
            continue;
        }
        cs << "const uint8_t base_data_" << count << "[12] = {";
        for (size_t j = 0; j < registry[i].instance->packet.data_len; j++) {
            cs << "0x" << format_hex(registry[i].instance->packet.data[j]);
            if (j < registry[i].instance->packet.data_len - 1) {
                cs << ",";
            }
        }
        cs << "};";
        ESP_LOGI(caller_tag, "%s", cs.str().c_str());
        cs.reset();
        count++;
    }

    count = 1;
    for (size_t i = 0; i < registry.size(); i++) {
        if (registry[i].instance->packet.data_len == 0) {
            continue;
        }
        cs << "static const packet_t base_" << count << " = packet_t(false, "
           << static_cast<int>(registry[i].instance->packet.data_len) << ", std::string(\""
           << registry[i].instance->type_string() << "\"),base_data_" << count << ", std::string(\""
           << registry[i].instance->format(true) << "\" ));";
        ESP_LOGI(caller_tag, "%s", cs.str().c_str());
        cs.reset();
        count++;
    }
    cs << "static const packet_t PROGMEM base_packets[] = {";
    count = 1;
    for (size_t i = 0; i < registry.size(); i++) {
        if (registry[i].instance->packet.data_len == 0) {
            continue;
        }
        cs << "base_" << count;
        if (i < registry.size() - 1) {
            cs << ",";
        }
        count++;
    }
    cs << "};";
    ESP_LOGI(caller_tag, "%s", cs.str().c_str());
    cs.reset();
}
} // namespace hwp
} // namespace esphome
