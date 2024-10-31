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
#pragma once

#include "CS.h"
#include "HPUtils.h"
#include "Schema.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/logger/logger.h"
#include "esphome/core/log.h"
#include "hwp_call.h"
#include <bitset>
#include <cstring>
#include <driver/rmt.h>
#include <iomanip>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

namespace esphome {
namespace hwp {

#define CLASS_DEFAULT_IMPL(DerivedFrameClass, type_name)                                           \
    static size_t class_type_id;                                                                   \
    DerivedFrameClass() : BaseFrame(), data_() {}                                                  \
    DerivedFrameClass(const BaseFrame& base) : BaseFrame(base), data_() {                          \
        *this->data_ = base.packet;                                                                \
    }                                                                                              \
    DerivedFrameClass(const DerivedFrameClass& other) : BaseFrame(other), data_(other.data_) {     \
        this->prev_data_ = other.data_;                                                            \
    }                                                                                              \
    template <size_t N>                                                                            \
    DerivedFrameClass(const unsigned char(&cmdTrame)[N]) : BaseFrame(cmdTrame) {                   \
        data_ = this->packet;                                                                      \
    }                                                                                              \
    static std::shared_ptr<BaseFrame> create();                                                    \
    static bool matches(BaseFrame& specialized, BaseFrame& base);                                  \
    bool matches(BaseFrame& base) { return matches(*this, base); }                                 \
    optional<type_name> data_;                                                                     \
    optional<type_name> prev_data_;                                                                \
    type_name& data() { return *data_; }                                                           \
    size_t get_type_id() const override { return type_id_; }                                        \
    void stage(const BaseFrame& base) override {                                                   \
        BaseFrame::stage(base);                                                                    \
        type_name data = base.packet;                                                              \
        this->data_ = optional<type_name>(data);                                                   \
    }                                                                                              \
    optional<std::shared_ptr<BaseFrame>> control(const HWPCall& call) override;                    \
    void transfer() override { this->prev_data_ = *this->data_; }                                  \
    std::string format(const type_name& val, const type_name& ref) const;                          \
    std::string format(bool no_diff = false) const override;                                       \
    std::string format_prev() const override;                                                      \
    const char* type_string() const override;                                                      \
    bool is_changed() const override {                                                             \
        return !prev_data_.has_value() || !data_.has_value() || *this->data_ != this->prev_data_;  \
    }                                                                                              \
    bool has_previous_data() const override {                                                      \
        return data_.has_value() && prev_data_.has_value();                                        \
    }                                                                                              \
    void finalize() { this->packet.from_type(*this->data_); }                                      \
    void parse(heat_pump_data_t& hp_data) override;

// Define the macro to accept a fully qualified class name.
#define CLASS_ID_DECLARATION(FullClassName)                                                        \
    size_t FullClassName::class_type_id =                                                          \
        BaseFrame::register_frame_class(&FullClassName::create, &FullClassName::matches);

#define REGISTER_FRAME_ID_DEFAULT(DerivedFrameClass)

// Enum declarations.
typedef enum { SOURCE_UNKNOWN, SOURCE_HEATER, SOURCE_CONTROLLER, SOURCE_LOCAL } frame_source_t;
static constexpr uint16_t pulse_duration_threshold_us = 600;
static constexpr uint32_t frame_heading_low_duration_ms = 9;
static constexpr uint32_t frame_heading_high_duration_ms = 5;
static constexpr uint32_t bit_long_high_duration_ms = 3;
static constexpr uint32_t bit_low_duration_ms = 1;
static constexpr uint32_t bit_short_high_duration_ms = bit_low_duration_ms;
static constexpr uint32_t frame_end_threshold_ms = 50; // spacing between each frame is 100ms.
static constexpr uint32_t controler_group_spacing_ms = 250;
static constexpr uint32_t controler_frame_spacing_duration_ms = 100;
static constexpr uint32_t delay_between_sending_messages_ms =
    10 * 1000; // restrict changes to once per 10 seconds
static constexpr uint32_t delay_between_controller_messages_ms = 60 * 1000;
static constexpr const char TAG_BF[] = "hwp";
static constexpr const char TAG_BF_HEX[] = "hwp.hex";

static constexpr uint32_t frame_heading_total_duration_ms =
    frame_heading_low_duration_ms + frame_heading_high_duration_ms;

// Class declarations.
/**
 * @brief Represents a base frame with common methods and members.
 *
 * The BaseFrame class is the base class for all frame classes. It provides common
 * methods and members for all the bus frames, which are exchanged between the heat pump bus and
 * the controller.
 */
class BaseFrame {
  public:
    /*
     * Frame registry definitions.
     */

