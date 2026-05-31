#pragma once

#include <qjs/result.h>
#include <qjs/value.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace qjs {

namespace detail {
struct CallContextAccess;
struct DynamicFuncWrap;
}

class Engine;

/** Argument view for dynamic native functions. */
class CallContext {
public:
    Engine& engine();
    const Engine& engine() const;

    int argc() const;

    Result<std::string> stringArg(int index) const;
    Result<int32_t> int32Arg(int index) const;
    Result<int64_t> int64Arg(int index) const;
    Result<double> float64Arg(int index) const;
    Result<bool> boolArg(int index) const;
    Result<std::vector<uint8_t>> bytesArg(int index) const;
    Result<Value> valueArg(int index) const;

    Value undefined() const;

    /** Sets a pending type error and returns an exception value for the native trampoline. */
    Value throwTypeError(std::string message) const;

private:
    friend class ModuleInstaller;
    friend struct detail::CallContextAccess;
    friend struct detail::DynamicFuncWrap;
    CallContext();
    ~CallContext();
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

using NativeDynamicFunction = std::function<Result<Value>(CallContext&)>;

} // namespace qjs
