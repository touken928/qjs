#pragma once

#include <qjs/value.h>

#include <quickjs.h>

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace qjs {

class Engine;

template <typename T>
struct JSConv;

template <>
struct JSConv<int> {
    static int from(JSContext* c, JSValue v, bool& ok);
    static JSValue to(JSContext* c, int v);
};

template <>
struct JSConv<int64_t> {
    static int64_t from(JSContext* c, JSValue v, bool& ok);
    static JSValue to(JSContext* c, int64_t v);
};

template <>
struct JSConv<double> {
    static double from(JSContext* c, JSValue v, bool& ok);
    static JSValue to(JSContext* c, double v);
};

template <>
struct JSConv<float> {
    static float from(JSContext* c, JSValue v, bool& ok);
    static JSValue to(JSContext* c, float v);
};

template <>
struct JSConv<bool> {
    static bool from(JSContext* c, JSValue v, bool& ok);
    static JSValue to(JSContext* c, bool v);
};

template <>
struct JSConv<std::string> {
    static std::string from(JSContext* c, JSValue v, bool& ok);
    static JSValue to(JSContext* c, const std::string& v);
};

template <typename T>
struct JSConv<std::vector<T>> {
    static std::vector<T> from(JSContext* c, JSValue v, bool& ok);
    static JSValue to(JSContext* c, const std::vector<T>& v);
};

template <>
struct JSConv<Value> {
    static Value from(JSContext* c, JSValue v, bool& ok);
    static JSValue to(JSContext* c, const Value& v);
};

template <typename T>
struct JSConv<std::optional<T>> {
    static std::optional<T> from(JSContext* c, JSValue v, bool& ok);
    static JSValue to(JSContext* c, const std::optional<T>& v);
};

template <typename T>
using decay_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename>
struct dependent_false : std::false_type {};

template <typename T>
struct JSConv {
    static_assert(dependent_false<T>::value,
        "JSConv<T> not specialized. Supported: int, int64_t, double, float, bool, std::string, std::vector<T>, Value");
};

Value valueFromNative(Engine& engine, int v);
Value valueFromNative(Engine& engine, int64_t v);
Value valueFromNative(Engine& engine, double v);
Value valueFromNative(Engine& engine, bool v);
Value valueFromNative(Engine& engine, const std::string& v);

} // namespace qjs
