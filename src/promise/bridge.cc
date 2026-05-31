#include "bridge.h"

#include "../binding/value_bridge.h"
#include "../runtime/error.h"

namespace qjs {

void PromiseBridge::bind(JSContext* ctx, ErrorReporter* errors, std::function<void()> onSettled) {
    ctx_ = ctx;
    errors_ = errors;
    onSettled_ = std::move(onSettled);
}

void PromiseBridge::settleCall(JSValue r) {
    if (JS_IsException(r) && errors_) {
        errors_->dump(ctx_);
    }
    if (ctx_) {
        JS_FreeValue(ctx_, r);
    }
    if (onSettled_) {
        onSettled_();
    }
}

std::unique_ptr<InternalPromise> PromiseBridge::create() {
    if (!ctx_) {
        return nullptr;
    }
    JSValue funcs[2];
    JSValue promise = JS_NewPromiseCapability(ctx_, funcs);
    if (JS_IsException(promise)) {
        if (errors_) {
            errors_->dump(ctx_);
        }
        return nullptr;
    }
    auto ip = std::make_unique<InternalPromise>();
    ip->promise = promise;
    ip->resolve = funcs[0];
    ip->reject = funcs[1];
    return ip;
}

Value PromiseBridge::promiseValue(const InternalPromise* ip) const {
    if (!ip || !ctx_) {
        return {};
    }
    return detail::makeValue(ctx_, JS_DupValue(ctx_, ip->promise));
}

void PromiseBridge::resolveString(InternalPromise* ip, const std::string& data) {
    if (!ip || !ctx_) {
        return;
    }
    JSValue arg = JS_NewString(ctx_, data.c_str());
    JSValue r = JS_Call(ctx_, ip->resolve, JS_UNDEFINED, 1, &arg);
    JS_FreeValue(ctx_, arg);
    settleCall(r);
}

void PromiseBridge::resolveInt64(InternalPromise* ip, int64_t n) {
    if (!ip || !ctx_) {
        return;
    }
    JSValue arg = JS_NewInt64(ctx_, n);
    JSValue r = JS_Call(ctx_, ip->resolve, JS_UNDEFINED, 1, &arg);
    JS_FreeValue(ctx_, arg);
    settleCall(r);
}

void PromiseBridge::resolveVoid(InternalPromise* ip) {
    if (!ip || !ctx_) {
        return;
    }
    JSValue undef = JS_UNDEFINED;
    JSValue r = JS_Call(ctx_, ip->resolve, JS_UNDEFINED, 1, &undef);
    settleCall(r);
}

void PromiseBridge::resolveValue(InternalPromise* ip, JSValue value) {
    if (!ip || !ctx_) {
        return;
    }
    JSValue r = JS_Call(ctx_, ip->resolve, JS_UNDEFINED, 1, &value);
    JS_FreeValue(ctx_, value);
    settleCall(r);
}

void PromiseBridge::resolveBytes(InternalPromise* ip, const uint8_t* data, size_t len) {
    if (!ip || !ctx_) {
        return;
    }
    static const uint8_t empty_buf{};
    const uint8_t* src = (len == 0) ? &empty_buf : data;
    JSValue ab = JS_NewArrayBufferCopy(ctx_, src, len);
    if (JS_IsException(ab)) {
        if (errors_) {
            errors_->dump(ctx_);
        }
        return;
    }
    JSValue r = JS_Call(ctx_, ip->resolve, JS_UNDEFINED, 1, &ab);
    JS_FreeValue(ctx_, ab);
    settleCall(r);
}

void PromiseBridge::reject(InternalPromise* ip, const std::string& error) {
    reject(ip, error, std::string{});
}

void PromiseBridge::reject(InternalPromise* ip, const std::string& error, const std::string& code) {
    if (!ip || !ctx_) {
        return;
    }
    JSValue errObj = JS_NewError(ctx_);
    JS_DefinePropertyValueStr(ctx_, errObj, "message", JS_NewString(ctx_, error.c_str()), JS_PROP_C_W_E);
    if (!code.empty()) {
        JS_DefinePropertyValueStr(ctx_, errObj, "code", JS_NewString(ctx_, code.c_str()), JS_PROP_C_W_E);
    }
    JSValue r = JS_Call(ctx_, ip->reject, JS_UNDEFINED, 1, &errObj);
    JS_FreeValue(ctx_, errObj);
    settleCall(r);
}

void PromiseBridge::destroy(InternalPromise* ip) {
    if (!ip) {
        return;
    }
    if (ctx_) {
        JS_FreeValue(ctx_, ip->promise);
        JS_FreeValue(ctx_, ip->resolve);
        JS_FreeValue(ctx_, ip->reject);
    }
    delete ip;
}

} // namespace qjs
