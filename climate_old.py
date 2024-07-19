"""
@file climate.py
@brief Defines the configuration and code generation for the PoolHeater climate component.

@project Pool Heater Controller Component
@developer S. Leclerc (sle118@hotmail.com)

@license MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.

@disclaimer Use at your own risk. The developer assumes no responsibility
for any damage or loss caused by the use of this software.
"""

CODEOWNERS = ["@sle118"]
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome import pins
from esphome.components import climate, sensor, switch
from esphome.const import (CONF_PIN, 
                           CONF_ID, 
                           CONF_OUTPUT, 
                           CONF_INPUT, 
                           CONF_NAME, 
                           ENTITY_CATEGORY_CONFIG, 
                           ICON_BUG, 
                           UNIT_CELSIUS, 
                           DEVICE_CLASS_TEMPERATURE, 
                           STATE_CLASS_MEASUREMENT, 
                           DEVICE_CLASS_SWITCH)

CONF_MAX_BUFFER_COUNT = "max_buffer_count"
CONF_TEMPERATURE_OUT = "temperature_out"
CONF_DEBUG = "debug"
CONF_PASSIVE_MODE = "passive"

hayward_pool_heater_ns = cg.esphome_ns.namespace('hayward_pool_heater')
PoolHeater = hayward_pool_heater_ns.class_('PoolHeater', climate.Climate, cg.Component)
PassiveModeSwitch = hayward_pool_heater_ns.class_("GenericSwitch", switch.Switch)
DebugModeSwitch = hayward_pool_heater_ns.class_("DebugSwitch", switch.Switch)


DEFAULT_TEMPERATURE_OUT_NAME = "Temperature Out"
DEFAULT_PASSIVE_MODE_NAME = "Passive Mode"
DEFAULT_DEBUG_NAME = "Debug Mode"

CONFIG_SCHEMA = cv.All(
    climate.CLIMATE_SCHEMA.extend({
        cv.GenerateID(): cv.declare_id(PoolHeater),
        cv.Optional(CONF_MAX_BUFFER_COUNT, default=8): cv.int_range(min=1, max=10),
        cv.Required(CONF_PIN): pins.gpio_pin_schema({
            CONF_OUTPUT: True, CONF_INPUT: True
        }),
        cv.Optional(CONF_TEMPERATURE_OUT, default={}): cv.Schema(
            sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT
            ).extend({
                cv.Optional(CONF_NAME, default=DEFAULT_TEMPERATURE_OUT_NAME): cv.string,
            })
        ),
        cv.Optional(CONF_PASSIVE_MODE, default={}): cv.Schema(
            switch.switch_schema(
                entity_category=ENTITY_CATEGORY_CONFIG,
                icon=ICON_BUG
            ).extend({
                cv.GenerateID(): cv.declare_id(PassiveModeSwitch),
                cv.Optional(CONF_NAME, default=DEFAULT_PASSIVE_MODE_NAME): cv.string,
            })
        ),
        cv.Optional(CONF_DEBUG, default={}): cv.Schema(
            switch.switch_schema(
                entity_category=ENTITY_CATEGORY_CONFIG,
                icon=ICON_BUG
            ).extend({
                cv.GenerateID(): cv.declare_id(DebugModeSwitch),
                cv.Optional(CONF_NAME, default=DEFAULT_DEBUG_NAME): cv.string,
            })
        )
    }).extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    pin = await cg.gpio_pin_expression(config[CONF_PIN])
    cg.add(var.set_gpio_pin(pin))    
    cg.add(var.set_max_buffer_count(config[CONF_MAX_BUFFER_COUNT]))

    # Handle Passive Mode Binary Sensor
    if CONF_PASSIVE_MODE in config:
        passive_binary_sensor = await switch.new_switch(config[CONF_PASSIVE_MODE])
        cg.add(var.set_passive(passive_binary_sensor))

    # Handle Temperature Out Sensor
    if CONF_TEMPERATURE_OUT in config:
        temp_out_sensor = await sensor.new_sensor(config[CONF_TEMPERATURE_OUT])
        cg.add(var.set_temperature_out_sensor(temp_out_sensor))

    # Handle Debug Binary Sensor
    if CONF_DEBUG in config:
        debug_binary_sensor = await switch.new_switch(config[CONF_DEBUG])
        cg.add(var.set_debug(debug_binary_sensor))


