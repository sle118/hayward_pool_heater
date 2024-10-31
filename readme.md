# Hayward Pool Heater ESPHome Component

This project is a continuation and enhancement of the original work by [njanik](https://github.com/njanik/hayward-pool-heater-mqtt), which focused on interfacing with Hayward pool heaters using MQTT. This new project aims to integrate the Hayward pool heater communication into the ESPHome ecosystem, enabling seamless integration with Home Assistant and providing more control and monitoring capabilities.

## CURRENT PROJECT STATE -- READ FIRST !

This code is running on my own heat pump mostly in passive mode right now because summer is over and the pool is closed. I have done several tests with the active mode and feel somewhat confident that what is currently implemented with work and what would not wasn't enabled in the code.

It was safe to use for me, but as usual with open source code, at your own risk.

## Current state of the project
A lot of spare time was spent over a long period of time, with the initial goal of reverse engineering the protocol used by the Hayward pool heater. In order to simplify the analysis, I've crafted packet logging in such way that changes are highlighted so that I could observe the data while manipulating the configuration on the device. The majority of the packet content was identified by changing parameters back and forth as they are found in the technical manual found in the documents section of this project.

Packet type names are still not finalized and seem t encompass 3 main types:

- Clock: This packet is sent by the controller every minute, most likely to support the ability of the heat pump to support timed operations, for example for the fan speed.
- Configuration: Sent on average every 5 seconds or every 11 seconds, depending on the type. Several packet types encompass the full configuration of the heat pump, such as the operating mode, temperature, the mode restriction (cooling only, heating only, no restriction), the fan mode (low, high, timed, etc.), as well as some upper/lower limits for each set point (auto, cooling, heating). 
- Condition: Sent on average every 5 seconds or every 11 seconds, depending on the type. This packet type is sent by the heat pump to notify the controller of various state values such as temperatures for coil, inlet, outlet, ambient, etc. It also contains some indicators for fan state (off, high, low), the compressor state (off, on), etc.

Several of configuration attributes are exposed as a user modifiable input in home assistant, but I have left what I believe are critical ones as read only for now since messing up with their values could damage the heat pump itself. It is still possible to modify them if you want to, by following the procedure highlighted in the technical manual.

Some sensors are implemented but they are still not properly located in individual packets and any help here would be greatly appreciated. 


## Supported Devices

The following pool heaters have been tested and are known to be compatible with the original work from [njanik]:
- **Trevium HP55TR** heat pump using a **PC1000** controller.
- **Hayward energy line pro** heat pump using a **PC1000** controller.
- **MONO 50 Basic** heat pump using a **CC203** controller.
- **Majestic** heat pump (Hayward white label) using a **PC1001** controller.
- **CPAC111** heat pump (Hayward) using a **PC1001** controller.

## Introduction

This project allows you to control and monitor your Hayward pool heater through Home Assistant using ESPHome. It supports receiving current parameters and sending commands to the heat pump, providing a comprehensive solution for pool heater automation. 


## Features

- **Receive Current Parameters:** Get real-time data from your pool heater, including temperatures and operating modes.
- **Send Commands:** Control your pool heater by sending commands such as setting the mode, adjusting temperatures, and turning the heater on or off.
- **Seamless Integration with Home Assistant:** Utilize the powerful capabilities of Home Assistant to automate and monitor your pool heater.

## Hardware Requirements

### Components

- **Recommended is any ESP32 based device supported by home assistant**
- **Bi-directional Logic Level Shifter**
- **Connection Wires**
- **Voltage regulator capable of lowering 12V to 5V**

### Connections

#### For PC1000 Controller

1. **Power Supply**: Since the PC1000 controller does not have a 5V source, use a 12V to 5V stepdown regulator to power the ESP32 and the level shifter.
   - Connect the input of the stepdown regulator to the 12V power source from the PC1000.
   - Connect the output of the stepdown regulator to the `5V` pin and `GND` of the ESP32.

2. **Bi-directional Level Shifter Connections**:
   - Connect the `5V` output from the stepdown regulator to the `HV` (high voltage) pin of the level shifter.
   - Connect the `3.3V` pin of the ESP32 to the `LV` (low voltage) pin of the level shifter.
   - Connect the `GND` from the PC1000, ESP32, and level shifter together.
   - Connect the `NET` pin of the PC1000 controller to `H1` (high voltage side) of the level shifter.
   - Connect the `D5` pin of the ESP32 to `L1` (low voltage side) of the level shifter.

3. **Ground Connection**:
   - Ensure that the `GND` of the PC1000 controller is connected to the `GND` of the ESP32.

#### For PC1001 Controller

1. **Power Supply**: The PC1001 controller provides a 5V source.
   - Connect the `+5V` and `GND` pins from the connector `CN16` on the PC1001 to the `5V` pin and `GND` of the ESP32.

2. **Bi-directional Level Shifter Connections**:
   - Connect the `+5V` from the PC1001 to the `HV` (high voltage) pin of the level shifter.
   - Connect the `3.3V` pin of the ESP32 to the `LV` (low voltage) pin of the level shifter.
   - Connect the `GND` from the PC1001, ESP32, and level shifter together.
   - Connect the `NET` pin of the PC1001 controller to `H1` (high voltage side) of the level shifter.
   - Connect the `D5` pin of the ESP32 to `L1` (low voltage side) of the level shifter.

3. **Ground Connection**:
   - Ensure that the `GND` of the PC1001 controller is connected to the `GND` of the ESP32.

By following these detailed steps, you can correctly connect the ESP32 to the Hayward pool heater controllers using a bi-directional level shifter. The level shifter ensures that the voltage levels are properly managed between the 5V signals from the controller and the 3.3V signals required by the ESP32.

**Note:** The 5V to 3.3V level shifter is mandatory because an ESP8266 is not 5V tolerant, and the heat pump controller does not operate with 3.3V.

**Note:** I have not tested this setup for the PC1000 so use at your own risk and confirm any successful attempt so I can adjust this readme.

## Software Setup

### ESPHome Configuration

Create a new ESPHome configuration file (e.g., `pool_heater.yaml`) with the following content:

#### Full Configuration Example

```yaml
climate:
  - platform: hwp
    id: pool_heater
    name: "Pool Heater"
    max_buffer_count: 8
    pin_txrx: GPIO14
    active_mode_switch:
      name: "Active mode"
      restore_mode: RESTORE_DEFAULT_OFF
```

#### Shortest Configuration Example
```yaml
climate:
  - platform: hwp
    pin_txrx: GPIO14
```
#### Custom Component
Place the custom component files in the custom_components/hwp directory within your ESPHome configuration directory.


### Future Goals
This project aims to eventually be merged into the official ESPHome repository, making it easier for users to integrate and use the Hayward pool heater component. Before it can get there, more protocol analysis will be needed, especially to understand how states are communicated back (compressor running/standby, etc). For example, these error conditions should be decoded:

| Code  | Description                                                                                                 | Solution                                                                                                                                                    |
|-------|-------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **P01** | Water inlet sensor malfunction | The sensor is open or short-circuited. | Check or replace the sensor. |
| **P02** | Water outlet sensor malfunction | The sensor is open or short-circuited. | Check or replace the sensor. |
| **P05** | Defrost sensor malfunction | The sensor is open or short-circuited. | Check or replace the sensor. |
| **P04** | Outside temperature sensor malfunction | The sensor is open or short-circuited. | Check or replace the sensor. |
| **E06** | Large temperature difference between inlet and outlet water | Insufficient water flow, too low or too high water pressure difference. | Check the water flow or system obstruction. |
| **E07** | Antifreeze protection in cooling mode | Insufficient outlet water flow. | Check the water flow or outlet water temperature sensor. |
| **E19** | Level 1 antifreeze protection | Ambient or inlet water temperature is too low. | |
| **E29** | Level 2 antifreeze protection | Ambient or inlet water temperature is even lower. | |
| **E01** | High pressure protection | Refrigerant circuit pressure is too high, water flow is too low, evaporator is blocked, or air flow is too low. | - Check the high pressure switch and refrigerant circuit pressure. <br> - Check the water or air flow. <br> - Ensure the flow controller is working properly. <br> - Check the inlet/outlet water valves. <br> - Check the bypass setting. |
| **E02** | Low pressure protection | Refrigerant circuit pressure is too low, air flow is too low, or evaporator is blocked. | - Check the low pressure switch and refrigerant circuit pressure for leaks. <br> - Clean the evaporator surface. <br> - Check the fan speed. <br> - Ensure air can circulate freely through the evaporator. |
| **E03** | Flow detector malfunction | Insufficient water flow or the detector is short-circuited or defective. | - Check the water flow. <br> - Check the filtration pump and flow detector for faults. |
| **EE8** | Communication problem | LED controller or PCB connection malfunction. | Check the cable connections. |


### Credits
Special thanks to the original author njanik for the groundwork on understanding the Hayward pool heater protocol and for the original MQTT bridge project. Additional thanks to the contributors from the French Arduino community, especially plode, and to GitHub users @jruibarroso and @marcphilibert for their contributions.

### Original Reverse Engineering Topic (in French)
Special thx to the french arduino community, and especially to plode and [].
[Whole reverse engineering topic (in french)](https://forum.arduino.cc/index.php?topic=258722.0)

### License
This project is licensed under the MIT License.
