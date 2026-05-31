#pragma once

#include <quickjs.h>

namespace qjs::detail {

struct ValueImpl {
    JSContext* ctx = nullptr;
    JSValue val = JS_UNDEFINED;

    ValueImpl() = default;
    ValueImpl(JSContext* c, JSValue v) : ctx(c), val(v) {}

    ValueImpl(const ValueImpl&) = delete;
    ValueImpl& operator=(const ValueImpl&) = delete;

    ValueImpl(ValueImpl&& other) noexcept;
    ValueImpl& operator=(ValueImpl&& other) noexcept;

    ~ValueImpl();

    void reset();
    JSValue dup() const;

    bool valid() const { return ctx != nullptr; }
    bool isUndefined() const { return !ctx || JS_IsUndefined(val); }
};

} // namespace qjs::detail
