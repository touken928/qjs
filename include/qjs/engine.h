#pragma once

#include <qjs/engine_access_fwd.h>
#include <qjs/context.h>
#include <qjs/module.h>
#include <qjs/object.h>
#include <qjs/plugin.h>
#include <qjs/promise.h>
#include <qjs/result.h>
#include <qjs/resolver.h>
#include <qjs/value.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace qjs {

struct EngineOptions {
    std::shared_ptr<IModuleResolver> resolver;
    std::function<void(const ErrorInfo&)> onError;
};

/** RAII QuickJS runtime + context. Construct to start; destroy to release. */
class Engine {
public:
    Engine();
    explicit Engine(EngineOptions options);
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) noexcept;
    Engine& operator=(Engine&&) noexcept;

    Context& context();
    const Context& context() const;
    Module& modules();
    const Module& modules() const;

    void setModuleResolver(std::shared_ptr<IModuleResolver> resolver);
    void setErrorCallback(std::function<void(const ErrorInfo&)> callback);

    Status evalModule(const std::string& virtualName, const std::string& source);
    Status evalScript(const std::string& virtualName, const std::string& source);
    Status runBytecode(const uint8_t* buf, size_t len);

    static CompileResult compile(const std::string& code, const std::string& filename);
    CompileResult compileModule(const std::string& code, const std::string& filename);

    std::unique_ptr<Promise> createPromise();

    Value undefined();
    Value nullValue();
    Value string(std::string_view value);
    Value int64(int64_t value);
    Value float64(double value);
    Value boolValue(bool value);
    Value arrayBuffer(const uint8_t* data, size_t len);
    ObjectBuilder object();
    ArrayBuilder array();
    Result<Value> call(const Value& fn, const Value* const* args = nullptr, size_t argc = 0);
    Result<Value> call(const Value& fn, const Value& arg0, const Value& arg1);
    Result<Value> getGlobal(std::string_view name);

    template <typename T>
    void setHost(T* ptr) {
        setHostImpl(typeKey<T>(), static_cast<void*>(ptr));
    }

    template <typename T>
    T* host() const {
        return static_cast<T*>(hostImpl(typeKey<T>()));
    }

    bool isJobPending() const;
    void pumpMicrotasks();

    /** Register a native module plugin (calls `IPlugin::install` immediately). */
    template <typename PluginT, typename... Args>
    PluginT& install(Args&&... args);

    void install(PluginPtr plugin);

    /** Register all plugins from a registry (registry must outlive this engine). */
    void install(const PluginRegistry& registry);

private:
    friend class Context;
    friend class ObjectBuilder;
    friend class ArrayBuilder;
    friend struct detail::EngineAccess;

    void installPlugin(IPlugin& plugin, PluginPtr owned);
    void executePendingJobs();
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::vector<PluginPtr> owned_plugins_;

    using TypeKey = const void*;
    template <typename T>
    static TypeKey typeKey() {
        static int dummy;
        return &dummy;
    }
    void setHostImpl(TypeKey key, void* ptr);
    void* hostImpl(TypeKey key) const;
};

template <typename PluginT, typename... Args>
PluginT& Engine::install(Args&&... args) {
    auto p = std::make_unique<PluginT>(std::forward<Args>(args)...);
    PluginT& ref = *p;
    installPlugin(ref, std::move(p));
    return ref;
}

} // namespace qjs
