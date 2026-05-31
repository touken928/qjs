#pragma once

#include <qjs/call.h>
#include <qjs/module.h>

#include "call_bridge.h"
#include "js_conv.h"
#include "value_bridge.h"

#include <quickjs.h>

#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace qjs {

class Engine;
class Module;

namespace detail {

struct FuncBase : FuncHolder {
    ~FuncBase() override = default;
    virtual JSValue call(JSContext*, int, JSValue*) = 0;
    virtual int arity() const = 0;
};

template <typename Tuple, size_t... I>
bool unpack_impl(JSContext* c, JSValue* v, Tuple& result, std::index_sequence<I...>) {
    bool ok = true;
    ((ok = ok && [&]() {
        bool elemOk = false;
        std::get<I>(result) = JSConv<decay_t<std::tuple_element_t<I, Tuple>>>::from(c, v[I], elemOk);
        return elemOk;
    }()),
        ...);
    return ok;
}

template <typename... Args>
std::tuple<decay_t<Args>...> unpackArgs(JSContext* c, JSValue* v, bool& ok) {
    std::tuple<decay_t<Args>...> result{};
    ok = unpack_impl(c, v, result, std::index_sequence_for<Args...>{});
    return result;
}

template <typename Ret, typename... Args>
struct FuncWrap : FuncBase {
    std::function<Ret(Args...)> fn;
    explicit FuncWrap(std::function<Ret(Args...)> f) : fn(std::move(f)) {}
    int arity() const override { return (int)sizeof...(Args); }
    JSValue call(JSContext* c, int argc, JSValue* argv) override {
        constexpr int expected = (int)sizeof...(Args);
        if (argc != expected) {
            return JS_ThrowTypeError(c, "expected exactly %d arguments, got %d", expected, argc);
        }
        bool ok = false;
        auto args = unpackArgs<Args...>(c, argv, ok);
        if (!ok) {
            if (!JS_IsException(JS_GetException(c))) {
                return JS_ThrowTypeError(c, "argument type conversion failed");
            }
            return JS_EXCEPTION;
        }
        if constexpr (std::is_void_v<Ret>) {
            std::apply(fn, args);
            return JS_UNDEFINED;
        } else {
            return JSConv<Ret>::to(c, std::apply(fn, args));
        }
    }
};

struct DynamicFuncWrap;
struct DynamicFuncWrap : FuncBase {
    int mn = 0;
    int mx = 0;
    NativeDynamicFunction f;

    DynamicFuncWrap(int a, int b, NativeDynamicFunction x) : mn(a), mx(b), f(std::move(x)) {}

    int arity() const override { return mx; }

    JSValue call(JSContext* c, int argc, JSValue* argv) override;
};

} // namespace detail

} // namespace qjs
