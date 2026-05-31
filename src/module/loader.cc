#include "loader.h"

#include "../binding/native_function_class.h"

namespace qjs {

Module* ModuleLoader::findCppModule(const std::string& name) const {
    if (!state_) {
        return nullptr;
    }
    auto it = state_->rootModule.children_.find(name);
    return it != state_->rootModule.children_.end() ? it->second.get() : nullptr;
}

JSModuleDef* ModuleLoader::createCppModule(JSContext* c, const char* name, Module* mod) {
    if (!state_ || !installer_) {
        return nullptr;
    }
    JSModuleDef* m = JS_NewCModule(c, name, &ModuleLoader::initModule);
    if (!m) {
        return nullptr;
    }
    state_->modData[m] = mod;
    for (auto& [n, _] : mod->funcs_) {
        JS_AddModuleExport(c, m, n.c_str());
    }
    for (auto& [n, _] : mod->values_) {
        JS_AddModuleExport(c, m, n.c_str());
    }
    for (auto& [n, _] : mod->children_) {
        JS_AddModuleExport(c, m, n.c_str());
    }
    return m;
}

int ModuleLoader::initModule(JSContext* c, JSModuleDef* m) {
    auto* state = static_cast<EngineState*>(JS_GetContextOpaque(c));
    if (!state) {
        return -1;
    }
    auto it = state->modData.find(m);
    if (it == state->modData.end()) {
        return -1;
    }
    ModuleInstaller installer;
    installer.setContext(c);
    installer.setFuncClassId(funcClassId());
    return installer.installModuleExports(c, m, *it->second) ? 0 : -1;
}

JSModuleDef* ModuleLoader::loadModuleCallback(JSContext* c, const char* name, void* opaque) {
    auto* loader = static_cast<ModuleLoader*>(opaque);
    if (!loader || !loader->state_ || loader->state_->cleanedUp) {
        JS_ThrowInternalError(c, "Engine has been cleaned up");
        return nullptr;
    }

    EngineState& state = *loader->state_;

    if (Module* mod = loader->findCppModule(name)) {
        return loader->createCppModule(c, name, mod);
    }

    const std::string modulePath = name;
    auto cacheIt = state.jsModuleCache.find(modulePath);
    if (cacheIt != state.jsModuleCache.end()) {
        return cacheIt->second;
    }

    auto code = state.activeResolver().load(modulePath);
    if (!code) {
        JS_ThrowReferenceError(c, "module not found: %s", name);
        return nullptr;
    }

    JSValue compiled = JS_Eval(c, code->c_str(), code->size(), name, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    if (JS_IsException(compiled)) {
        return nullptr;
    }
    auto* moduleDef = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(compiled));
    JS_FreeValue(c, compiled);
    state.jsModuleCache[modulePath] = moduleDef;
    return moduleDef;
}

} // namespace qjs