    using FrameFactoryMethod = std::shared_ptr<BaseFrame> (*)();
    using FrameMatchesMethod = bool (*)(BaseFrame& specialized, BaseFrame& base);
    typedef struct {
        /*
         * @brief Create specialized frame instance
         *
         * The factory method, which is used to create an instance of the specialized frame class.
         * Each frame class is responsible for calling register_frame_class to register itself. This
         * is made easy with the use of macro CLASS_ID_DECLARATION in the cpp file of the spcialized
         * frame class.
         */
        FrameFactoryMethod factory;
        /*
         * @brief Check if bus frame matches specialized frame
         *
         * The frame matches method, which is used to determine if a bus frame matches
         * specialized frame.
         */
        FrameMatchesMethod matches;
        /*
         * @brief Specialized frame instance
         *
         * This is used to store the instance of the specialized frame class.
         */
        std::shared_ptr<BaseFrame> instance;
    } frame_registry_t;

    /**
     * @brief Get the registry of all frame classes.
     *
     * The registry is a vector of frame_registry_t structures. Each frame_registry_t
     * structure contains a factory method, a matches method, and an instance of the
     * specialized frame class.
     *
     * Note: that the for now the registry is a static variable, which means that if there
     * are multiple buses connected to the heat pump, they will all share the same registry.
     * If you require monitoring multiple buses, this will need to be changed.
     *
     * @return A reference to the registry vector.
     */
    static std::vector<frame_registry_t>& get_registry();

    /**
     * @brief Factory method for creating the base frame instance.
     *
     * This method is the is the factory for the base frame class. It is used to create
     * an instance of the base frame class when no specialized frame class is available.
     *
     * @return A shared pointer to the base frame instance.
     */
    static std::shared_ptr<BaseFrame> base_create();

    /**
     * @brief Matches method for specialized frame.
     *
     * This method is used by the frame_registry_t::matches method to determine if a
     * bus frame matches the specialized frame.
     *
     * @param specialized The specialized frame to check against.
     * @param base The base frame to check against.
     * @return true if the bus frame matches the specialized frame, false otherwise.
     */
    static bool base_matches(BaseFrame& specialized, BaseFrame& base);

    /**
     * @brief Register a specialized frame class.
     *
     * This method is used to register a specialized frame class with the registry.
     * Each frame class is responsible for calling this method to register itself.
     *
     * @param factory The factory method for creating a specialized frame instance.
     * @param match_method The matches method for the specialized frame.
     * @return The type id of the registered frame class.
     */
    static size_t register_frame_class(FrameFactoryMethod factory, FrameMatchesMethod match_method);

    /**
     * @brief Default constructor.
     *
     * Initializes a new instance of the BaseFrame class.
     */
    BaseFrame();

    /**
     * @brief Copy constructor.
     *
     * Creates a copy of an existing BaseFrame instance.
     *
     * @param other The BaseFrame instance to copy.
     */
    BaseFrame(const BaseFrame& other);

    /**
     * @brief Move constructor.
     *
     * Transfers ownership of resources from another BaseFrame instance.
     *
     * @param other The BaseFrame instance to move from.
     */
    BaseFrame(BaseFrame&& other) noexcept;

    /**
     * @brief Pointer-based constructor.
     *
     * Initializes a new BaseFrame instance from a pointer to another BaseFrame.
     *
     * @param other A pointer to the BaseFrame instance to copy from.
     */
    BaseFrame(const BaseFrame* other);

