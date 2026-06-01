#pragma once

#include <qjs/module.h>
#include <qjs/value.h>

#include "js_conv.h"
#include "value_bridge.h"

#include <quickjs.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace qjs {

class Engine;
class Module;

namespace detail {

struct FuncBase : FuncHolder {
    ~FuncBase() override = default;
    virtual JSValue call(JSContext*, int, JSValue*) = 0;
    virtual int arity() const = 0;
};

template <typename T>
struct is_optional : std::false_type {};
template <typename T>
struct is_optional<std::optional<T>> : std::true_type {};

template <typename T>
constexpr bool is_optional_v = is_optional<decay_t<T>>::value;

template <typename... Args>
struct is_rest_vector_pack : std::false_type {};
template <>
struct is_rest_vector_pack<std::vector<Value>> : std::true_type {};

template <typename Tuple, size_t... I, typename F>
bool unpack_impl(JSContext* c, int argc, JSValue* v, Tuple& result, F&& is_missing, std::index_sequence<I...>) {
    bool ok = true;
    ((ok = ok && [&]() {
         if (is_missing(I)) {
             if constexpr (is_optional_v<std::tuple_element_t<I, Tuple>>) {
                 std::get<I>(result) = std::nullopt;
                 return true;
             }
             return false;
         }
         bool elemOk = false;
         using Elem = decay_t<std::tuple_element_t<I, Tuple>>;
         if constexpr (is_optional_v<Elem>) {
             using Inner = typename Elem::value_type;
             Inner inner = JSConv<Inner>::from(c, v[I], elemOk);
             if (!elemOk) {
                 return false;
             }
             std::get<I>(result) = std::move(inner);
         } else {
             std::get<I>(result) = JSConv<Elem>::from(c, v[I], elemOk);
         }
         return elemOk;
     }()),
        ...);
    return ok;
}

template <typename F, typename Tuple, size_t... I>
decltype(auto) apply_moved(F&& f, Tuple& t, std::index_sequence<I...>) {
    return std::forward<F>(f)(std::move(std::get<I>(t))...);
}

template <typename... Args>
std::tuple<decay_t<Args>...> unpackArgs(JSContext* c, int argc, JSValue* v, bool& ok) {
    std::tuple<decay_t<Args>...> result{};
    constexpr size_t n = sizeof...(Args);
    constexpr size_t required = []() {
        size_t r = 0;
        ((r += is_optional_v<decay_t<Args>> ? 0 : 1), ...);
        return r;
    }();
    auto is_missing = [&](size_t i) { return static_cast<size_t>(argc) <= i; };
    if constexpr (is_rest_vector_pack<decay_t<Args>...>::value) {
        if (argc < 0 || argc > 32) {
            ok = false;
            return result;
        }
        std::vector<Value> rest;
        rest.reserve(static_cast<size_t>(argc));
        for (int i = 0; i < argc; i++) {
            bool elemOk = false;
            Value val = JSConv<Value>::from(c, v[i], elemOk);
            if (!elemOk) {
                ok = false;
                return result;
            }
            rest.push_back(std::move(val));
        }
        std::get<0>(result) = std::move(rest);
        ok = true;
        return result;
    }
    if (static_cast<size_t>(argc) < required || static_cast<size_t>(argc) > n) {
        ok = false;
        return result;
    }
    ok = unpack_impl(c, argc, v, result, is_missing, std::index_sequence_for<Args...>{});
    return result;
}

template <typename Ret, typename... Args>
struct FuncWrap : FuncBase {
    std::function<Ret(Args...)> fn;
    explicit FuncWrap(std::function<Ret(Args...)> f) : fn(std::move(f)) {}

    static constexpr bool kRest = is_rest_vector_pack<decay_t<Args>...>::value;

    int arity() const override {
        if constexpr (kRest) {
            return 32;
        }
        return static_cast<int>(sizeof...(Args));
    }

    JSValue call(JSContext* c, int argc, JSValue* argv) override {
        constexpr size_t n = sizeof...(Args);
        constexpr size_t required = []() {
            size_t r = 0;
            ((r += is_optional_v<decay_t<Args>> ? 0 : 1), ...);
            return r;
        }();

        if constexpr (n == 0) {
            if (argc != 0) {
                return JS_ThrowTypeError(c, "expected exactly 0 arguments, got %d", argc);
            }
            if constexpr (std::is_void_v<Ret>) {
                fn();
                return JS_UNDEFINED;
            } else if constexpr (std::is_same_v<Ret, Value>) {
                Value r = fn();
                if (isException(r)) {
                    return raw(r);
                }
                return JSConv<Value>::to(c, r);
            } else {
                return JSConv<Ret>::to(c, fn());
            }
        }

        if constexpr (kRest) {
            if (argc < 0 || argc > 32) {
                return JS_ThrowTypeError(c, "expected 0 to 32 arguments, got %d", argc);
            }
        } else if (static_cast<size_t>(argc) < required || static_cast<size_t>(argc) > n) {
            return JS_ThrowTypeError(c, "expected %zu to %zu arguments, got %d", required, n, argc);
        }
        bool ok = false;
        auto args = unpackArgs<Args...>(c, argc, argv, ok);
        if (!ok) {
            if (!JS_IsException(JS_GetException(c))) {
                return JS_ThrowTypeError(c, "argument type conversion failed");
            }
            return JS_EXCEPTION;
        }
        if constexpr (std::is_void_v<Ret>) {
            apply_moved(fn, args, std::index_sequence_for<Args...>{});
            return JS_UNDEFINED;
        } else if constexpr (std::is_same_v<Ret, Value>) {
            Value r = apply_moved(fn, args, std::index_sequence_for<Args...>{});
            if (isException(r)) {
                return raw(r);
            }
            return JSConv<Value>::to(c, r);
        } else {
            return JSConv<Ret>::to(c, apply_moved(fn, args, std::index_sequence_for<Args...>{}));
        }
    }
};

} // namespace detail

} // namespace qjs
