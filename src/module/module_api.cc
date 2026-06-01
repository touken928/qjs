#include <qjs/module.h>

#include "../binding/js_conv.h"
#include "../binding/module_binding.h"

namespace qjs {

Module& Module::module(const std::string& name) {
    auto& child = children_[name];
    if (!child) {
        child = std::make_unique<Module>(name, this);
    }
    return *child;
}

Module& Module::value(const std::string& name, int v) {
    values_[name] = [v](Engine& e) { return valueFromNative(e, v); };
    return *this;
}

Module& Module::value(const std::string& name, int64_t v) {
    values_[name] = [v](Engine& e) { return valueFromNative(e, v); };
    return *this;
}

Module& Module::value(const std::string& name, double v) {
    values_[name] = [v](Engine& e) { return valueFromNative(e, v); };
    return *this;
}

Module& Module::value(const std::string& name, bool v) {
    values_[name] = [v](Engine& e) { return valueFromNative(e, v); };
    return *this;
}

Module& Module::value(const std::string& name, std::string v) {
    values_[name] = [v = std::move(v)](Engine& e) { return valueFromNative(e, v); };
    return *this;
}

} // namespace qjs