    /**
     * @brief Array-based constructor.
     *
     * Initializes a new BaseFrame instance from a raw byte array.
     *
     * @tparam N The size of the byte array.
     * @param base_data The raw byte array to initialize from.
     */
    template <size_t N> BaseFrame(const unsigned char (&base_data)[N]);

    /**
     * @brief Destructor.
     *
     * Cleans up resources used by the BaseFrame instance.
     */
    virtual ~BaseFrame() = default;

    /**
     * @brief Copy assignment operator.
     *
     * This copies the contents of the other frame into this frame.
     *
     * @param other The other frame to copy from.
     * @return A reference to this frame.
     */
    BaseFrame& operator=(const BaseFrame& other);

    /**
     * @brief Cast operator into a specialized frame.
     *
     * This can be used to safely cast a BaseFrame to a specialized frame.
     *
     * @tparam T The specialized frame class to cast to.
     * @return A pointer to the casted specialized frame.
     */
    template <typename T> std::shared_ptr<T> operator()();

    /**
     * @brief Copy assignment operator from a raw byte array.
     *
     * This copies the contents of the raw byte array into this frame.
     *
     * @param base_data The raw byte array to copy from.
     * @return A reference to this frame.
     */
    template <size_t N> BaseFrame& operator=(const unsigned char (&base_data)[N]);

    /**
     * @brief Access operator for mutable access to the packet data.
     *
     * This allows direct access to the packet data of this frame.
     *
     * @param index The index of the packet data to access.
     * @return A reference to the accessed packet data.
     */
    uint8_t& operator[](size_t index);

    /**
     * @brief Access operator for const access to the packet data.
     *
     * This allows direct access to the packet data of this frame.
     *
     * @param index The index of the packet data to access.
     * @return A const reference to the accessed packet data.
     */
    const uint8_t& operator[](size_t index) const;

    /**
     * @brief Initializes the BaseFrame.
     *
     * This method is called once when the BaseFrame is created. It is used to
     * initialize the BaseFrame and set default values.
     */
    virtual void initialize();

    /**
     * @brief Parses the heat pump data from the raw packet data.
     *
     * This method is called when the raw packet data is received from the heat pump.
     * It is used to parse the heat pump data and store it in the heat pump data
     * struct.
     *
     * @param data A reference to the heat pump data struct.
     */
    virtual void parse(heat_pump_data_t& data);

    /**
     * @brief Checks if the frame has previous data.
     *
     * This method checks if the frame has previous data by checking if the
     * previous data is set.
     *
     * @return true if the frame has previous data, false otherwise.
     */
    virtual bool has_previous_data() const;

    /**
     * @brief Formats the frame data as a string.
     *
     * This method formats the frame data as a string and returns it. The string
     * is formatted according to the type of frame data. Specialized frames
     * should override this method. If no specialized method is available, the
     * hex values of the frame data will be formatted. Declaration is made by default
     * with the use of macros CLASS_DEFAULT_IMPL and CLASS_DEFAULT_IMPL.
     *
     * @param no_diff If true, the difference between the current and previous
     *        frame data wil be highlighted.
     * @return A string containing the formatted frame data.
     */
    virtual std::string format(bool no_diff = false) const;

    /**
     * @brief Formats the previous frame data as a string.
     *
     * This method formats the previous frame data as a string and returns it.
     * The string is formatted according to the type of frame data. The specialized
     * frames should override this method. Declaration is made by default with the use of
     * macros CLASS_DEFAULT_IMPL and CLASS_DEFAULT_IMPL.
     *
     * @return A string containing the formatted previous frame data.
     */
    virtual std::string format_prev() const;

    /**
     * @brief Returns the type ID of the frame.
     *
     * This method returns the type ID of the frame, which is a unique identifier
     * for the frame. Each specialized frame should override this method, which is done
     * automatically using one of the macros CLASS_DEFAULT_IMPL and CLASS_DEFAULT_IMPL.
     *
     * @return The type ID of the frame.
     */
    virtual size_t get_type_id() const;

