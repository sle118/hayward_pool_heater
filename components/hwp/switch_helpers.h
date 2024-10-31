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

#include "PoolHeater.h"
#include "esphome/components/switch/switch.h"

namespace esphome {
namespace hwp {

/**
 * @class PoolHeaterSwitch
 * @brief Base class for switches that control the state of a PoolHeater.
 *
 * This class is a base class for switches that control the state of a PoolHeater.
 * It provides a common implementation for setup() and write_state() that sets the
 * initial state of the switch based on its restore mode and sets the passive mode
 * of the parent PoolHeater accordingly.
 */
class PoolHeaterSwitch : public switch_::Switch, public Component, public Parented<PoolHeater> {
  public:
    /// Default constructor.
    PoolHeaterSwitch() = default;

    /// Inherit the constructor from Parented.
    using Parented<PoolHeater>::Parented;

    /**
     * @brief Called once when the component is set up.
     *
     * Sets the initial state of the switch based on its restore mode and sets the
     * passive mode of the parent PoolHeater accordingly.
     */
    void setup() override {
        this->state = this->get_initial_state_with_restore_mode().value_or(false);
        this->write_state(state);
    };

  protected:
    /**
     * @brief Called when the switch is turned on or off.
     *
     * This method is overridden by derived classes to implement the logic for
     * turning the switch on or off.
     *
     * @param state The new state of the switch.
     */
    virtual void write_state(bool state) override;
};

/**
 * @class ActiveModeSwitch
 * @brief Exposes a switch that enables sending command packets to the heat pump when active.
 *
 * This class creates a switch that controls whether the heat pump can be controlled by the pool heater component.
 * When the switch is turned on, the component can send command packets to the heat pump to control its state.
 * When the switch is turned off, the component will not send any command packets to the heat pump, effectively
 * disabling control of the heat pump.
 */
class ActiveModeSwitch : public PoolHeaterSwitch {
  protected:
    void write_state(bool new_state) {
        this->publish_state(new_state);
        this->parent_->set_passive_mode(!new_state);
    }
};

/**
 * @class UpdateStatusSwitch
 * @brief Exposes a switch that enables updating all the heat pump sensors in the loop phase.
 *
 * This class creates a switch that controls whether the heat pump sensors are updated
 * in the loop phase. When the switch is turned on, the component will update all the
 * heat pump sensors in the loop phase. When the switch is turned off, the component
 * will not update any heat pump sensors in the loop phase, effectively disabling
 * updating all the heat pump sensors.
 */
class UpdateStatusSwitch : public PoolHeaterSwitch {
  protected:
    /**
     * @brief Updates the state of the switch and the parent component.
     *
     * Publishes the new state of the switch and updates the parent component's
     * active update status based on the provided state.
     *
     * @param state The new state to set for the switch and the parent component.
     */
    void write_state(bool state) {
        this->publish_state(state);
        this->parent_->set_update_active(state);
    };
};

} // namespace hwp
} // namespace esphome