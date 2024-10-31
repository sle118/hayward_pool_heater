import asyncio
from enum import Enum
import re
import time

from typing import Tuple

from utils import Colors, colored, decode_decimal, decode_temperature


class TagType(Enum):
    INVALID = ("", "", "")
    FLUSH_BUFFER = ("f", "flush the log buffer", "")
    EVENT = (
        "e",
        "log an observed state change",
        "Enter label observed state change then press enter",
    )
    CHANGE = (
        "c",
        "log a config change",
        "Enter label for upcoming config change, and press enter after the change was seen in the logs",
    )
    NUMBER = (
        "n",
        "search for number",
        "Enter the number to search for in previous frames",
    )
    TEMPERATURE = (
        "t",
        "search for a temperature",
        "Enter the temperature to search for in previous frames",
    )

    def __new__(cls, char, description, prompt_name):
        """
        Create a new instance of TagType with char as the value, description, and prompt_name.
        """
        obj = object.__new__(cls)
        obj._value_ = char  # Store the char as the enum value
        obj._description = description  # Store the description
        obj._prompt_name = (
            prompt_name  # Store the prompt name if the type is searchable
        )
        return obj

    @property
    def description(self):
        """
        Returns the description associated with the TagType.
        """
        return self._description

    @property
    def prompt_name(self):
        """
        Returns the name of the data to enter for searchable tag types (e.g., 'number', 'temperature').
        Returns None if the tag type is not searchable.
        """
        return self._prompt_name

    @classmethod
    def from_char(cls, char: str):
        """
        Create a TagType instance from a single character.
        Raises ValueError if the character is invalid.
        """
        char = char.lower()
        for tag in cls:
            if tag.value == char:
                return tag
        raise ValueError(f"Invalid TagType character: {char}")

    @classmethod
    def get_prompt(cls):
        """
        Returns a string with a prompt for each tag type.
        """
        commands = "\n\t".join(
            [
                f"{tag.value}: {tag.description}"
                for tag in cls
                if tag.description is not None and len(tag.description) > 0
            ]
        )
        return f"followed by [duration(s|m|h)]\n\t{commands}  "

    @classmethod
    def valid(cls, value):
        """
        Checks if a given value is a valid TagType character.
        """
        return any(tag_type.value == value for tag_type in cls)

    @classmethod
    def allowed_chars(cls):
        """
        Returns a list of all allowed characters (values) for TagType.
        """
        return [tag.value for tag in cls]

    def flush_logs(self) -> bool:
        """
        Returns True if the buffer should be flushed for the specified tag type.
        """
        return self not in [TagType.TEMPERATURE, TagType.NUMBER]

    def is_searchable(self) -> bool:
        """
        Returns True if the tag type is searchable (i.e., NUMBER or TEMPERATURE).
        """
        return self in [TagType.TEMPERATURE, TagType.NUMBER]

    def get_search_prompt(self) -> str:
        """
        Returns the search prompt name if the tag type is searchable, otherwise raises a ValueError.
        """
        return f"Enter the {self.prompt_name}: "