    /**
     * @brief Checks if the frame has changed.
     *
     * This method checks if the frame data has changed since the last time it was
     * written. Each specialized frame should override this method, which is done
     * automatically using one of the macros CLASS_DEFAULT_IMPL and
     * CLASS_DEFAULT_IMPL.
     *
     * @return true if the frame data has changed, false otherwise.
     */
    virtual bool is_changed() const;

    /**
     * @brief Returns the type string of the frame.
     *
     * This method returns the type string of the frame, which is a human-readable
     * string that describes the type of the frame. Each specialized frame should override
     * this method. Declaration is done automatically using one of the macros
     * CLASS_DEFAULT_IMPL and CLASS_DEFAULT_IMPL but code has to be manually written.
     *
     * @return The type string of the frame.
     */
    virtual const char* type_string() const;

    /**
     * @brief Controls the frame based on the given call.
     *
     * This method is called when and action is performed in home assistant. The frame
     * registry will loop through all specialized frames and call control() on each one.
     * The specialized frames should override this method and based on the condent of HWPCall
     * decide if a new specialized frame should be instantiated and written on the bus.
     *
     * Existing examples can be found in implementation of the FrameRunMode class.
     *
     * @param call The call to process.
     * @return An optional pointer to a new frame that should be written.
     */
    virtual optional<std::shared_ptr<BaseFrame>> control(const HWPCall& call);

    /**
     * @brief Sets the traits of the climate device.
     *
     * This method sets the traits of the climate device based on the given heat
     * pump data. For example, the heat pump could have restrictions on which modes
     * are allowed (cooling only, heating only, no restrictions).
     *
     * @param traits The climate traits to set.
     * @param hp_data The heat pump data to set the traits from.
     */
    virtual void traits(climate::ClimateTraits& traits, heat_pump_data_t& hp_data) {}

    /**
     * @brief Returns true if the frame is a long frame.
     *
     * A long frame is a frame that has a length greater than 0.
     *
     * @return True if the frame is a long frame, false otherwise.
     */
    bool is_long_frame() const;

    /**
     * @brief Returns the length of the frame data.
     *
     * @return The length of the frame data.
     */
    size_t get_data_len() const;

    /**
     * @brief Returns true if the frame is a short frame.
     *
     * A short frame is a frame that has a length of 0.
     *
     * @return True if the frame is a short frame, false otherwise.
     */
    bool is_short_frame() const;

    /**
     * @brief Returns true if the given frame is the same as the current frame.
     *
     * This method compares the type and data of the two frames and returns
     * true if they are the same, false otherwise.
     *
     * @param frame The frame to compare with.
     * @return True if the frames are the same, false otherwise.
     */
    bool is_type_id(const BaseFrame& frame) const;

    /**
     * @brief Sets the frame time to the current time.
     */
    void set_frame_time_ms();

    /**
     * @brief Sets the frame time to the given value.
     *
     * @param frame_time The frame time to set, in milliseconds.
     */
    void set_frame_time_ms(uint32_t frame_time);

    /**
     * @brief Checks if the frame checksum is valid.
     *
     * @return True if the frame checksum is valid, false otherwise.
     */
    bool is_checksum_valid() const;

    /**
     * @brief Checks if the frame checksum is valid and sets the inverted flag.
     *
     * This method checks if the frame checksum is valid and sets the inverted
     * flag to true if the frame is inverted.
     *
     * @param inverted The flag to set if the frame is inverted, which means the source is the
     * heater
     * @return True if the frame checksum is valid, false otherwise.
     */
    bool is_checksum_valid(bool& inverted) const;

    /**
     * @brief Returns the age of the frame in milliseconds.
     *
     * This method returns the age of the frame in milliseconds, which is the
     * time elapsed since the frame was last received.
     *
     * @return The age of the frame in milliseconds.
     */
    uint32_t get_frame_age_ms() const;

    /**
     * @brief Returns the frame time in milliseconds.
     *
     * @return The frame time in milliseconds.
     */
    uint32_t get_frame_time_ms() const;

    /**
     * @brief Returns the frame data.
     *
     * @return The frame data.
     */
    uint8_t* data();

