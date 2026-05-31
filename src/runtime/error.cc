#include "error.h"

#include <cstdio>

namespace qjs {

ErrorInfo ErrorReporter::capture(JSContext* ctx) {
    ErrorInfo info;
    JSValue ex = JS_GetException(ctx);

    JSValue msgVal = JS_GetPropertyStr(ctx, ex, "message");
    if (!JS_IsUndefined(msgVal) && !JS_IsException(msgVal)) {
        const char* msg = JS_ToCString(ctx, msgVal);
        if (msg) {
            info.message = msg;
            JS_FreeCString(ctx, msg);
        }
    }
    JS_FreeValue(ctx, msgVal);

    if (info.message.empty()) {
        const char* s = JS_ToCString(ctx, ex);
        if (s) {
            info.message = s;
            JS_FreeCString(ctx, s);
        } else {
            info.message = "[unable to convert exception to string]";
        }
    }

    JSValue stack = JS_GetPropertyStr(ctx, ex, "stack");
    if (!JS_IsUndefined(stack) && !JS_IsException(stack)) {
        const char* st = JS_ToCString(ctx, stack);
        if (st) {
            info.stack = st;
            JS_FreeCString(ctx, st);
        }
    }
    JS_FreeValue(ctx, stack);

    JSValue codeVal = JS_GetPropertyStr(ctx, ex, "code");
    if (!JS_IsUndefined(codeVal) && !JS_IsException(codeVal)) {
        const char* c = JS_ToCString(ctx, codeVal);
        if (c) {
            info.code = c;
            JS_FreeCString(ctx, c);
        }
    }
    JS_FreeValue(ctx, codeVal);
    JS_FreeValue(ctx, ex);
    return info;
}

void ErrorReporter::dump(const ErrorInfo& info) {
    if (callback_) {
        callback_(info);
        return;
    }
    std::string errorMsg = "Error: " + info.message;
    if (!info.stack.empty()) {
        errorMsg += "\n" + info.stack;
    }
    fprintf(stderr, "%s\n", errorMsg.c_str());
}

void ErrorReporter::dump(JSContext* ctx) {
    dump(capture(ctx));
}

} // namespace qjs
