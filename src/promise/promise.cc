#include <qjs/promise.h>

#include "../binding/value_bridge.h"
#include "bridge.h"

namespace qjs {

Promise::Promise(PromiseBridge* bridge, InternalPromise* impl)
    : bridge_(bridge), impl_(impl) {}

Promise::Promise(Promise&& other) noexcept {
    bridge_ = other.bridge_;
    impl_ = std::move(other.impl_);
    other.bridge_ = nullptr;
}

Promise& Promise::operator=(Promise&& other) noexcept {
    if (this != &other) {
        if (bridge_ && impl_) {
            bridge_->destroy(impl_.release());
        }
        bridge_ = other.bridge_;
        impl_ = std::move(other.impl_);
        other.bridge_ = nullptr;
    }
    return *this;
}

Promise::~Promise() {
    if (bridge_ && impl_) {
        bridge_->destroy(impl_.release());
    }
}

Value Promise::toValue() const {
    if (!bridge_ || !impl_) {
        return {};
    }
    return bridge_->promiseValue(impl_.get());
}

void Promise::resolveVoid() {
    if (bridge_ && impl_) {
        bridge_->resolveVoid(impl_.get());
    }
}

void Promise::resolveString(const std::string& data) {
    if (bridge_ && impl_) {
        bridge_->resolveString(impl_.get(), data);
    }
}

void Promise::resolveInt64(int64_t n) {
    if (bridge_ && impl_) {
        bridge_->resolveInt64(impl_.get(), n);
    }
}

void Promise::resolveBytes(const uint8_t* data, size_t len) {
    if (bridge_ && impl_) {
        bridge_->resolveBytes(impl_.get(), data, len);
    }
}

void Promise::resolve(Value value) {
    if (bridge_ && impl_) {
        JSContext* c = detail::context(value);
        if (!c) {
            return;
        }
        JSValue v = detail::raw(value);
        bridge_->resolveValue(impl_.get(), JS_DupValue(c, v));
    }
}

void Promise::reject(const std::string& error, const std::string& code) {
    if (bridge_ && impl_) {
        bridge_->reject(impl_.get(), error, code);
    }
}

} // namespace qjs
