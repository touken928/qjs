#pragma once

#include <qjs/promise.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <quickjs.h>

namespace qjs {

class ErrorReporter;

struct InternalPromise {
    JSValue promise = JS_UNDEFINED;
    JSValue resolve = JS_UNDEFINED;
    JSValue reject = JS_UNDEFINED;
};

class PromiseBridge {
public:
    void bind(JSContext* ctx, ErrorReporter* errors, std::function<void()> onSettled);

    std::unique_ptr<InternalPromise> create();
    Value promiseValue(const InternalPromise* ip) const;
    void resolveString(InternalPromise* ip, const std::string& data);
    void resolveInt64(InternalPromise* ip, int64_t n);
    void resolveVoid(InternalPromise* ip);
    void resolveValue(InternalPromise* ip, JSValue value);
    void resolveBytes(InternalPromise* ip, const uint8_t* data, size_t len);
    void reject(InternalPromise* ip, const std::string& error);
    void reject(InternalPromise* ip, const std::string& error, const std::string& code);
    void destroy(InternalPromise* ip);

private:
    JSContext* ctx_ = nullptr;
    ErrorReporter* errors_ = nullptr;
    std::function<void()> onSettled_;

    void settleCall(JSValue r);
};

} // namespace qjs
