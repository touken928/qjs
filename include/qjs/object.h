#pragma once

#include <qjs/call.h>
#include <qjs/value.h>

#include <cstdint>
#include <string>
#include <string_view>

namespace qjs {

class Engine;

/** Fluent builder for plain JS objects. */
class ObjectBuilder {
public:
    explicit ObjectBuilder(Engine& engine);
    ~ObjectBuilder();

    ObjectBuilder(const ObjectBuilder&) = delete;
    ObjectBuilder& operator=(const ObjectBuilder&) = delete;
    ObjectBuilder(ObjectBuilder&&) noexcept;
    ObjectBuilder& operator=(ObjectBuilder&&) noexcept;

    ObjectBuilder& set(std::string_view name, Value value);
    ObjectBuilder& setString(std::string_view name, std::string_view value);
    ObjectBuilder& setInt64(std::string_view name, int64_t value);
    ObjectBuilder& setDouble(std::string_view name, double value);
    ObjectBuilder& setBool(std::string_view name, bool value);
    ObjectBuilder& funcDynamic(std::string_view name, int min_argc, int max_argc, NativeDynamicFunction fn);

    Value build();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/** Fluent builder for JS arrays. */
class ArrayBuilder {
public:
    explicit ArrayBuilder(Engine& engine);
    ~ArrayBuilder();

    ArrayBuilder(const ArrayBuilder&) = delete;
    ArrayBuilder& operator=(const ArrayBuilder&) = delete;
    ArrayBuilder(ArrayBuilder&&) noexcept;
    ArrayBuilder& operator=(ArrayBuilder&&) noexcept;

    ArrayBuilder& push(Value value);
    ArrayBuilder& pushString(std::string_view value);
    ArrayBuilder& pushInt64(int64_t value);
    ArrayBuilder& pushDouble(double value);
    ArrayBuilder& pushBool(bool value);

    Value build();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace qjs
