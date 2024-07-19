# Hayward Pool Heater ESPHome Component

This project is a continuation and enhancement of the original work by [njanik](https://github.com/njanik/hayward-pool-heater-mqtt), which focused on interfacing with Hayward pool heaters using MQTT. This new project aims to integrate the Hayward pool heater communication into the ESPHome ecosystem, enabling seamless integration with Home Assistant and providing more control and monitoring capabilities.

## Supported Devices

The following pool heaters have been tested and are known to be compatible:
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

- **ESP8266 (e.g., Wemos D1 Mini)**
- **Bi-directional Logic Level Shifter**
- **Connection Wires**

### Connections

#### For PC1000 Controller
1. Connect the `NET` pin of the PC1000 controller to the `D5` pin of the ESP8266 via a bi-directional level shifter.
2. Connect the `GND` of the PC1000 to the `GND` of the ESP8266.

#### For PC1001 Controller
1. Connect the Wemos `+5V` and `GND` to the corresponding pins on the controller using the connector `CN16`.
2. Connect `NET`, `+5V`, and `GND` to the high voltage side of the bi-directional logic level converter.
3. On the low voltage side, connect the Wemos `+3.3V`, `GND`, and `D5`.

**Note:** The 5V to 3.3V level shifter is mandatory because the ESP8266 is not 5V tolerant, and the heat pump controller does not operate with 3.3V.

## Software Setup

### ESPHome Configuration

Create a new ESPHome configuration file (e.g., `pool_heater.yaml`) with the following content:

#### Full Configuration Example

```yaml
climate:
  - platform: hayward_pool_heater
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
  - platform: hayward_pool_heater
    pin_txrx: GPIO14
```
#### Custom Component
Place the custom component files in the custom_components/hayward_pool_heater directory within your ESPHome configuration directory.

### Future Goals
This project aims to eventually be merged into the official ESPHome repository, making it easier for users to integrate and use the Hayward pool heater component.

### Credits
Special thanks to the original author njanik for the groundwork on understanding the Hayward pool heater protocol and for the original MQTT bridge project. Additional thanks to the contributors from the French Arduino community, especially plode, and to GitHub users @jruibarroso and @marcphilibert for their contributions.

### Original Reverse Engineering Topic (in French)

### License
This project is licensed under the MIT License.