    /**
     * @brief Format the given frame data for logging.
     *
     * Format the given frame data as a string for logging. If the frame data
     * differs from the reference frame data, the string will be formatted with a
     * yellow color.
     *
     * @param val The frame data to format.
     * @param ref The reference frame data to compare with.
     * @return A string representation of the frame data.
     */
    std::string format(const hp_packetdata_t& val, const hp_packetdata_t& ref) const;

    /**
     * @brief Formats a single byte value as a two-digit uppercase hexadecimal string.
     *
     * A static buffer is used to return the formatted string, so it is not thread-safe.
     *
     * @param val The byte value to format.
     * @return A pointer to the formatted string.
     */
    static const char* format_hex(uint8_t val);

    /**
     * @brief Formats a single byte value as a hexadecimal string,
     * with inverted colors if it differs from the given reference value.
     *
     * @param val The value to format.
     * @param ref The reference value to compare against.
     * @return A string representation of the value in hexadecimal format,
     * with inverted colors if the value changed.
     */
    static const char* format_hex_diff(uint8_t val, uint8_t ref);

    /**
     * @brief Prints the given frame in a uniform format.
     *
     * The first parameter is a string that is prepended to the output to denote if the frame is
     * new, changed or same. The frame data is then printed in a uniform format, which makes it easy
     * to analyze packets from a heat pump.
     *
     * The typical format is:
     * `prefix` [ frame id ][ frame data ][ checksum ] NAME (source) (age) specialized frame
     * representation
     *
     * @param prefix The string to prepend to the output.
     * @param frame The frame to print.
     * @param tag The tag of the logger to use.
     * @param min_level The minimum log level to print at.
     * @param line The line number to print.
     */
    static void print(const std::string& prefix, const BaseFrame& frame, const char* tag,
        int min_level, int line);

    /**
     * @brief Prints the frame in a uniform format.
     *
     * This method is a convenience wrapper around the static print() method.
     *
     * @param prefix The string to prepend to the output.
     * @param tag The tag of the logger to use.
     * @param min_level The minimum log level to print at.
     * @param line The line number to print.
     * @see BaseFrame::print
     */

    void print(const std::string& prefix, const char* tag, int min_level, int line) const;

    /**
     * @brief Prints a diff of the given frame in a uniform format.
     *
     * The first parameter is a string that is prepended to the output to denote if the frame is
     * new, changed or same. The frame data is then printed in a uniform format, which makes it easy
     * to analyze packets from a heat pump. The method also highlights the differences between the
     * frame and the previous frame.
     *
     * @param prefix The string to prepend to the output.
     * @param frame The frame to print.
     * @param tag The tag of the logger to use.
     * @param min_level The minimum log level to print at.
     * @param line The line number to print.
     */
    static void print_diff(const std::string& prefix, const BaseFrame& frame, const char* tag,
        int min_level, int line);

    /**
     * @brief Prints the previous frame in a uniform format.
     *
     * This method is a convenience wrapper around the static print_diff() method.
     *
     * @param prefix The string to prepend to the output.
     * @param frame The frame to print.
     * @param tag The tag of the logger to use.
     * @param min_level The minimum log level to print at.
     * @param line The line number to print.
     * @see BaseFrame::print_diff
     */
    static void print_prev(const std::string& prefix, const BaseFrame& frame, const char* tag,
        int min_level, int line);

    /**
     * @brief Prints the previous frame in a uniform format.
     *
     * This method is a convenience wrapper around the static print_prev() method.
     *
     * @param prefix The string to prepend to the output.
     * @param tag The tag of the logger to use.
     * @param min_level The minimum log level to print at.
     * @param line The line number to print.
     */
    void print_prev(const std::string& prefix, const char* tag, int min_level, int line) const;

    /**
     * @brief Prints a diff of the frame in a uniform format.
     *
     * This method is a convenience wrapper around the static print_diff() method.
     *
     * @param prefix The string to prepend to the output.
     * @param tag The tag of the logger to use.
     * @param min_level The minimum log level to print at.
     * @param line The line number to print.
     */
    void print_diff(const std::string& prefix, const char* tag, int min_level, int line) const;

