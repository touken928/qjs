#pragma once

#include <qjs/value.h>

struct JSContext;
struct JSValue;

namespace qjs::detail {

struct ValueImpl;

struct ValueAccess {
    static ValueImpl* impl(Value& v);
    static const ValueImpl* impl(const Value& v);
    static void set(Value& v, JSContext* ctx, JSValue val);
};

Value makeValue(JSContext* ctx, JSValue val);
JSValue raw(const Value& v);
JSContext* context(const Value& v);
bool isException(const Value& v);

} // namespace qjs::detail
