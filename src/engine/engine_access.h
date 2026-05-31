#pragma once

struct JSContext;

namespace qjs {

class Engine;

namespace detail {

struct EngineAccess {
    static JSContext* ctx(Engine& engine);
    static const JSContext* ctx(const Engine& engine);
};

inline JSContext* engineJsContext(Engine& engine) {
    return EngineAccess::ctx(engine);
}

inline const JSContext* engineJsContext(const Engine& engine) {
    return EngineAccess::ctx(engine);
}

} // namespace detail

} // namespace qjs
