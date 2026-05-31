#pragma once

#include "../engine/state.h"
#include "installer.h"

#include <quickjs.h>

namespace qjs {

class ModuleLoader {
public:
    void bind(EngineState* state, ModuleInstaller* installer) {
        state_ = state;
        installer_ = installer;
    }

    static JSModuleDef* loadModuleCallback(JSContext* c, const char* name, void* opaque);

    Module* findCppModule(const std::string& name) const;
    JSModuleDef* createCppModule(JSContext* c, const char* name, Module* mod);
    static int initModule(JSContext* c, JSModuleDef* m);

    void clearCache() {
        if (state_) {
            state_->jsModuleCache.clear();
            state_->modData.clear();
        }
    }

private:
    EngineState* state_ = nullptr;
    ModuleInstaller* installer_ = nullptr;
};

} // namespace qjs