class TagEntry:
    """
    Represents a tagging request that has a type, delay, text, and number for search operations.
    """

    def __init__(self, tag_type: TagType, delay: float = 0, text: str = ""):
        if not isinstance(tag_type, TagType):
            raise ValueError(f"Invalid tag type: {tag_type}")
        self.tag_type: TagType = tag_type
        self.delay = delay
        self.text = text
        self.search_number: float = 0
        self._tagtime = time.localtime(time.time())  # Initialize with current time

    def get_start_time(self):
        """
        Calculate and return the start time based on the tag type and delay.
        """
        if self.tag_type in [TagType.EVENT, TagType.TEMPERATURE, TagType.NUMBER]:
            start_time = time.mktime(self._tagtime) - self.delay
            return time.localtime(start_time)
        return self._tagtime

    def get_end_time(self):
        """
        Return the end time for the tag (current device time).
        """
        return time.localtime(time.time())

    def __repr__(self):
        """
        Provide a string representation of the TagEntry.
        """
        text_format = (
            f" with text: '{self.text}'" if self.text and len(self.text) else ""
        )
        return f"TagType({self.tag_type.value}) at {time.strftime('%Y-%m-%d %H:%M:%S', self._tagtime)}{text_format}, search_number: {self.search_number}"

    def is_searchable(self):
        """
        Returns True if the tag type is searchable.
        """
        return self.tag_type.is_searchable()

    def get_search_prompt(self):
        """
        Returns the search prompt for the tag type.
        """
        return self.tag_type.get_search_prompt()

    def has_prompt(self):
        """
        Returns True if the tag type has a search prompt.
        """
        return len(self.tag_type.prompt_name) > 0

    async def prompt_user(self):
        """
        Prompts the user for input based on the tag type. Handles the input internally and updates the tag properties.
        """
        if self.has_prompt():
            answer = await asyncio.to_thread(
                input, colored(self.get_search_prompt(), Colors.br_green)
            )
            if self.is_searchable():
                self.search_number = float(answer.strip())
                self.text = f"{self.tag_type.description}:{self.search_number:.1f}"

            else:
                self.text = answer.strip()

    def is_line_type(self, log_line, tagtype="Diff"):
        """
        Filters log lines using a regular expression to match '[RX]: Diff'.
        This method can be expanded to support other log types by adding more regex patterns.
        """
        # Regex pattern for matching the 'Diff' packet
        diff_pattern = re.compile(r"\[RX\].*:.*(" + tagtype + r")\s")

        if diff_pattern.search(log_line):
            return True
        return False

    def get_source(self, log_line):
        """
        Gets the source of the log line using a regular expression.
        to match (CONT) or (HEAT).
        """
        pat = re.compile(r"\s\(((?:H|C)\w*)\):")
        match = pat.match(log_line)
        if match:
            return match.group(1)

        return ""

    def is_controller(self, log_line):
        """
        Check if a log line is of type (CONT) - Controller/Keypad
        """
        return self.get_source(log_line).startswith("CONT")

    def get_frame_type(self, log_line):
        pat = r"\]\s*([\w_]+)"
        match = re.search(pat, log_line)
        if match:
            return match.group(1)
        return ""

    def filter(self, log_line) -> Tuple[bool, str]:
        """
        Filters log lines of type 'New', 'Ping', or 'Chg' and checks if any double-digit hex value
        (excluding the first and last byte) in the bracketed data after the tag matches the provided
        search_number as either an integer or as a decoded temperature value. Additionally, modifies
        the log line to highlight the found values using inverse colors.

        :param log_line: The log line string to search through.
        :return: Tuple (found, modified_log_line) where:
                 - found: True if any matching hex number or decoded value is found.
                 - modified_log_line: The modified log line with highlighted found values.
        """
        found = False  # Track if we found any matching number
        if not self.is_searchable():
            return self.is_line_type(log_line), log_line
        # Check if the log line is of type 'New', 'Ping', or 'Chg'
        if (
            not any(
                self.is_line_type(log_line, tagtype)
                for tagtype in ["New", "Ping", "Chg"]
            )
            # EVENTS types should only capture packets originating from the HEATER itself
            or (self.tag_type in [TagType.EVENT] and self.is_controller(log_line))
            # CHANGE types should only capture packets originating from the CONTROLLER as
            # commands are issued from there
            or (self.tag_type in [TagType.CHANGE] and not self.is_controller(log_line))
            # do not consider clock frames for logging 
            or (self.get_frame_type(log_line).startswith("CLO"))
        ):
            return (
                False,
                log_line,
            )  # Return False if the log line doesn't match any target types

        # Find the bracketed hex data that follows the tag type
        # Example:  [86B100000000000000000037] from the log line
        bracketed_data_pattern = re.compile(
            r"(.*\[(?:[0-9A-F]{4})\]\[)([0-9A-F\s]*)(\].*)"
        )
        single_hex = re.compile(r"[0-9A-F]{2}")
        match = bracketed_data_pattern.search(log_line)

        if match:
            # Convert search_number to uppercase string to match the hex format for comparison

            modified_log_line = match.group(1)
            for hex_value in single_hex.findall(match.group(2)):
                is_match = False
                if (
                    self.tag_type == TagType.TEMPERATURE
                    and ( decode_temperature(int(hex_value, 16)) == self.search_number
                         or decode_temperature(int(hex_value, 16),low_range=True) == self.search_number )
                ):
                    found = True  # Indicate that a match was found
                    is_match = True

                elif (
                    self.tag_type == TagType.NUMBER
                    and ( self.search_number == int(hex_value, 16) or 
                         decode_decimal(int(hex_value, 16)) == self.search_number )
                ):
                    found = True  # Indicate that a match was found
                    is_match = True
                    # Replace the matching hex value with the highlighted version in the log line
                modified_log_line += (
                    f"{Colors.inverse}{hex_value}{Colors.inverse_rst} "
                    if is_match
                    else hex_value
                )

            modified_log_line += match.group(3)

            # Return the found state and the modified log line with highlighted matches
            return found, modified_log_line

        # If no match was found in the brackets, return the original log line
        return found, log_line

    @classmethod
    def parse(cls, tag_type_input: str):
        """
        Parses the tag input (e.g., e30s, e 1m) and extracts the tag type (e or c) and the delay.
        """
        match = re.match(
            r"([" + "".join(TagType.allowed_chars()) + r"])\s*(\d+)?\s*([smh])?",
            tag_type_input.strip().lower(),
        )
        if not match:
            return TagEntry(tag_type=TagType.INVALID)
        tag_type = match.group(1)
        number = match.group(2)
        unit = match.group(3)
        if not number:
            return TagEntry(tag_type=TagType.from_char(tag_type))
        delay_seconds = int(number)
        if unit == "m":
            delay_seconds *= 60
        elif unit == "h":
            delay_seconds *= 3600

        return TagEntry(tag_type=TagType.from_char(tag_type), delay=delay_seconds)

    def is_valid(self) -> bool:
        """
        Checks if the tag entry is valid (i.e., if the tag type is valid).
        """
        return self.tag_type is not TagType.INVALID

    def reset_buffer(self) -> bool:
        """
        Returns True if the buffer should be flushed for the specified tag type.
        """
        return self.tag_type in [TagType.FLUSH_BUFFER]
