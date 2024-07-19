#include <string>
#include <vector>
namespace esphome {
namespace hayward_pool_heater {
typedef enum { ERROR_SOURCE_HARDWARE, ERROR_SOURCE_OPERATIONAL } ErrorSources;
typedef enum {
    ERROR_STATUS_S00,
    ERROR_STATUS_P01,
    ERROR_STATUS_P02,
    ERROR_STATUS_P05,
    ERROR_STATUS_P04,
    ERROR_STATUS_E06,
    ERROR_STATUS_E07,
    ERROR_STATUS_E19,
    ERROR_STATUS_E29,
    ERROR_STATUS_E01,
    ERROR_STATUS_E02,
    ERROR_STATUS_E03,
    ERROR_STATUS_EE8
} ErrorStatuses;
class ErrorEntry {
  public:
    ErrorEntry(uint8_t value, std::string code, ErrorSources source, ErrorStatuses status,
        std::string description, std::string solution)
        : source_(source), status_(status), code_(code), description_(description),
          solution_(solution) {}
    bool operator==(const uint8_t value) const { return this->value_ == value; }
    // To-String Method
    std::string to_string() const {
        return "Code: " + this->code_ + "\nDescription: " + this->description_ +
               "\nSolution: " + this->solution_ + "\nSource: " +
               (this->source_ == ERROR_SOURCE_HARDWARE ? "Hardware Issue" : "Operational Problem");
    }
    const std::string& get_code() const { return this->code_; }
    const std::string& get_description() const { return this->description_; }
    const std::string& get_solution() const { return this->solution_; }
    ErrorSources get_source() const { return this->source_; }
    ErrorStatuses get_status() const { return this->status_; }

  protected:
    uint8_t value_;
    std::string code_;
    ErrorSources source_;
    ErrorStatuses status_;
    std::string description_;
    std::string solution_;
};
class ErrorCode {
  public:
    // Constructor
    ErrorCode(uint8_t value) { findErrorEntry(value); }
    void findErrorEntry(uint8_t value) {
        bool found = false;
        uint8_t pass_data[2] = {value, 0};
        for (int pass = 0; pass < sizeof(pass_data); pass++) {
            for (const auto& entry : error_codes_) {
                if (entry == pass_data[pass]) {
                    this->error_entry_ = entry;
                    return;
                }
            }
        }
        this->error_entry_ = this->error_codes_[0];
    }

    const std::string& get_code() const { return this->error_entry_.get_code(); }
    const std::string& get_description() const { return this->error_entry_.get_description(); }
    const std::string& get_solution() const { return this->error_entry_.get_solution(); }
    ErrorSources get_source() const { return this->error_entry_.get_source(); }
    ErrorStatuses get_status() const { return this->error_entry_.get_status(); }
    std::string to_string() const { return this->error_entry_.to_string(); }

  protected:
    ErrorEntry error_entry_;

    const std::vector<ErrorEntry> error_codes_ = {
        {0, "S00", ERROR_SOURCE_OPERATIONAL, ERROR_STATUS_S00, "Operational", ""},
        {1, "P01", ERROR_SOURCE_HARDWARE, ERROR_STATUS_P01, "Water inlet sensor malfunction",
            "Check or replace the sensor."},
        {2, "P02", ERROR_SOURCE_HARDWARE, ERROR_STATUS_P02, "Water outlet sensor malfunction",
            "Check or replace the sensor."},
        {5, "P05", ERROR_SOURCE_HARDWARE, ERROR_STATUS_P05, "Defrost sensor malfunction",
            "Check or replace the sensor."},
        {4, "P04", ERROR_SOURCE_HARDWARE, ERROR_STATUS_P04,
            "Outside temperature sensor malfunction", "Check or replace the sensor."},
        {6, "E06", ERROR_SOURCE_OPERATIONAL, ERROR_STATUS_E06,
            "Large temperature difference between inlet and outlet water",
            "Check the water flow or system obstruction."},
        {7, "E07", ERROR_SOURCE_OPERATIONAL, ERROR_STATUS_E07,
            "Antifreeze protection in cooling mode",
            "Check the water flow or outlet water temperature sensor."},
        {19, "E19", ERROR_SOURCE_OPERATIONAL, ERROR_STATUS_E19, "Level 1 antifreeze protection",
            "Ambient or inlet water temperature is too low."},
        {29, "E29", ERROR_SOURCE_OPERATIONAL, ERROR_STATUS_E29, "Level 2 antifreeze protection",
            "Ambient or inlet water temperature is even lower."},
        {1, "E01", ERROR_SOURCE_OPERATIONAL, ERROR_STATUS_E01, "High pressure protection",
            "Check the high pressure switch and refrigerant circuit pressure.\nCheck the water or "
            "air flow.\nEnsure the flow controller is working properly.\nCheck the inlet/outlet "
            "water valves.\nCheck the bypass setting."},
        {2, "E02", ERROR_SOURCE_OPERATIONAL, ERROR_STATUS_E02, "Low pressure protection",
            "Check the low pressure switch and refrigerant circuit pressure for leaks.\nClean the "
            "evaporator surface.\nCheck the fan speed.\nEnsure air can circulate freely through "
            "the evaporator."},
        {3, "E03", ERROR_SOURCE_OPERATIONAL, ERROR_STATUS_E03, "Flow detector malfunction",
            "Check the water flow.\nCheck the filtration pump and flow detector for faults."},
        {8, "EE8", ERROR_SOURCE_OPERATIONAL, ERROR_STATUS_EE8, "Communication problem",
            "Check the cable connections."}};
};
} // namespace hayward_pool_heater
} // namespace esphome