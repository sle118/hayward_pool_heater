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
#include <iomanip>
#include <iostream>
#include <sstream>

namespace esphome {
namespace hwp {

class CS : public std::stringstream {
  public:
    // Enum class to represent basic ANSI colors by their number
    enum class Color {
        reset = 0,
        black = 30,
        red = 31,
        green = 32,
        yellow = 33,
        blue = 34,
        magenta = 35,
        cyan = 36,
        white = 37,
        fg_default = 39,

        // Bright Colors
        bright_black = 90,
        bright_red = 91,
        bright_green = 92,
        bright_yellow = 93,
        bright_blue = 94,
        bright_magenta = 95,
        bright_cyan = 96,
        bright_white = 97,

        // Background Colors
        bg_black = 40,
        bg_red = 41,
        bg_green = 42,
        bg_yellow = 43,
        bg_blue = 44,
        bg_magenta = 45,
        bg_cyan = 46,
        bg_white = 47,
        bg_default = 49,

        bg_gray = 100

    };

    // Constructor: accept a specific base color and highlight color
    CS(Color base_color = Color::fg_default) : _base_color(base_color) { apply_settings(); }
    void apply_settings() {
        bc = ansi_color_code(_base_color); // Store base color ANSI sequence
        // hc = ansi_color_code(_highlight_color);         // Store highlight color ANSI sequence
        rsc = "\033[0m" + ansi_color_code(_base_color); // Reset all formats and restore base color
    }
    void set_base_color(CS::Color base_color) {
        _base_color = base_color;
        apply_settings();
    }
    void set_changed_base_color(bool changed) {
        _changed_base_color = changed;
        if (!changed) {
            _base_color = Color::fg_default;

        } else {
            _base_color = Color::bright_yellow;
        }
        apply_settings();
    }
    // Reset the stream and apply color
    void reset() {
        std::stringstream::str(""); // Clear the stream
        std::stringstream::clear(); // Reset error state
    }

    // Return the content of the stream with applied color formatting
    std::string str() const {
        std::stringstream colored_stream;
        if (_changed_base_color) {
            colored_stream << bc; // Apply base color at the start if requested
        }

        // Add the string content from the current stream
        colored_stream << std::stringstream::str();

        // Append the reset format if colorize is true
        if (_changed_base_color) {
            colored_stream << invert_rst;
        }

        return colored_stream.str();
    }

    // Function to generate an ANSI escape sequence for a given color (foreground or background)
    static std::string ansi_color_code(Color color) {
        return "\033[0;" + std::to_string(static_cast<int>(color)) + "m"; // Standard colors
    }

    // Public members for base color (bc), highlight color (hc), invert mode, and reset+restore
    // (rsc)
    std::string bc;  // Base color escape sequence
    std::string hc;  // Highlight color escape sequence
    std::string rsc; // Reset all formats and restore the base color (Reset+Set Color)

    // Static constants for ANSI text formatting
    static constexpr const char* bold = "\033[1m";
    static constexpr const char* dim = "\033[2m";
    static constexpr const char* italic = "\033[3m";
    static constexpr const char* underline = "\033[4m";
    static constexpr const char* blink = "\033[5m";
    static constexpr const char* hidden = "\033[8m";
    static constexpr const char* strikethrough = "\033[9m";
    static constexpr const char* reset_format = "\033[0m";

    // ANSI commands for terminal control as static constants
    static constexpr const char* clear_screen = "\033[2J";
    static constexpr const char* save_cursor = "\033[s";
    static constexpr const char* restore_cursor = "\033[u";
    static constexpr const char* hide_cursor = "\033[?25l";
    static constexpr const char* show_cursor = "\033[?25h";
    static constexpr const char* invert = "\033[7m"; // Store the ANSI sequence for invert mode
    static constexpr const char* invert_rst = "\033[27m";

  private:
    Color _base_color; // Preset base color for the stream instance
    bool _changed_base_color{false};
};

} // namespace hwp
} // namespace esphome
