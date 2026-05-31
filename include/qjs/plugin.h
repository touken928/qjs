#pragma once

#include <qjs/context.h>
#include <qjs/module.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace qjs {

class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual const char* name() const = 0;
    virtual void install(Context& ctx, Module& root) = 0;
};

using PluginPtr = std::unique_ptr<IPlugin>;

class PluginRegistry {
public:
    void add(PluginPtr plugin) {
        if (!plugin) {
            return;
        }
        plugins_.emplace(plugin->name(), std::move(plugin));
    }

    template <typename PluginT, typename... Args>
    PluginT& emplace(Args&&... args) {
        auto p = std::make_unique<PluginT>(std::forward<Args>(args)...);
        PluginT& ref = *p;
        add(std::move(p));
        return ref;
    }

    void installAll(Context& ctx, Module& root) const;

private:
    std::unordered_map<std::string, PluginPtr> plugins_;
};

} // namespace qjs
