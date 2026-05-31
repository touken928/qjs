#include <qjs/value.h>

#include "value_bridge.h"
#include "value_impl.h"

#include <quickjs.h>

namespace qjs {

struct Value::Impl {
    detail::ValueImpl inner;
};

namespace detail {

ValueImpl* ValueAccess::impl(Value& v) {
    return v.impl_ ? &v.impl_->inner : nullptr;
}

const ValueImpl* ValueAccess::impl(const Value& v) {
    return v.impl_ ? &v.impl_->inner : nullptr;
}

void ValueAccess::set(Value& v, JSContext* ctx, JSValue val) {
    v.impl_->inner = ValueImpl(ctx, val);
}

Value makeValue(JSContext* ctx, JSValue val) {
    Value v;
    ValueAccess::set(v, ctx, val);
    return v;
}

JSValue raw(const Value& v) {
    const auto* i = ValueAccess::impl(v);
    return i ? i->val : JS_UNDEFINED;
}

JSContext* context(const Value& v) {
    const auto* i = ValueAccess::impl(v);
    return i ? i->ctx : nullptr;
}

bool isException(const Value& v) {
    const auto* i = ValueAccess::impl(v);
    return i && i->ctx && JS_IsException(i->val);
}

} // namespace detail

Value::Value() : impl_(std::make_unique<Impl>()) {}

Value::~Value() = default;

Value::Value(Value&& other) noexcept : impl_(std::move(other.impl_)) {
    if (!impl_) {
        impl_ = std::make_unique<Impl>();
    }
}

Value& Value::operator=(Value&& other) noexcept {
    if (this != &other) {
        impl_ = std::move(other.impl_);
        if (!impl_) {
            impl_ = std::make_unique<Impl>();
        }
    }
    return *this;
}

Value::operator bool() const {
    return valid() && !isUndefined();
}

bool Value::valid() const {
    return impl_ && impl_->inner.valid();
}

bool Value::isUndefined() const {
    return !impl_ || impl_->inner.isUndefined();
}

bool Value::isNull() const {
    const auto* i = detail::ValueAccess::impl(*this);
    return i && i->ctx && JS_IsNull(i->val);
}

bool Value::isFunction() const {
    const auto* i = detail::ValueAccess::impl(*this);
    return i && i->ctx && JS_IsFunction(i->ctx, i->val);
}

bool Value::isObject() const {
    const auto* i = detail::ValueAccess::impl(*this);
    return i && i->ctx && JS_IsObject(i->val);
}

bool Value::isArray() const {
    const auto* i = detail::ValueAccess::impl(*this);
    return i && i->ctx && JS_IsArray(i->ctx, i->val);
}

bool Value::isString() const {
    const auto* i = detail::ValueAccess::impl(*this);
    return i && i->ctx && JS_IsString(i->val);
}

Result<std::string> Value::toString() const {
    const auto* i = detail::ValueAccess::impl(*this);
    if (!i || !i->ctx) {
        return Result<std::string>::fail(ErrorInfo{"invalid value", {}, {}});
    }
    const char* s = JS_ToCString(i->ctx, i->val);
    if (!s) {
        ErrorInfo e;
        e.message = "toString failed";
        return Result<std::string>::fail(std::move(e));
    }
    std::string out = s;
    JS_FreeCString(i->ctx, s);
    return Result<std::string>::ok(std::move(out));
}

Result<int32_t> Value::toInt32() const {
    const auto* i = detail::ValueAccess::impl(*this);
    if (!i || !i->ctx) {
        return Result<int32_t>::fail(ErrorInfo{"invalid value", {}, {}});
    }
    int32_t n = 0;
    if (JS_ToInt32(i->ctx, &n, i->val) < 0) {
        return Result<int32_t>::fail(ErrorInfo{"toInt32 failed", {}, {}});
    }
    return Result<int32_t>::ok(n);
}

Result<int64_t> Value::toInt64() const {
    const auto* i = detail::ValueAccess::impl(*this);
    if (!i || !i->ctx) {
        return Result<int64_t>::fail(ErrorInfo{"invalid value", {}, {}});
    }
    int64_t n = 0;
    if (JS_ToInt64(i->ctx, &n, i->val) < 0) {
        return Result<int64_t>::fail(ErrorInfo{"toInt64 failed", {}, {}});
    }
    return Result<int64_t>::ok(n);
}

Result<double> Value::toFloat64() const {
    const auto* i = detail::ValueAccess::impl(*this);
    if (!i || !i->ctx) {
        return Result<double>::fail(ErrorInfo{"invalid value", {}, {}});
    }
    double n = 0;
    if (JS_ToFloat64(i->ctx, &n, i->val) < 0) {
        return Result<double>::fail(ErrorInfo{"toFloat64 failed", {}, {}});
    }
    return Result<double>::ok(n);
}

Result<bool> Value::toBool() const {
    const auto* i = detail::ValueAccess::impl(*this);
    if (!i || !i->ctx) {
        return Result<bool>::fail(ErrorInfo{"invalid value", {}, {}});
    }
    if (!JS_IsBool(i->val)) {
        return Result<bool>::fail(ErrorInfo{"expected boolean", {}, {}});
    }
    return Result<bool>::ok(JS_ToBool(i->ctx, i->val) != 0);
}

Result<std::vector<uint8_t>> Value::toBytes() const {
    const auto* i = detail::ValueAccess::impl(*this);
    if (!i || !i->ctx) {
        return Result<std::vector<uint8_t>>::fail(ErrorInfo{"invalid value", {}, {}});
    }
    size_t len = 0;
    uint8_t* ptr = JS_GetArrayBuffer(i->ctx, &len, i->val);
    if (ptr) {
        return Result<std::vector<uint8_t>>::ok(std::vector<uint8_t>(ptr, ptr + len));
    }
    size_t boff = 0, blen = 0, bpe = 0;
    JSValue buf = JS_GetTypedArrayBuffer(i->ctx, i->val, &boff, &blen, &bpe);
    if (JS_IsException(buf)) {
        return Result<std::vector<uint8_t>>::fail(ErrorInfo{"expected ArrayBuffer or typed array", {}, {}});
    }
    size_t ablen = 0;
    uint8_t* base = JS_GetArrayBuffer(i->ctx, &ablen, buf);
    JS_FreeValue(i->ctx, buf);
    if (!base || boff + blen > ablen) {
        return Result<std::vector<uint8_t>>::fail(ErrorInfo{"bytes conversion failed", {}, {}});
    }
    return Result<std::vector<uint8_t>>::ok(std::vector<uint8_t>(base + boff, base + boff + blen));
}

Result<Value> Value::getProperty(std::string_view name) const {
    const auto* i = detail::ValueAccess::impl(*this);
    if (!i || !i->ctx) {
        return Result<Value>::fail(ErrorInfo{"invalid value", {}, {}});
    }
    JSValue prop = JS_GetPropertyStr(i->ctx, i->val, std::string(name).c_str());
    if (JS_IsException(prop)) {
        return Result<Value>::fail(ErrorInfo{"getProperty failed", {}, {}});
    }
    return Result<Value>::ok(detail::makeValue(i->ctx, prop));
}

} // namespace qjs
