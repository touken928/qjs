#pragma once

#include <qjs/result.h>

#include <quickjs.h>

#include <functional>

namespace qjs {

class ErrorReporter {
public:
    void setCallback(std::function<void(const ErrorInfo&)> callback) {
        callback_ = std::move(callback);
    }

    ErrorInfo capture(JSContext* ctx);
    void dump(JSContext* ctx);
    void dump(const ErrorInfo& info);

private:
    std::function<void(const ErrorInfo&)> callback_;
};

} // namespace qjs
