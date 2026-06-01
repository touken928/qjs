#pragma once

#include <qjs/result.h>
#include <qjs/value.h>

#include <cstdint>
#include <memory>
#include <string>

namespace qjs {

namespace detail {
struct CallContextAccess;
}

class Engine;

/** Low-level argument helpers (prefer typed `Module::func` / `ObjectBuilder::func`). */
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
    Value throwTypeError(std::string message) const;

private:
    friend class ModuleInstaller;
    friend struct detail::CallContextAccess;
    CallContext();
    ~CallContext();
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace qjs
