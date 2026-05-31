#include "installer.h"

#include "../binding/module_binding.h"
#include "../binding/native_function_class.h"
#include "../binding/value_bridge.h"
#include "../engine/state.h"

#include <cstdio>

namespace qjs {

namespace {

class ObjectPropertySink final : public ExportSink {
public:
    ObjectPropertySink(JSContext* c, JSValue obj) : ctx_(c), obj_(obj) {}
    int set(const char* name, JSValue value) override {
        return JS_SetPropertyStr(ctx_, obj_, name, value);
    }

private:
    JSContext* ctx_;
    JSValue obj_;
};

class ModuleExportSink final : public ExportSink {
public:
    ModuleExportSink(JSContext* c, JSModuleDef* m) : ctx_(c), mod_(m) {}
    int set(const char* name, JSValue value) override {
        return JS_SetModuleExport(ctx_, mod_, name, value);
    }

private:
    JSContext* ctx_;
    JSModuleDef* mod_;
};

} // namespace

JSValue ModuleInstaller::callFunc(JSContext* c, JSValue /*thisVal*/, int argc, JSValue* argv, int /*magic*/, JSValue* data) {
    auto* holder = static_cast<detail::FuncHolder*>(JS_GetOpaque(data[0], funcClassId()));
    detail::FuncBase* wrapper = static_cast<detail::FuncBase*>(holder);
    if (!wrapper) {
        return JS_ThrowInternalError(c, "invalid function wrapper");
    }
    return wrapper->call(c, argc, argv);
}

JSValue ModuleInstaller::createJSFunction(detail::FuncHolder* wrapper) {
    JSValue funcData = JS_NewObjectClass(ctx_, funcClassId());
    JS_SetOpaque(funcData, wrapper);
    JSValue fn = JS_NewCFunctionData(ctx_, &ModuleInstaller::callFunc, 0, 0, 1, &funcData);
    JS_FreeValue(ctx_, funcData);
    return fn;
}

bool ModuleInstaller::installRecursive(JSContext* c, Module& mod, ExportSink& sink, bool strict) {
    for (auto& [name, wrapper] : mod.funcs_) {
        JSValue fn = createJSFunction(wrapper.get()); // FuncHolder*
        if (JS_IsException(fn) || sink.set(name.c_str(), fn) < 0) {
            fprintf(stderr, "Error: Failed to install function '%s'\n", name.c_str());
            if (!JS_IsException(fn)) {
                JS_FreeValue(c, fn);
            }
            if (strict) {
                return false;
            }
        }
    }
    auto* state = static_cast<EngineState*>(JS_GetContextOpaque(c));
    Engine* engine = state ? state->engine : nullptr;
    for (auto& [name, creator] : mod.values_) {
        if (!engine) {
            fprintf(stderr, "Error: engine not bound for value '%s'\n", name.c_str());
            if (strict) {
                return false;
            }
            continue;
        }
        Value nativeVal = creator(*engine);
        JSValue val = detail::raw(nativeVal);
        val = JS_DupValue(c, val);
        if (JS_IsException(val) || sink.set(name.c_str(), val) < 0) {
            fprintf(stderr, "Error: Failed to install value '%s'\n", name.c_str());
            if (!JS_IsException(val)) {
                JS_FreeValue(c, val);
            }
            if (strict) {
                return false;
            }
        }
    }
    for (auto& [name, child] : mod.children_) {
        JSValue childObj = JS_NewObject(c);
        if (JS_IsException(childObj)) {
            fprintf(stderr, "Error: Failed to create child module '%s'\n", name.c_str());
            if (strict) {
                return false;
            }
            continue;
        }
        ObjectPropertySink childSink(c, childObj);
        if (!installRecursive(c, *child, childSink, strict)) {
            JS_FreeValue(c, childObj);
            if (strict) {
                return false;
            }
            continue;
        }
        if (sink.set(name.c_str(), childObj) < 0) {
            fprintf(stderr, "Error: Failed to install child module '%s'\n", name.c_str());
            JS_FreeValue(c, childObj);
            if (strict) {
                return false;
            }
        }
    }
    return true;
}

void ModuleInstaller::installToObject(JSValue obj, Module& mod) {
    ObjectPropertySink sink(ctx_, obj);
    (void)installRecursive(ctx_, mod, sink, false);
}

bool ModuleInstaller::installModuleExports(JSContext* c, JSModuleDef* m, Module& mod) {
    ModuleExportSink sink(c, m);
    return installRecursive(c, mod, sink, true);
}

void ModuleInstaller::registerModuleExportsToGlobal(JSValue moduleNS) {
    if (JS_IsUndefined(moduleNS) || JS_IsException(moduleNS)) {
        return;
    }
    JSValue global = JS_GetGlobalObject(ctx_);

    JSPropertyEnum* props = nullptr;
    uint32_t propCount = 0;
    if (JS_GetOwnPropertyNames(ctx_, &props, &propCount, moduleNS, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) == 0) {
        for (uint32_t i = 0; i < propCount; i++) {
            JSValue val = JS_GetProperty(ctx_, moduleNS, props[i].atom);
            if (!JS_IsException(val)) {
                JS_SetProperty(ctx_, global, props[i].atom, JS_DupValue(ctx_, val));
            }
            JS_FreeValue(ctx_, val);
        }
        JS_FreePropertyEnum(ctx_, props, propCount);
    }
    JS_FreeValue(ctx_, global);
}

} // namespace qjs
