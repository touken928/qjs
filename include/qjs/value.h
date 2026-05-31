#pragma once

#include <qjs/result.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace qjs {

namespace detail {
struct ValueAccess;
}

class Engine;

/** Opaque RAII holder for a single JavaScript value. */
class Value {
public:
    Value();
    ~Value();

    Value(const Value&) = delete;
    Value& operator=(const Value&) = delete;

    Value(Value&& other) noexcept;
    Value& operator=(Value&& other) noexcept;

    explicit operator bool() const;

    bool valid() const;
    bool isUndefined() const;
    bool isNull() const;
    bool isFunction() const;
    bool isObject() const;
    bool isArray() const;
    bool isString() const;

    Result<std::string> toString() const;
    Result<int32_t> toInt32() const;
    Result<int64_t> toInt64() const;
    Result<double> toFloat64() const;
    Result<bool> toBool() const;
    Result<std::vector<uint8_t>> toBytes() const;
    Result<Value> getProperty(std::string_view name) const;

private:
    friend class Engine;
    friend class CallContext;
    friend class ObjectBuilder;
    friend class ArrayBuilder;
    friend class Promise;
    friend struct detail::ValueAccess;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace qjs
