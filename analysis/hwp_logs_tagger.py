import re
import threading
import logging
from logging.handlers import TimedRotatingFileHandler
import os
from multiprocessing import Manager

from TagEntry import TagEntry, TagType
from utils import Colors, TimedLogLine, colored_print, get_formatted_time

# Enum for Tag Types

class LogsTagger:
    _lock = threading.Lock()  # Thread lock for synchronization
    _manager = Manager()  # Create a Manager object
    _log_queue = _manager.Queue()  # Shared log queue between processes
    _tagging_queue = _manager.Queue()  # Shared tagging queue between processes
    _buffer = _manager.list()  # Shared buffer between processes
    _logger = None  # Logger is a class attribute shared by all instances
    _device_time = None  # This can also be shared through the manager

    @classmethod
    def initialize(cls, args):
        """
        Initializes the logging setup, creating log directories, files, and handlers.
        """
        with cls._lock:
            cls._buffer.maxlen = 100  # Adjust if manually managed
            if cls._logger is None:
                cls._delay = args.delay
                cls._logfile = f"{args.log_prefix}_esphome_logs.log"
                cls._logdir = os.path.dirname(args.log_prefix)
                if cls._logdir and not os.path.exists(cls._logdir):
                    os.makedirs(cls._logdir)
                    colored_print(f"Created log directory: {cls._logdir}", Colors.br_green)
                colored_print(f"Log file: {cls._logfile}", Colors.br_green)

                # Set up logging with rotation every day
                cls._logger = logging.getLogger(__name__)
                cls._logger.setLevel(logging.DEBUG)
                handler = TimedRotatingFileHandler(
                    cls._logfile, when="midnight", interval=1, backupCount=7
                )
                handler.setFormatter(logging.Formatter("%(asctime)s - %(message)s"))
                handler.suffix = "%Y-%m-%d"
                cls._logger.addHandler(handler)

    @classmethod
    def get_logger(cls):
        """
        Retrieve the global logger instance.
        """
        if cls._logger is None:
            raise ValueError("Logger is not initialized. Call initialize() first.")
        return cls._logger

    @classmethod
    def add_log(cls, logline: TimedLogLine):
        """
        Adds a log line to the buffer and processes any pending tag request.

        :param logline: TimedLogLine instance with the log line and its device time
        :return: None
        """
        formatted_time = get_formatted_time(logline.time)
        pattern = r"(\x1b\[.*?m)*(\[)"  # Match ANSI escape sequences and the first "[" outside them
        
        def insert_timestamp(match):
            ansi_code = match.group(1) if match.group(1) else ""
            return f"{ansi_code}[{formatted_time}]["

        updated_logline = re.sub(pattern, insert_timestamp, logline.text, count=1)
        cls.log_line(updated_logline)
        cls._device_time = logline.time
        cls._buffer.append(logline)

        if not cls._tagging_queue.empty():
            tag: TagEntry = cls._tagging_queue.get_nowait()
            cls.flush(tag)
    @classmethod
    def flush_buffer(cls):
        """
        Flushes the buffer and processes any pending tag requests.
        :return: None
        """
        cls._buffer = []

    @classmethod
    def log_line(cls, log_line, color=None, level=logging.INFO):
        """
        Logs the line with the specified logging level and color.
        """
        if color:
            colored_print(log_line, color)
        else:
            print(f"{log_line}")
        cls._logger.log(level, log_line)
    @classmethod
    def enqueue(cls, tag: TagEntry):
        """
        Enqueues a tag request for processing.
        :param tag: TagEntry instance representing the tag to be processed.
        :return: None
        """
        # colored_print(f"Processing: {tag}", Colors.br_white)
        cls.flush(tag)

    @classmethod
    def flush(cls, tag: TagEntry):
        """
        Flushes the buffer for the specified tag, processes relevant log lines, and logs the output.
        :param tag: TagEntry instance
        :return: None
        """
        # colored_print(f"Flushing buffer for tag: {tag}", Colors.br_green)
        cls.log_line(f"{tag.text} -- BEGIN", Colors.br_green)

        found = False
        fullfound = []
        starttime = tag.get_start_time()
        endtime = tag.get_end_time()
        if tag.tag_type == TagType.FLUSH_BUFFER:
            cls.flush_buffer()
            colored_print("Flushed lines from buffer", Colors.br_green)
            return
        # Remove old log lines before the start time
        if tag.reset_buffer():
            while len(cls._buffer) > 0 and cls._buffer[0].time < starttime:
                old_logline = cls._buffer.pop(0)

        # Process log lines within the time range
        for logline in list(cls._buffer):
            if starttime <= logline.time <= endtime:
                match,newtext  = tag.filter(logline.text)
                if match:
                    found = True
                    cls.log_line(f"{tag.text}: {newtext}")
                    fullfound.append(logline)

        if not found:
            cls.log_line(f"{tag.text} NO DATA", Colors.br_yellow)

        if tag.reset_buffer():
            cls._buffer = [line for line in cls._buffer if line not in fullfound]
        cls.log_line(f"{tag.text} -- END", Colors.br_green)

        if not found:
            colored_print(f"No logs found between {get_formatted_time(tag.get_start_time())} and {get_formatted_time(tag.get_end_time())}", Colors.dk_white)
            for logline in fullfound:
                colored_print(f"Excluded: [{get_formatted_time(logline.time)}]{logline.text}", Colors.dk_white)

