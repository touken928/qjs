#pragma once

#include <qjs/value.h>

#include <cstdint>
#include <memory>
#include <string>

namespace qjs {

class Engine;
class PromiseBridge;
struct InternalPromise;

/**
 * Owns a QuickJS promise capability (promise + resolve/reject functions).
 * Hosts that need shutdown semantics must track pending promises themselves
 * (e.g. QianJS PromiseRegistry) and reject or wait before teardown.
 */
class Promise {
public:
    Promise(const Promise&) = delete;
    Promise& operator=(const Promise&) = delete;
    Promise(Promise&& other) noexcept;
    Promise& operator=(Promise&& other) noexcept;
    ~Promise();

    Value toValue() const;

    void resolveVoid();
    void resolveString(const std::string& data);
    void resolveInt64(int64_t n);
    void resolveBytes(const uint8_t* data, size_t len);
    void resolve(Value value);
    void reject(const std::string& error, const std::string& code = {});

private:
    friend class Engine;
    Promise(PromiseBridge* bridge, InternalPromise* impl);

    PromiseBridge* bridge_ = nullptr;
    std::unique_ptr<InternalPromise> impl_;
};

} // namespace qjs
