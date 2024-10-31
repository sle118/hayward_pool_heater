#include "HPUtils.h"
#include "CS.h"

namespace esphome {
namespace hwp {
std::string format_bool_diff(bool current, bool reference) {
    CS cs;
    bool changed = (current != reference);
    cs.set_changed_base_color(changed);

    auto cs_inv = changed ? CS::invert : "";
    auto cs_inv_rst = changed ? CS::invert_rst : "";
    cs << cs_inv << (current ? "TRUE " : "FALSE") << cs_inv_rst;
    return cs.str();
}

template <typename T> bool memcmp_equal(const T& lhs, const T& rhs) {
    static_assert(std::is_trivially_copyable<T>::value,
        "Type must be trivially copyable for memcmp comparison.");
    return memcmp(&lhs, &rhs, sizeof(T)) == 0;
}
template <typename T> bool update_if_changed(esphome::optional<T>& original, const T& new_value) {
    bool changed = (!original.has_value() || !memcmp_equal(*original, new_value));
    if (changed) {
        original = new_value;
    }
    return changed;
}

} // namespace hwp
} // namespace esphome
