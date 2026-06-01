#pragma once

#include <qjs/value.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace qjs {

class Engine;
class Context;

/** Fluent builder for plain JS objects. */
class ObjectBuilder {
public:
    explicit ObjectBuilder(Engine& engine);
    explicit ObjectBuilder(Context& ctx);

    ~ObjectBuilder();

    ObjectBuilder(const ObjectBuilder&) = delete;
    ObjectBuilder& operator=(const ObjectBuilder&) = delete;
    ObjectBuilder(ObjectBuilder&&) noexcept;
    ObjectBuilder& operator=(ObjectBuilder&&) noexcept;

    ObjectBuilder& set(std::string_view name, Value value);

    /** Sets a property from a C++ value (`bool`, integers, `double`, string-like, or `Value`). */
    template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Value>>>
    ObjectBuilder& set(std::string_view name, T&& value) {
        using U = std::decay_t<T>;
        if constexpr (std::is_same_v<U, bool>) {
            return setBoolProperty(name, value);
        } else if constexpr (std::is_same_v<U, int64_t>) {
            return setInt64Property(name, value);
        } else if constexpr (std::is_integral_v<U>) {
            return setInt64Property(name, static_cast<int64_t>(value));
        } else if constexpr (std::is_same_v<U, double>) {
            return setDoubleProperty(name, value);
        } else if constexpr (std::is_same_v<U, float>) {
            return setDoubleProperty(name, static_cast<double>(value));
        } else if constexpr (std::is_convertible_v<U, std::string_view>) {
            return setStringProperty(name, std::string_view(value));
        } else {
            static_assert(sizeof(U) == 0, "unsupported ObjectBuilder::set type");
            return *this;
        }
    }

    ObjectBuilder& func(std::string_view name, std::function<void()> f);
    ObjectBuilder& func(std::string_view name, std::function<void(double)> f);
    ObjectBuilder& func(std::string_view name, std::function<void(double, double)> f);
    ObjectBuilder& func(std::string_view name, std::function<void(double, double, double, double)> f);
    ObjectBuilder& func(std::string_view name, std::function<void(std::string)> f);
    ObjectBuilder& func(std::string_view name, std::function<void(std::string, double, double)> f);
    ObjectBuilder& func(std::string_view name, std::function<void(Value)> f);
    ObjectBuilder& func(std::string_view name, std::function<double()> f);
    ObjectBuilder& func(std::string_view name, std::function<Value()> f);
    ObjectBuilder& func(std::string_view name, std::function<Value(std::string)> f);
    ObjectBuilder& func(std::string_view name, std::function<Value(double)> f);

    Value build();

private:
    ObjectBuilder& setStringProperty(std::string_view name, std::string_view value);
    ObjectBuilder& setInt64Property(std::string_view name, int64_t value);
    ObjectBuilder& setDoubleProperty(std::string_view name, double value);
    ObjectBuilder& setBoolProperty(std::string_view name, bool value);

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/** Fluent builder for JS arrays. */
class ArrayBuilder {
public:
    explicit ArrayBuilder(Engine& engine);
    explicit ArrayBuilder(Context& ctx);

    ~ArrayBuilder();

    ArrayBuilder(const ArrayBuilder&) = delete;
    ArrayBuilder& operator=(const ArrayBuilder&) = delete;
    ArrayBuilder(ArrayBuilder&&) noexcept;
    ArrayBuilder& operator=(ArrayBuilder&&) noexcept;

    ArrayBuilder& push(Value value);

    template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Value>>>
    ArrayBuilder& push(T&& value) {
        using U = std::decay_t<T>;
        if constexpr (std::is_same_v<U, bool>) {
            return pushBoolElement(value);
        } else if constexpr (std::is_same_v<U, int64_t>) {
            return pushInt64Element(value);
        } else if constexpr (std::is_integral_v<U>) {
            return pushInt64Element(static_cast<int64_t>(value));
        } else if constexpr (std::is_same_v<U, double>) {
            return pushDoubleElement(value);
        } else if constexpr (std::is_same_v<U, float>) {
            return pushDoubleElement(static_cast<double>(value));
        } else if constexpr (std::is_convertible_v<U, std::string_view>) {
            return pushStringElement(std::string_view(value));
        } else {
            static_assert(sizeof(U) == 0, "unsupported ArrayBuilder::push type");
            return *this;
        }
    }

    Value build();

private:
    ArrayBuilder& pushStringElement(std::string_view value);
    ArrayBuilder& pushInt64Element(int64_t value);
    ArrayBuilder& pushDoubleElement(double value);
    ArrayBuilder& pushBoolElement(bool value);

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace qjs