    /**
     * @brief Formats a uniform header for the frame.
     *
     * The header is formatted as follows:
     * `prefix` [ frame id ][ frame data ][ checksum ] NAME (source) (age)
     *
     * @param prefix The string to prepend to the output.
     * @param no_diff If true, the differences will not be highlighted.
     * @return std::string
     */
    std::string header_format(const std::string& prefix, bool no_diff = false) const;

    /**
     * @brief Returns a short string representation of the frame source.
     *
     * @param source
     * @return const char*  with values "CONT", "HEAT", "LOC ", "UNK "
     * @see frame_source_t
     */
    static const char* source_string(frame_source_t source);

    /**
     * @brief Gets the source of the frame.
     *
     * This function returns the current source of the frame.
     *
     * @return The frame source.
     */
    frame_source_t get_source() const;

    /**
     * @brief Sets the source of the frame.
     *
     * This function sets the source of the frame to the specified value.
     *
     * @param source The new source to set for the frame.
     * @see frame_source_t
     */
    void set_source(frame_source_t source);

    /**
     * @brief Gets the source string of the frame.
     *
     * This function returns a string representation of the frame's source.
     *
     * @return A const char pointer to the source string.
     */
    const char* source_string() const;

    /**
     * @brief Checks if logging is active for the specified tag and minimum level.
     *
     * This function determines whether the logger is set up and if the log level
     * for the given tag meets or exceeds the specified minimum level.
     *
     * @param tag The tag associated with the log messages.
     * @param min_level The minimum log level required for logging to be active.
     *                  Defaults to ESPHOME_LOG_LEVEL_VERBOSE.
     * @return True if logging is active for the specified tag and level, false otherwise.
     */
    static inline bool log_active(const char* tag, int min_level = ESPHOME_LOG_LEVEL_VERBOSE) {
        auto* log = logger::global_logger;
        if (log == nullptr || log->level_for(tag) < min_level) {
            return false;
        }
        return true;
    }

    /**
     * @brief Dumps all known packets to the log.
     *
     * Dumps all known packets to the log, prefixed with the given caller tag.
     * The packets are formatted with their header and data.
     *
     * @param caller_tag The tag to use for the log output.
     */
    static void dump_known_packets(const char* CALLER_TAG);

    /**
     * @brief Debug print a buffer of bytes in hex format.
     * @param buffer The buffer to print.
     * @param length The length of the data in the buffer.
     * @param source The source of the frame.
     */
    template <size_t N>
    static void debug_print_hex(
        const uint8_t (&buffer)[N], const size_t length, const frame_source_t source);
    void debug_print_hex() const;

    /**
     * @brief Returns the packet data length
     *
     * @return size_t  The packet data length
     */
    size_t size() const;

    /**
     * @brief Check if the frame data length is valid.
     *
     * This function checks if the frame data length is either a short frame or a
     * long frame. If it is not, it returns false.
     *
     * @return true if the frame data length is valid, false otherwise.
     */
    bool is_size_valid() const;

    /**
     * @brief Check if the frame is valid.
     *
     * This function verifies whether the source of the frame
     * is known or unknown. If the source is not unknown, it
     * returns true, indicating the frame is valid.
     *
     * @return true if the frame source is known, false otherwise.
     */
    bool is_valid() const;

    /**
     * @brief Inverts the byte array, setting 1 to 0 and 0 to 1.
     *
     * When a packet is received from the bus and checksum is incorrect, it could be that the source
     * is the heat pump, which sends its data using inverted durations. Reversing the array will
     * make the checksum correct if packets are valid and the source is the heat pump, or invalid if
     * checksum fails in both cases.
     *
     */
    void inverse();

    /**
     * @brief Format the given frame data for logging.
     *
     * This is typically used when invalid frames are detected on the bus.
     *
     * @param prefix The prefix to use for the log message.
     * @return std::string  The formatted frame data.
     */
    std::string to_string(const std::string& prefix) const;

