#pragma once

#include <qjs/call.h>
#include <qjs/value.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace qjs {

class Engine;
class Context;
class ModuleInstaller;
class ModuleLoader;

namespace detail {
/** Type-erased native function slot (implementation in qjs library). */
class FuncHolder {
public:
    virtual ~FuncHolder() = default;
};
} // namespace detail

class Module {
    friend class Engine;
    friend class Context;
    friend class ModuleInstaller;
    friend class ModuleLoader;

public:
    Module& module(const std::string& name);

    Module& func(const std::string& name, std::function<int()> f);
    Module& func(const std::string& name, std::function<int(int)> f);
    Module& func(const std::string& name, std::function<void(int)> f);
    Module& func(const std::string& name, std::function<void(int64_t)> f);

    Module& funcDynamic(const std::string& name, int minArgc, int maxArgc, NativeDynamicFunction fn);

    Module& value(const std::string& name, int v);
    Module& value(const std::string& name, int64_t v);
    Module& value(const std::string& name, double v);
    Module& value(const std::string& name, bool v);
    Module& value(const std::string& name, std::string v);

private:
    std::string name_;
    Module* parent_ = nullptr;
    std::unordered_map<std::string, std::unique_ptr<Module>> children_;
    std::unordered_map<std::string, std::unique_ptr<detail::FuncHolder>> funcs_;
    std::unordered_map<std::string, std::function<Value(Engine&)>> values_;

public:
    Module(const std::string& name, Module* parent) : name_(name), parent_(parent) {}
};

} // namespace qjs
