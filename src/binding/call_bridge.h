#pragma once

#include <qjs/call.h>

struct JSContext;
struct JSValue;

namespace qjs {
class Engine;
namespace detail {

struct CallContextAccess {
    static void bind(CallContext& ctx, Engine* engine, JSContext* jsctx, int argc, JSValue* argv);
};

} // namespace detail
} // namespace qjs
