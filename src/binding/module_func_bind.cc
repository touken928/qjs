#include "module_func_bind.h"

#include <qjs/module.h>
#include <qjs/value.h>

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace qjs {

#define QJS_BIND_FUNC(SIG, RET, ...)                                                                                   \
    Module& Module::func(const std::string& name, std::function<SIG> f) {                                              \
        funcs_[name] = detail::makeFuncHolder<RET, ##__VA_ARGS__>(std::move(f));                                      \
        return *this;                                                                                                  \
    }

QJS_BIND_FUNC(void(), void)
QJS_BIND_FUNC(void(int), void, int)
QJS_BIND_FUNC(void(int64_t), void, int64_t)
QJS_BIND_FUNC(void(double), void, double)
QJS_BIND_FUNC(void(double, double), void, double, double)
QJS_BIND_FUNC(void(double, double, double, double), void, double, double, double, double)
QJS_BIND_FUNC(void(std::string), void, std::string)
QJS_BIND_FUNC(void(std::vector<Value>), void, std::vector<Value>)
QJS_BIND_FUNC(int(), int)
QJS_BIND_FUNC(int(int), int, int)
QJS_BIND_FUNC(int64_t(), int64_t)
QJS_BIND_FUNC(std::string(), std::string)
QJS_BIND_FUNC(std::string(std::string), std::string, std::string)
QJS_BIND_FUNC(double(), double)
QJS_BIND_FUNC(Value(), Value)
QJS_BIND_FUNC(Value(std::string), Value, std::string)
QJS_BIND_FUNC(Value(Value), Value, Value)
QJS_BIND_FUNC(Value(Value, int64_t), Value, Value, int64_t)
QJS_BIND_FUNC(Value(Value, Value), Value, Value, Value)
QJS_BIND_FUNC(Value(int, int), Value, int, int)
QJS_BIND_FUNC(Value(std::string, Value), Value, std::string, Value)

#undef QJS_BIND_FUNC

Module& Module::func(const std::string& name, std::function<Value(int, int, std::optional<Value>)> f) {
    funcs_[name] = detail::makeFuncHolder<Value, int, int, std::optional<Value>>(std::move(f));
    return *this;
}

Module& Module::func(const std::string& name, std::function<Value(std::optional<std::string>)> f) {
    funcs_[name] = detail::makeFuncHolder<Value, std::optional<std::string>>(std::move(f));
    return *this;
}

Module& Module::func(const std::string& name, std::function<Value(Value, Value, std::optional<Value>)> f) {
    funcs_[name] = detail::makeFuncHolder<Value, Value, Value, std::optional<Value>>(std::move(f));
    return *this;
}

} // namespace qjs
