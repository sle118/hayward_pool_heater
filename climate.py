# *
#  @file climate.py
#  @brief 
# 
#  Copyright (c) 2024 S. Leclerc (sle118@hotmail.com)
# 
#  This file is part of the Pool Heater Controller component project.
# 
#  @project Pool Heater Controller Component
#  @developer S. Leclerc (sle118@hotmail.com)
# 
#  @license MIT License
# 
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
# 
#  The above copyright notice and this permission notice shall be included in all
#  copies or substantial portions of the Software.
# 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#  SOFTWARE.
# 
#  @disclaimer Use at your own risk. The developer assumes no responsibility
#  for any damage or loss caused by the use of this software.
# 
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, sensor, text_sensor, switch
from esphome import pins
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    CONF_SENSORS,
    DEVICE_CLASS_TEMPERATURE,
    ENTITY_CATEGORY_CONFIG,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    CONF_OUTPUT,
    CONF_INPUT,
    CONF_FILTERS
)
from esphome.core import coroutine

CODEOWNERS = ["@sle118"]

AUTO_LOAD = [
    "climate",
    "select",
    "sensor",
    "binary_sensor",
    "button",
    "text_sensor",
    "time",
]
DEPENDENCIES = [
    "climate","esp32"
]
CONF_ACTIVE_MODE_SWITCH = "active_mode_switch"

CONF_BUTTONS = "buttons"
CONF_FILTER_RESET_BUTTON = "filter_reset_button"

CONF_TEMPERATURE_SOURCES = (
    "temperature_sources"  # This is for specifying additional sources
)

CONF_ENHANCED_MHK_SUPPORT = (
    "enhanced_mhk"  # EXPERIMENTAL. Will be set to default eventually.
)

CONF_GPIO_NETPIN = "pin_txrx"
CONF_MAX_BUFFER_COUNT = "max_buffer_count"
CONF_OUT_TEMPERATURE = "out_temperature"
CONF_TEMPERATURE_SUCTION = "suction_temperature_T01"
CONF_TEMPERATURE_COIL = "coil_temperature_T04"
CONF_TEMPERATURE_AMBIENT = "ambient_temperature_T05"
CONF_TEMPERATURE_EXHAUST = "exhaust_temperature_T06"
CONF_TEMPERATURE_T1 = "temperature_1"
CONF_TEMPERATURE_T2 = "temperature_2"
CONF_TEMPERATURE_T3 = "temperature_3"
CONF_TEMPERATURE_T4 = "temperature_4"


hayward_pool_heater_ns = cg.esphome_ns.namespace("hayward_pool_heater")
PoolHeater = hayward_pool_heater_ns.class_("PoolHeater", cg.Component, climate.Climate)
ActiveModeSwitch = hayward_pool_heater_ns.class_(
    "ActiveModeSwitch", switch.Switch, cg.Component
)

BASE_SCHEMA = climate.CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(PoolHeater),
        cv.Required(CONF_GPIO_NETPIN): pins.gpio_pin_schema(
            {CONF_OUTPUT: True, CONF_INPUT: True}
        ),
        cv.Optional(CONF_NAME, default="Pool Heater"): cv.Any(
            cv.All(
                cv.none,
                cv.requires_friendly_name(
                    "Name cannot be None when esphome->friendly_name is not set!"
                ),
            ),
            cv.string,
        ),
        cv.Optional(
            CONF_ACTIVE_MODE_SWITCH, default={"name": "Active Mode"}
        ): switch.switch_schema(
            ActiveModeSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="RESTORE_DEFAULT_OFF",
            icon="mdi:upload-network",
        )
    }
)


SENSORS = dict[str, tuple[str, cv.Schema, callable]](
    {
        CONF_OUT_TEMPERATURE: (
            "Out Temperature",
            sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                accuracy_decimals=1,
                icon="mdi:sun-thermometer-outline",
                
            ),
            sensor.register_sensor,
        ),
CONF_TEMPERATURE_SUCTION:
(
    "Suction Temperature",
    sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=1,
        icon="mdi:sun-thermometer-outline"
    ),
    sensor.register_sensor,
),

