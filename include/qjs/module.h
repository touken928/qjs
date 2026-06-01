#pragma once

#include <qjs/value.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

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

    Module& func(const std::string& name, std::function<void()> f);
    Module& func(const std::string& name, std::function<void(int)> f);
    Module& func(const std::string& name, std::function<void(int64_t)> f);
    Module& func(const std::string& name, std::function<void(double)> f);
    Module& func(const std::string& name, std::function<void(double, double)> f);
    Module& func(const std::string& name, std::function<void(double, double, double, double)> f);
    Module& func(const std::string& name, std::function<void(std::string)> f);
    /** Variadic (0–32 args); each element is one JS argument. */
    Module& func(const std::string& name, std::function<void(std::vector<Value>)> f);

    Module& func(const std::string& name, std::function<int()> f);
    Module& func(const std::string& name, std::function<int(int)> f);
    Module& func(const std::string& name, std::function<int64_t()> f);
    Module& func(const std::string& name, std::function<std::string()> f);
    Module& func(const std::string& name, std::function<std::string(std::string)> f);
    Module& func(const std::string& name, std::function<double()> f);

    Module& func(const std::string& name, std::function<Value()> f);
    Module& func(const std::string& name, std::function<Value(std::string)> f);
    Module& func(const std::string& name, std::function<Value(Value)> f);
    Module& func(const std::string& name, std::function<Value(Value, int64_t)> f);
    Module& func(const std::string& name, std::function<Value(Value, Value)> f);
    Module& func(const std::string& name, std::function<Value(int, int)> f);
    Module& func(const std::string& name, std::function<Value(int, int, std::optional<Value>)> f);
    Module& func(const std::string& name, std::function<Value(std::string, Value)> f);
    /** `process.env()` with no args, or `process.env(key)` with one string. */
    Module& func(const std::string& name, std::function<Value(std::optional<std::string>)> f);
    /** Trailing optional argument (e.g. `game.run(canvas, app, opts?)`). */
    Module& func(const std::string& name, std::function<Value(Value, Value, std::optional<Value>)> f);

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