    /**
     * @brief Get the instance of a specialized frame class.
     *
     * This function will return the instance of a frame class if it has been registered
     * and if the frame type is valid.
     *
     * @return std::shared_ptr<T>  The instance of the frame class if it has been registered,
     *          nullptr otherwise.
     */
    template <typename T> static std::shared_ptr<T> get() {
        auto& registry = get_registry();
        if (T::class_type_id < registry.size()) {
            return std::static_pointer_cast<T>(registry[T::class_type_id].instance);
        }
        return nullptr;
    }

    hp_packetdata_t packet;  ///< Static packet data structure.
    size_t transmitBitIndex; ///< Index of the bit to be transmitted.

    bool finalized; ///< Whether the frame has been finalized.

    /**
     * @brief Reverses the bits of a byte.
     *
     * This function takes a byte as input and reverses all of its bits.
     *
     * This function is inspired from:
     * https://stackoverflow.com/questions/2602823/in-c-c-whats-the-simplest-way-to-reverse-the-bits-of-a-byte
     *
     * @param[in] x The byte to reverse.
     * @return The reversed byte.
     */
    static uint8_t reverse_bits(unsigned char x);

    /**
     * @brief Dump all known frames to the logger in a format that can be used to initialize
     *        the frames in the mock Arduino code.
     *
     * @param caller_tag The tag of the caller, used for logging
     */
    static void dump_c_code(const char* caller_tag);

  protected:
    frame_source_t source_;  ///< The source of the frame. @see frame_source_t
    uint32_t frame_time_ms_; ///< The time at which the frame was received in milliseconds.
    uint32_t frame_age_ms_;  ///< The age of the frame in milliseconds.
    size_t type_id_ = 0;     ///< The type id of the frame. Each time a frame type is added to the
                             ///< registry, the value is assigned its index from the registry

    /**
     * @brief The byte signature of the base frame when instantiated. This is mostly used for
     * specialized frames which are not implemented yet.
     */
    optional<uint8_t> byte_signature_ = 0;

    /**
     * @brief The previous frame data (if any).
     */
    optional<hp_packetdata_t> prev_;

    /**
     * @brief The frame registry of all specialized frames.
     * Each specialized frame class is responsible for registering itself. This
     * is simplified with macro `CLASS_ID_DECLARATION`.
     *
     */
    static std::vector<frame_registry_t> registry_; ///< The frame registry.

    /**
     * @brief This function is used to transfer the frame data to the previous frame data
     *
     * It is automatically implemented with the use of macro CLASS_ID_DECLARATION and automatically
     * called by the decoder.
     *
     */
    virtual void transfer();

    /**
     * @brief This function is used to stage the raw frame data to the specialized frame structures.
     *
     * It is automatically implemented with the use of macro CLASS_ID_DECLARATION and automatically
     * called by the decoder.
     *
     */
    virtual void stage(const BaseFrame& base);

    /**
     * @brief Returns a specialized version of the frame.
     *
     * This method goes through the registry and checks if there is a specialized
     * version of the frame. If there is, it returns it. Otherwise, it creates a new
     * entry in the registry with the same type id and byte signature as the current
     * frame and returns that.
     *
     * @return A shared pointer to a specialized version of the frame or nullptr
     * if it failed to register the frame type.
     */
    std::shared_ptr<BaseFrame> get_specialized();

    /**
     * @brief Processes a frame and if a specialized frame exists, it is used to further process the
     * frame
     *
     * This function takes a heat_pump_data_t object as input and if a specialized frame exists, it
     * is used to further process the frame, transferring known data to the canonical frame if
     * needed.
     *
     * @param hp_data The heat_pump_data_t object that stores the heat pump's state
     *
     * @return A shared pointer to the specialized frame if it exists, otherwise nullptr
     */
    std::shared_ptr<BaseFrame> process(heat_pump_data_t& hp_data);

    /**
     * @brief Get a specialized frame from its type id
     *
     * @param type_id  ///< The type id of the frame
     * @return BaseFrame::frame_registry_t*  A pointer to the specialized frame registry entry
     */
    frame_registry_t* get_registry_by_id(size_t type_id);
};

} // namespace hwp
} // namespace esphome
