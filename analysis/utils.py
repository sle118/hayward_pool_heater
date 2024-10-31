import re
import time
from esphome.yaml_util import load_yaml

class Colors:
    """
    ANSI color codes for terminal output with added common colors, 
    brightness levels (br for bright, dk for dark), and small-caps naming.
    """
    reset = "\033[0m"  # Resets the color to default

    # Basic Colors
    red = "\033[91m"
    green = "\033[92m"
    yellow = "\033[93m"
    blue = "\033[94m"
    magenta = "\033[95m"
    cyan = "\033[96m"
    gray = "\033[90m"
    white = "\033[97m"
    orange = "\033[38;5;214m"  # ANSI code for orange (256-color mode)

    # Bright Colors
    br_red = "\033[1;91m"
    br_green = "\033[1;92m"
    br_yellow = "\033[1;93m"
    br_blue = "\033[1;94m"
    br_magenta = "\033[1;95m"
    br_cyan = "\033[1;96m"
    br_gray = "\033[1;90m"
    br_white = "\033[1;97m"
    br_orange = "\033[1;38;5;214m"

    # Dark (dimmed) Colors
    dk_red = "\033[2;91m"
    dk_green = "\033[2;92m"
    dk_yellow = "\033[2;93m"
    dk_blue = "\033[2;94m"
    dk_magenta = "\033[2;95m"
    dk_cyan = "\033[2;96m"
    dk_gray = "\033[2;90m"
    dk_white = "\033[2;97m"
    dk_orange = "\033[2;38;5;214m"

    # Background Colors
    bg_red = "\033[41m"
    bg_green = "\033[42m"
    bg_yellow = "\033[43m"
    bg_blue = "\033[44m"
    bg_magenta = "\033[45m"
    bg_cyan = "\033[46m"
    bg_gray = "\033[100m"
    bg_white = "\033[47m"
    bg_orange = "\033[48;5;214m"  # Orange background in 256-color mode
    inverse="\033[7m"
    inverse_rst="\033[27m"
    @staticmethod
    def get_inverse_color(bright_color):
        """
        Returns an inverted color (background color with black foreground) for a given bright color.
        """
        inverse_mapping = {
            Colors.br_red: "\033[41m\033[30m",      # Red background, black text
            Colors.br_green: "\033[42m\033[30m",    # Green background, black text
            Colors.br_yellow: "\033[43m\033[30m",   # Yellow background, black text
            Colors.br_blue: "\033[44m\033[30m",     # Blue background, black text
            Colors.br_magenta: "\033[45m\033[30m",  # Magenta background, black text
            Colors.br_cyan: "\033[46m\033[30m",     # Cyan background, black text
            Colors.br_gray: "\033[100m\033[30m",    # Gray background, black text
            Colors.br_white: "\033[47m\033[30m",    # White background, black text
            Colors.br_orange: "\033[48;5;214m\033[30m"  # Orange background, black text
        }
        
        return inverse_mapping.get(bright_color, bright_color)  # Return the inverse or the same bright color

    @staticmethod
    def get_highlight_color(base_color):
        """
        Returns the corresponding bright color for a given ANSI color code.
        If the base color is already bright, return the inverse (background with black text).
        """
        color_mapping = {
            Colors.dk_red: Colors.br_red,
            Colors.dk_green: Colors.br_green,
            Colors.dk_yellow: Colors.br_yellow,
            Colors.dk_blue: Colors.br_blue,
            Colors.dk_magenta: Colors.br_magenta,
            Colors.dk_cyan: Colors.br_cyan,
            Colors.dk_gray: Colors.br_gray,
            Colors.dk_white: Colors.br_white,
            Colors.dk_orange: Colors.br_orange,

            # Default bright colors for basic ANSI codes
            Colors.red: Colors.br_red,
            Colors.green: Colors.br_green,
            Colors.yellow: Colors.br_yellow,
            Colors.blue: Colors.br_blue,
            Colors.magenta: Colors.br_magenta,
            Colors.cyan: Colors.br_cyan,
            Colors.gray: Colors.br_gray,
            Colors.white: Colors.br_white,
            Colors.orange: Colors.br_orange,
        }
        
        # If it's a dark color, return the bright version
        if base_color in color_mapping:
            return color_mapping[base_color]
        
        # If already bright, return the inverse
        if base_color in [Colors.br_red, Colors.br_green, Colors.br_yellow, Colors.br_blue, Colors.br_magenta,
                          Colors.br_cyan, Colors.br_gray, Colors.br_white, Colors.br_orange]:
            return Colors.get_inverse_color(base_color)

        # Fallback to base color if no match
        return base_color


    @staticmethod
    def highlight(text, base_color):
        """
        Highlights a given text with the specified highlight color.
        :param text: The text to highlight.
        :param base_color: The default base color.
        :return: The color-coded text.
        """
        highlight_color = Colors.get_highlight_color(base_color)
        return f"{highlight_color}{text}{base_color}"




