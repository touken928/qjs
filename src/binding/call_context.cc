#include <qjs/call.h>
#include <qjs/engine.h>

#include "call_bridge.h"
#include "value_bridge.h"
#include "value_impl.h"

#include <quickjs.h>

namespace qjs {

struct CallContext::Impl {
    Engine* engine = nullptr;
    JSContext* ctx = nullptr;
    int argc = 0;
    JSValue* argv = nullptr;
};

CallContext::CallContext() : impl_(std::make_unique<Impl>()) {}

CallContext::~CallContext() = default;

Engine& CallContext::engine() {
    return *impl_->engine;
}

const Engine& CallContext::engine() const {
    return *impl_->engine;
}

int CallContext::argc() const {
    return impl_->argc;
}

Result<std::string> CallContext::stringArg(int index) const {
    if (index < 0 || index >= impl_->argc) {
        return Result<std::string>::fail(ErrorInfo{"argument index out of range", {}, {}});
    }
    const char* s = JS_ToCString(impl_->ctx, impl_->argv[index]);
    if (!s) {
        return Result<std::string>::fail(ErrorInfo{"expected string", {}, {}});
    }
    std::string out = s;
    JS_FreeCString(impl_->ctx, s);
    return Result<std::string>::ok(std::move(out));
}

Result<int32_t> CallContext::int32Arg(int index) const {
    if (index < 0 || index >= impl_->argc) {
        return Result<int32_t>::fail(ErrorInfo{"argument index out of range", {}, {}});
    }
    int32_t n = 0;
    if (JS_ToInt32(impl_->ctx, &n, impl_->argv[index]) < 0) {
        return Result<int32_t>::fail(ErrorInfo{"expected int32", {}, {}});
    }
    return Result<int32_t>::ok(n);
}

Result<int64_t> CallContext::int64Arg(int index) const {
    if (index < 0 || index >= impl_->argc) {
        return Result<int64_t>::fail(ErrorInfo{"argument index out of range", {}, {}});
    }
    int64_t n = 0;
    if (JS_ToInt64(impl_->ctx, &n, impl_->argv[index]) < 0) {
        return Result<int64_t>::fail(ErrorInfo{"expected int64", {}, {}});
    }
    return Result<int64_t>::ok(n);
}

Result<double> CallContext::float64Arg(int index) const {
    if (index < 0 || index >= impl_->argc) {
        return Result<double>::fail(ErrorInfo{"argument index out of range", {}, {}});
    }
    double n = 0;
    if (JS_ToFloat64(impl_->ctx, &n, impl_->argv[index]) < 0) {
        return Result<double>::fail(ErrorInfo{"expected number", {}, {}});
    }
    return Result<double>::ok(n);
}

Result<bool> CallContext::boolArg(int index) const {
    if (index < 0 || index >= impl_->argc) {
        return Result<bool>::fail(ErrorInfo{"argument index out of range", {}, {}});
    }
    if (!JS_IsBool(impl_->argv[index])) {
        return Result<bool>::fail(ErrorInfo{"expected boolean", {}, {}});
    }
    return Result<bool>::ok(JS_ToBool(impl_->ctx, impl_->argv[index]) != 0);
}

Result<std::vector<uint8_t>> CallContext::bytesArg(int index) const {
    if (index < 0 || index >= impl_->argc) {
        return Result<std::vector<uint8_t>>::fail(ErrorInfo{"argument index out of range", {}, {}});
    }
    Value v = detail::makeValue(impl_->ctx, JS_DupValue(impl_->ctx, impl_->argv[index]));
    return v.toBytes();
}

Result<Value> CallContext::valueArg(int index) const {
    if (index < 0 || index >= impl_->argc) {
        return Result<Value>::fail(ErrorInfo{"argument index out of range", {}, {}});
    }
    return Result<Value>::ok(detail::makeValue(impl_->ctx, JS_DupValue(impl_->ctx, impl_->argv[index])));
}

Value CallContext::undefined() const {
    return detail::makeValue(impl_->ctx, JS_UNDEFINED);
}

Value CallContext::throwTypeError(std::string message) const {
    JSValue ex = JS_ThrowTypeError(impl_->ctx, "%s", message.c_str());
    return detail::makeValue(impl_->ctx, ex);
}

namespace detail {

void CallContextAccess::bind(CallContext& ctx, Engine* engine, JSContext* jsctx, int argc, JSValue* argv) {
    ctx.impl_->engine = engine;
    ctx.impl_->ctx = jsctx;
    ctx.impl_->argc = argc;
    ctx.impl_->argv = argv;
}

} // namespace detail

} // namespace qjs