CONF_TEMPERATURE_COIL:
(
    "Coil Temperature",
    sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=1,
        icon="mdi:air-conditioner",
        
    ),
    sensor.register_sensor,
),

CONF_TEMPERATURE_AMBIENT:
(
    "Ambient Temperature",
    sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=1,
        icon="mdi:thermometer",
        
    ),
    sensor.register_sensor,
),

CONF_TEMPERATURE_EXHAUST:
(
    "Exhaust Temperature",
    sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=1,
        icon="mdi:smoke-detector",
        
    ),
    sensor.register_sensor,
),

CONF_TEMPERATURE_T1:
(
    "Temperature Sensor 1",
    sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=1,
        icon="mdi:smoke-detector",
        
    ),
    sensor.register_sensor,
),

CONF_TEMPERATURE_T2:
(
    "Temperature Sensor 2",
    sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=1,
        icon="mdi:smoke-detector",
        
    ),
    sensor.register_sensor,
),

CONF_TEMPERATURE_T3:
(
    "Temperature Sensor 3",
    sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=1,
        icon="mdi:smoke-detector",
        
    ),
    sensor.register_sensor,
),

CONF_TEMPERATURE_T4:
(
    "Temperature Sensor 4",
    sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=1,
        icon="mdi:smoke-detector",
        

    ),
    sensor.register_sensor,
),

        "actual_status": (
            "Actual Status",
            text_sensor.text_sensor_schema(
                icon="mdi:heat",
            ),
            text_sensor.register_text_sensor,
        ),
        "heater_status_code": (
            "Heater Status",
            text_sensor.text_sensor_schema(
                icon="mdi:alert-circle-outline",
            ),
            text_sensor.register_text_sensor,
        ),
        "heater_status_description": (
            "Status Description",
            text_sensor.text_sensor_schema(
                icon="mdi:information-outline",
                
            ),
            text_sensor.register_text_sensor,
        ),
        "heater_status_solution": (
            "Status Solution",
            text_sensor.text_sensor_schema(
                icon="mdi:toolbox-outline",
            ),
            text_sensor.register_text_sensor,
        ),
    }
)

SENSORS_SCHEMA = cv.All(
    {
        cv.Optional(
            sensor_designator,
            default={"name": f"{sensor_name}","disabled_by_default":"false"},
        ): sensor_schema
        for sensor_designator, (
            sensor_name,
            sensor_schema,
            registration_function,
        ) in SENSORS.items()
    }
)

CONFIG_SCHEMA = BASE_SCHEMA.extend(
    {
        cv.Optional(CONF_SENSORS, default={}): SENSORS_SCHEMA,
        cv.Optional(CONF_MAX_BUFFER_COUNT, default=8): cv.int_range(min=1, max=10),
    }
)


@coroutine
async def to_code(config):

    pin_component = await cg.gpio_pin_expression(config[CONF_GPIO_NETPIN])
    max_buffer_count = config[CONF_MAX_BUFFER_COUNT]
    heater_component = cg.new_Pvariable(
        config[CONF_ID], pin_component, max_buffer_count
    )

    await cg.register_component(heater_component, config)
    await climate.register_climate(heater_component, config)

    # Sensors

    for sensor_designator, (
        _,
        _,
        registration_function,
    ) in SENSORS.items():
        sensor_conf = config[CONF_SENSORS][sensor_designator]
        sensor_component = cg.new_Pvariable(sensor_conf[CONF_ID])
        await registration_function(sensor_component, sensor_conf)

        cg.add(
            getattr(heater_component, f"set_{sensor_designator}_sensor")(
                sensor_component
            )
        )

    # Debug Settings
    if am_switch_conf := config.get(CONF_ACTIVE_MODE_SWITCH):
        switch_component = await switch.new_switch(am_switch_conf)
        await cg.register_component(switch_component, am_switch_conf)
        await cg.register_parented(switch_component, heater_component)
