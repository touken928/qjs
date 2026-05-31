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

Module& Module::func(const std::string& name, std::function<int()> f) {
    funcs_[name] = std::unique_ptr<detail::FuncHolder>(std::make_unique<detail::FuncWrap<int>>(std::move(f)));
    return *this;
}

Module& Module::func(const std::string& name, std::function<int(int)> f) {
    funcs_[name] = std::unique_ptr<detail::FuncHolder>(std::make_unique<detail::FuncWrap<int, int>>(std::move(f)));
    return *this;
}

Module& Module::func(const std::string& name, std::function<void(int)> f) {
    funcs_[name] = std::unique_ptr<detail::FuncHolder>(std::make_unique<detail::FuncWrap<void, int>>(std::move(f)));
    return *this;
}

Module& Module::func(const std::string& name, std::function<void(int64_t)> f) {
    funcs_[name] = std::unique_ptr<detail::FuncHolder>(std::make_unique<detail::FuncWrap<void, int64_t>>(std::move(f)));
    return *this;
}

Module& Module::funcDynamic(const std::string& name, int minArgc, int maxArgc, NativeDynamicFunction fn) {
    funcs_[name] = std::unique_ptr<detail::FuncHolder>(std::make_unique<detail::DynamicFuncWrap>(minArgc, maxArgc, std::move(fn)));
    return *this;
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
