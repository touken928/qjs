#include "native_function_class.h"

namespace qjs {

static JSClassID g_funcClassId = 0;

JSClassID& funcClassId() {
    return g_funcClassId;
}

void ensureFuncClassRegistered(JSRuntime* rt) {
    if (g_funcClassId == 0) {
        JS_NewClassID(&g_funcClassId);
    }
    JSClassDef classDef = {};
    classDef.class_name = "CppFunc";
    classDef.finalizer = nullptr;
    JS_NewClass(rt, g_funcClassId, &classDef);
}

} // namespace qjs