def load_esphome_yaml(yaml_file):
    """
    Loads and applies substitutions in the loaded YAML configuration.
    """
    substitution_pattern = re.compile(r'\$\{([^\}]+)\}')
    config = load_yaml(yaml_file, clear_secrets=False)
    substitutions = config.get('substitutions', {})

    def substitute_value(value):
        if isinstance(value, str):
            return substitution_pattern.sub(lambda match: substitutions.get(match.group(1), match.group(0)), value)
        elif isinstance(value, dict):
            return {k: substitute_value(v) for k, v in value.items()}
        elif isinstance(value, list):
            return [substitute_value(v) for v in value]
        else:
            return value
    return substitute_value(config)
def colored_print(text, color:Colors=Colors.dk_white):
    """
    Prints text in the specified color.
    :param text: Text to print.
    :param color: Color to use (standard names like "RED", "BLUE", etc.).
    """
    print(f"{color}{text}{Colors.reset}")
def colored(text, color:Colors=Colors.dk_white):
    """
    Wraps a given text in the specified color escape sequences.
    :param text: Text to colorize.
    :param color: Color to use (standard names like "RED", "BLUE", etc.).
    :return: A string with the escape sequences added.
    """
    return f"{color}{text}{Colors.reset}"
def get_formatted_time(local_time):
    """
    Converts a local time (as returned by time.localtime()) to a string in the format
    '%Y-%m-%d %H:%M:%S'.
    :param local_time: The local time to convert.
    :return: A string representing the local time in the specified format.
    """
    return time.strftime('%Y-%m-%d %H:%M:%S', local_time)


def decode_temperature(hex_value,low_range:bool=False):
    # Extract bits from the hex_value
    decimal = (hex_value ) & 0x1  #  (decimal part)
    integer_bits = (hex_value >> 1) & 0x1F  #  (integer part)
    offset_bit = (hex_value >> 6) & 0x1  #  (offset bit)
    sign_bit = (hex_value >>7)& 0x1  #  (sign bit)
    # get bit representation of the value left padded to 8 bits
    # as follow bit7, bit6, bit5-bit1, bit0

    # Apply the offset theory
    temperature = integer_bits 
    if offset_bit:
        temperature += 2
    if low_range:
        temperature -=30

    # Handle negative temperatures (sign bit)
    if sign_bit:
        temperature = -temperature

    if decimal:
        temperature += 0.5

    return temperature
def decode_decimal(hex_value):
    """
    Decode a decimal value from a given hex value.

    The hex value is expected to be a byte where the 0th bit is the decimal part,
    bits 1-6 are the integer part and bit 7 is the sign bit.

    The function returns the decoded value as a float, which may be negative
    if the sign bit is set.

    Parameters
    ----------
    hex_value : int
        The hex value to decode.

    Returns
    -------
    float
        The decoded value.
    """
    decimal = (hex_value ) & 0x1  #  (decimal part)
    integer_bits = (hex_value >> 1) & 0b111111  #  (integer part)
    sign_bit = (hex_value >>7)& 0b1  #  (sign bit)
    value = integer_bits 
    if sign_bit:
        value = -value

    if decimal:
        value += 0.5

    return value
class TimedLogLine:
    def __init__(self, log_line, device_time):
        """
        Initializes a TimedLogLine instance with a given log line and a device time.

        :param log_line: The log line to associate with the device time.
        :param device_time: The local time at which the log line was generated (as returned by time.localtime()).
        """
        self.text = log_line
        self.time = device_time