
#include "Schema.h"
namespace esphome {
namespace hwp {
const HeatPumpRestrict HeatPumpRestrict::Cooling(0);
const HeatPumpRestrict HeatPumpRestrict::Any(1);
const HeatPumpRestrict HeatPumpRestrict::Heating(2);
const char* HeatPumpRestrict::HeatPumpStrCooling = "Cooling Only";
const char* HeatPumpRestrict::HeatPumpStrAny = "Any Mode";
const char* HeatPumpRestrict::HeatPumpStrHeating = "Heating Only";

// Define the static constants
const DefrostEcoMode DefrostEcoMode::Eco(0);
const DefrostEcoMode DefrostEcoMode::Normal(1);
const char* DefrostEcoMode::DefrostEcoStrEco = "Eco";
const char* DefrostEcoMode::DefrostEcoStrNormal = "Normal";

// Define the static constants
const FlowMeterEnable FlowMeterEnable::Enabled(0);
const FlowMeterEnable FlowMeterEnable::Disabled(1);
const char* FlowMeterEnable::FlowMeterStrEnabled = "Enabled";
const char* FlowMeterEnable::FlowMeterStrDisabled = "Disabled";

const char * FanMode::ambient_desc = "Ambient";
const char * FanMode::ambient_scheduled_desc = "Scheduled Ambient";
const char * FanMode::scheduled_desc = "Scheduled";
const FanMode unknown = FanMode::Value::UNKNOWN;
const FanMode low_speed = FanMode::Value::LOW_SPEED;
const FanMode high_speed = FanMode::Value::HIGH_SPEED;
const FanMode ambient = FanMode::Value::AMBIENT;
const FanMode scheduled = FanMode::Value::SCHEDULED;
const FanMode ambient_scheduled = FanMode::Value::AMBIENT_SCHEDULED;

} // namespace hwp
} // namespace esphome
