#include "module_binding.h"

#include "../engine/state.h"
#include "call_bridge.h"
#include "value_bridge.h"

namespace qjs::detail {

JSValue DynamicFuncWrap::call(JSContext* c, int argc, JSValue* argv) {
    if (argc < mn || argc > mx) {
        return JS_ThrowTypeError(c, "expected %d to %d arguments, got %d", mn, mx, argc);
    }
    auto* state = static_cast<EngineState*>(JS_GetContextOpaque(c));
    Engine* engine = state ? state->engine : nullptr;
    if (!engine) {
        return JS_ThrowInternalError(c, "engine not bound to context");
    }
    CallContext ctx;
    CallContextAccess::bind(ctx, engine, c, argc, argv);
    auto r = f(ctx);
    if (!r.success) {
        if (!r.error.message.empty()) {
            return JS_ThrowTypeError(c, "%s", r.error.message.c_str());
        }
        return JS_ThrowTypeError(c, "native function failed");
    }
    if (isException(r.value)) {
        return raw(r.value);
    }
    JSValue out = raw(r.value);
    return JS_DupValue(c, out);
}

} // namespace qjs::detail
