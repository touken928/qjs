#pragma once

#include <qjs/module.h>
#include <qjs/resolver.h>

#include <quickjs.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace qjs {
namespace detail {
struct FuncHolder;
}

class Engine;

struct EngineState {
    Engine* engine = nullptr;
    Module rootModule{"global", nullptr};
    std::unordered_map<JSModuleDef*, Module*> modData;
    std::unordered_map<std::string, JSModuleDef*> jsModuleCache;

    bool installed = false;
    bool cleanedUp = false;

    std::unordered_map<const void*, void*> hostStorage;

    FileModuleResolver defaultResolver;
    std::shared_ptr<IModuleResolver> resolver;

    IModuleResolver& activeResolver() {
        return resolver ? *resolver : defaultResolver;
    }

    /** Keeps native function bindings alive for JS objects (e.g. canvas 2d context). */
    std::vector<std::unique_ptr<detail::FuncHolder>> persistent_holders;
};

} // namespace qjs
