#pragma once

#include <qjs/context.h>
#include <qjs/module.h>

#include <memory>

namespace qjs {

class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual const char* name() const = 0;
    virtual void install(Context& ctx, Module& root) = 0;
};

using PluginPtr = std::unique_ptr<IPlugin>;

} // namespace qjs
