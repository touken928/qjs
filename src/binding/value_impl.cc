#include "value_impl.h"

namespace qjs::detail {

ValueImpl::ValueImpl(ValueImpl&& other) noexcept {
    ctx = other.ctx;
    val = other.val;
    other.ctx = nullptr;
    other.val = JS_UNDEFINED;
}

ValueImpl& ValueImpl::operator=(ValueImpl&& other) noexcept {
    if (this != &other) {
        reset();
        ctx = other.ctx;
        val = other.val;
        other.ctx = nullptr;
        other.val = JS_UNDEFINED;
    }
    return *this;
}

ValueImpl::~ValueImpl() {
    reset();
}

void ValueImpl::reset() {
    if (ctx && !JS_IsUndefined(val)) {
        JS_FreeValue(ctx, val);
    }
    ctx = nullptr;
    val = JS_UNDEFINED;
}

JSValue ValueImpl::dup() const {
    return ctx ? JS_DupValue(ctx, val) : JS_UNDEFINED;
}

} // namespace qjs::detail
