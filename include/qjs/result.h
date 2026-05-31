#pragma once

#include <string>
#include <utility>
#include <vector>

namespace qjs {

struct ErrorInfo {
    std::string message;
    std::string stack;
    std::string code;
};

struct Status {
    bool success = false;
    ErrorInfo error;

    static Status ok() {
        return Status{true, {}};
    }

    static Status fail(ErrorInfo err) {
        Status s;
        s.error = std::move(err);
        return s;
    }

    static Status fail(std::string message, std::string code = {}) {
        ErrorInfo e;
        e.message = std::move(message);
        e.code = std::move(code);
        return fail(std::move(e));
    }
};

template <typename T>
struct Result {
    bool success = false;
    T value{};
    ErrorInfo error;

    static Result ok(T v) {
        Result r;
        r.success = true;
        r.value = std::move(v);
        return r;
    }

    static Result fail(ErrorInfo err) {
        Result r;
        r.error = std::move(err);
        return r;
    }
};

using Bytecode = std::vector<uint8_t>;
using CompileResult = Result<Bytecode>;

} // namespace qjs
