import argparse
import asyncio
import threading
import time
from aioesphomeapi import APIClient, APIConnectionError,  LogLevel, ReconnectLogic
from TagEntry import TagEntry, TagType
from utils import Colors, TimedLogLine, colored, colored_print, load_esphome_yaml
# Shared context for storing the client and reconnect logic
class SharedContext:
    client: APIClient = None
    reconnect_logic: ReconnectLogic = None

shared_context = SharedContext()
# Global event to stop all threads
stop_event = threading.Event()

async def run_esphome_logs(yaml_config, device_ip):
    """
    Handles connecting to the ESPHome device and processing logs via aioesphomeapi.
    """
    api_key = yaml_config.get("api", {}).get("encryption", {}).get("key", "")
    client_name = "hwp_logger"
    client_info = f"{client_name} 1.0.0.1"
    client = APIClient(address=device_ip, port=6053, password="", noise_psk=api_key, client_info=client_info)
    shared_context.client = client

    colored_print(f"Connecting to ESPHome device at {device_ip}...", Colors.cyan)
    def handle_message(message):
        if message is None:
            return
        LogsTagger.add_log(TimedLogLine(message.message.decode('utf-8', errors='ignore'), time.localtime(time.time())))
    async def on_connect():
        try:
            colored_print(f"\nConnected to {device_ip}!", Colors.blue)
            await client.subscribe_logs( log_level=LogLevel.LOG_LEVEL_DEBUG,on_log= lambda message: handle_message(message), dump_config=True)
        except APIConnectionError as err:
            colored_print(f"\nError getting initial data for {device_ip}: {err}", Colors.br_red)
            await client.disconnect()

    async def on_disconnect(expected_disconnect):
        if expected_disconnect:
            colored_print(f"\nDisconnected from {device_ip}!", Colors.br_green)
        else:
            colored_print(f"\nDisconnected from {device_ip}!", Colors.br_red)

    async def on_connect_error(err: Exception):
        colored_print(f"Error connecting to {device_ip}: {err}", Colors.br_red)

    reconnect_logic = ReconnectLogic(
        client=client,
        on_connect=on_connect,
        on_disconnect=on_disconnect,
        zeroconf_instance=None,
        name=client_name,
        on_connect_error=on_connect_error,
    )
    shared_context.reconnect_logic = reconnect_logic  # Store reconnect logic in shared context

    # Start the connection logic
    await reconnect_logic.start()
    
async def input_listener(args):
    """
    Handles user input and tagging logic in an asynchronous loop.
    """
    colored_print("Starting input listener...", Colors.cyan)
    while not stop_event.is_set():
        await asyncio.sleep(0)  # Yield control back to the event loop

        # Run input in a separate thread to avoid blocking
        tag_input = await asyncio.to_thread(input, colored(f"\nPress {TagType.get_prompt()}  \n\tq: quit: ", Colors.br_green))
        tag_input = tag_input.strip().lower()

        if tag_input == 'q':
            colored_print("Exiting program...", Colors.br_green)
            stop_event.set()
            break

        tag = TagEntry.parse(tag_input)
        if  tag.is_valid():
            await tag.prompt_user()
            LogsTagger.enqueue(tag)



async def stop_all():
    """
    Stop the reconnect logic and disconnect the client.
    """
    if shared_context.reconnect_logic:
        await shared_context.reconnect_logic.stop()

    if shared_context.client:
        await shared_context.client.disconnect()
async def main_task(yaml_config, device_ip,args):
    """
    Main task for managing logs and asyncio event loop.
    """
    
    log_task: asyncio.Task = asyncio.create_task(run_esphome_logs(yaml_config, device_ip))
    
    input_task = asyncio.create_task(input_listener(args))

    # Wait for both tasks to complete
    await asyncio.gather(log_task, input_task)

    # Stop everything gracefully after the input listener finishes
    await stop_all()

# Command-line argument parsing
def main():
    """
    Main entry point for the script. Parses command-line arguments, sets up logging, and starts the main task.

    Args:
        --yaml (required): YAML configuration file for ESPHome.
        --device (optional): Device IP address for ESPHome logs.
        --log_prefix (optional): Path and prefix for the log files. Default is 'POOL'.
        --delay (optional): Reaction delay in seconds. Default is 1.5.

    Returns:
        None
    """
    parser = argparse.ArgumentParser(description="Run ESPHome logs with daily log rotation, manual tagging, and filtering.")
    parser.add_argument("--yaml", required=True, help="YAML configuration file for ESPHome.")
    parser.add_argument("--device", required=False, default=None, help="Device IP address for ESPHome logs.")
    parser.add_argument("--log_prefix", required=False, default="POOL", help="Path and prefix for the log files. Example: /path/to/logs/my_log_prefix")
    parser.add_argument("--delay", type=float, default=1.5, help="Reaction delay in seconds. Default is 1.5")

    args = parser.parse_args()
    yaml_config = load_esphome_yaml(args.yaml)

    LogsTagger.initialize(args)

    device_ip = args.device if args.device else f"{yaml_config['esphome']['name']}.local"

    try:
        # Instead of using asyncio.run(), we manually create the event loop to prevent premature exits
        loop = asyncio.get_event_loop()
        loop.run_until_complete(main_task(yaml_config, device_ip,args))
    except KeyboardInterrupt:
        colored_print("Shutting down...", Colors.br_red)
    finally:
        loop.close()

if __name__ == "__main__":
    import multiprocessing
    multiprocessing.freeze_support()
    from analysis.hwp_logs_tagger import LogsTagger
    main()
