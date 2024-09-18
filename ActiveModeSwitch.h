#pragma once

#include "esphome/components/switch/switch.h"
#include "PoolHeater.h"


namespace esphome {
namespace hayward_pool_heater {

class PoolHeaterSwitch : public switch_::Switch, public Component, public Parented<PoolHeater> {
  public:
    PoolHeaterSwitch() = default;
    using Parented<PoolHeater>::Parented;
    void setup() override {
      this->state = this->get_initial_state_with_restore_mode().value_or(false);
      this->parent_->set_passive_mode(!state);};
  protected:
    virtual void write_state(bool state) override;
};

class ActiveModeSwitch : public PoolHeaterSwitch {
  protected:
    void write_state(bool new_state) {
      this->publish_state(new_state);
      this->parent_->set_passive_mode(!new_state);
    }
};

}  // namespace mitsubishi_uart
}  // namespace esphome