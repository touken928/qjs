#include "js_conv.h"

#include "../engine/engine_access.h"
#include "value_bridge.h"
#include "value_impl.h"

namespace qjs {

int JSConv<int>::from(JSContext* c, JSValue v, bool& ok) {
    int32_t r;
    if (JS_ToInt32(c, &r, v) < 0) {
        ok = false;
        return 0;
    }
    ok = true;
    return (int)r;
}

JSValue JSConv<int>::to(JSContext* c, int v) {
    return JS_NewInt32(c, v);
}

int64_t JSConv<int64_t>::from(JSContext* c, JSValue v, bool& ok) {
    int64_t r;
    if (JS_ToInt64(c, &r, v) < 0) {
        ok = false;
        return 0;
    }
    ok = true;
    return r;
}

JSValue JSConv<int64_t>::to(JSContext* c, int64_t v) {
    return JS_NewInt64(c, v);
}

double JSConv<double>::from(JSContext* c, JSValue v, bool& ok) {
    double r;
    if (JS_ToFloat64(c, &r, v) < 0) {
        ok = false;
        return 0;
    }
    ok = true;
    return r;
}

JSValue JSConv<double>::to(JSContext* c, double v) {
    return JS_NewFloat64(c, v);
}

float JSConv<float>::from(JSContext* c, JSValue v, bool& ok) {
    double r;
    if (JS_ToFloat64(c, &r, v) < 0) {
        ok = false;
        return 0;
    }
    ok = true;
    return (float)r;
}

JSValue JSConv<float>::to(JSContext* c, float v) {
    return JS_NewFloat64(c, v);
}

bool JSConv<bool>::from(JSContext* c, JSValue v, bool& ok) {
    if (!JS_IsBool(v)) {
        ok = false;
        return false;
    }
    ok = true;
    return JS_ToBool(c, v);
}

JSValue JSConv<bool>::to(JSContext* c, bool v) {
    return JS_NewBool(c, v);
}

std::string JSConv<std::string>::from(JSContext* c, JSValue v, bool& ok) {
    const char* s = JS_ToCString(c, v);
    if (!s) {
        ok = false;
        return {};
    }
    std::string r = s;
    JS_FreeCString(c, s);
    ok = true;
    return r;
}

JSValue JSConv<std::string>::to(JSContext* c, const std::string& v) {
    return JS_NewString(c, v.c_str());
}

template <typename T>
std::vector<T> JSConv<std::vector<T>>::from(JSContext* c, JSValue v, bool& ok) {
    constexpr int64_t MAX_ARRAY_LENGTH = 1000000;
    if (!JS_IsArray(c, v)) {
        ok = false;
        JS_ThrowTypeError(c, "expected array");
        return {};
    }
    JSValue lenVal = JS_GetPropertyStr(c, v, "length");
    int64_t len = 0;
    if (JS_ToInt64(c, &len, lenVal) < 0) {
        JS_FreeValue(c, lenVal);
        ok = false;
        return {};
    }
    JS_FreeValue(c, lenVal);

    if (len > MAX_ARRAY_LENGTH) {
        ok = false;
        JS_ThrowRangeError(c, "array length %lld exceeds maximum %lld", (long long)len, (long long)MAX_ARRAY_LENGTH);
        return {};
    }

    std::vector<T> result;
    result.reserve((size_t)len);
    for (int64_t i = 0; i < len; i++) {
        JSValue elem = JS_GetPropertyUint32(c, v, (uint32_t)i);
        bool elemOk = false;
        T val = JSConv<T>::from(c, elem, elemOk);
        JS_FreeValue(c, elem);
        if (!elemOk) {
            ok = false;
            JS_ThrowTypeError(c, "array element at index %lld conversion failed", (long long)i);
            return {};
        }
        result.push_back(std::move(val));
    }
    ok = true;
    return result;
}

template <typename T>
JSValue JSConv<std::vector<T>>::to(JSContext* c, const std::vector<T>& v) {
    JSValue arr = JS_NewArray(c);
    if (JS_IsException(arr)) {
        return arr;
    }
    for (size_t i = 0; i < v.size(); i++) {
        JSValue elem = JSConv<T>::to(c, v[i]);
        if (JS_IsException(elem)) {
            JS_FreeValue(c, arr);
            return elem;
        }
        if (JS_SetPropertyUint32(c, arr, (uint32_t)i, elem) < 0) {
            JS_FreeValue(c, arr);
            return JS_EXCEPTION;
        }
    }
    return arr;
}

template struct JSConv<std::vector<int>>;
template struct JSConv<std::vector<std::string>>;

Value JSConv<Value>::from(JSContext* c, JSValue v, bool& ok) {
    (void)c;
    ok = true;
    return detail::makeValue(c, JS_DupValue(c, v));
}

JSValue JSConv<Value>::to(JSContext* c, const Value& v) {
    return detail::ValueAccess::impl(const_cast<Value&>(v))->dup();
}

Value valueFromNative(Engine& engine, int v) {
    return detail::makeValue(detail::engineJsContext(engine), JS_NewInt32(detail::engineJsContext(engine), v));
}

Value valueFromNative(Engine& engine, int64_t v) {
    return detail::makeValue(detail::engineJsContext(engine), JS_NewInt64(detail::engineJsContext(engine), v));
}

Value valueFromNative(Engine& engine, double v) {
    return detail::makeValue(detail::engineJsContext(engine), JS_NewFloat64(detail::engineJsContext(engine), v));
}

Value valueFromNative(Engine& engine, bool v) {
    return detail::makeValue(detail::engineJsContext(engine), JS_NewBool(detail::engineJsContext(engine), v));
}

Value valueFromNative(Engine& engine, const std::string& v) {
    JSContext* c = detail::engineJsContext(engine);
    return detail::makeValue(c, JS_NewString(c, v.c_str()));
}

} // namespace qjs
