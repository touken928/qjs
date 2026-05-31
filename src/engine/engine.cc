#include <qjs/engine.h>
#include <qjs/object.h>
#include <qjs/plugin.h>

#include "engine_access.h"
#include "state.h"
#include "../binding/value_bridge.h"
#include "../binding/value_impl.h"
#include "../runtime/error.h"
#include "../module/installer.h"
#include "../module/loader.h"
#include "../promise/bridge.h"
#include "../runtime/vm.h"

#include <quickjs.h>

namespace qjs {

namespace {

JSModuleDef* moduleLoaderTrampoline(JSContext* c, const char* name, void* opaque) {
    return ModuleLoader::loadModuleCallback(c, name, opaque);
}

} // namespace

struct Engine::Impl {
    EngineState state;
    ErrorReporter errors;
    ModuleInstaller installer;
    ModuleLoader loader;
    Vm vm;
    PromiseBridge promises;
    Context context{nullptr, nullptr};
};

void PluginRegistry::installAll(Context& ctx, Module& root) const {
    for (auto& [_, plugin] : plugins_) {
        plugin->install(ctx, root);
    }
}

Engine::Engine() : Engine(EngineOptions{}) {}

Engine::Engine(EngineOptions options) : impl_(std::make_unique<Impl>()) {
    if (options.resolver) {
        impl_->state.resolver = std::move(options.resolver);
    }
    if (options.onError) {
        impl_->errors.setCallback(std::move(options.onError));
    }
    impl_->vm.bind(&impl_->state, &impl_->errors, &impl_->installer);
    impl_->loader.bind(&impl_->state, &impl_->installer);
    impl_->vm.initialize();
    impl_->state.engine = this;
    impl_->promises.bind(impl_->vm.ctx(), &impl_->errors, [this]() { impl_->vm.executePendingJobs(); });
    JS_SetContextOpaque(impl_->vm.ctx(), &impl_->state);
    JS_SetModuleLoaderFunc(impl_->vm.rt(), nullptr, moduleLoaderTrampoline, &impl_->loader);
    impl_->context = Context(this, &impl_->state.rootModule);
    impl_->vm.installModulesOnce();
}

Engine::~Engine() {
    if (impl_) {
        impl_->vm.cleanup();
    }
}

Engine::Engine(Engine&&) noexcept = default;
Engine& Engine::operator=(Engine&&) noexcept = default;

Context& Engine::context() {
    return impl_->context;
}

const Context& Engine::context() const {
    return impl_->context;
}

Module& Engine::modules() {
    return impl_->state.rootModule;
}

const Module& Engine::modules() const {
    return impl_->state.rootModule;
}

void Engine::setModuleResolver(std::shared_ptr<IModuleResolver> resolver) {
    impl_->state.resolver = std::move(resolver);
}

void Engine::setErrorCallback(std::function<void(const ErrorInfo&)> callback) {
    impl_->errors.setCallback(std::move(callback));
}

Status Engine::evalModule(const std::string& virtualName, const std::string& source) {
    return impl_->vm.evalModule(virtualName, source);
}

Status Engine::evalScript(const std::string& virtualName, const std::string& source) {
    return impl_->vm.evalScript(virtualName, source);
}

Status Engine::runBytecode(const uint8_t* buf, size_t len) {
    return impl_->vm.runBytecode(buf, len);
}

CompileResult Engine::compile(const std::string& code, const std::string& filename) {
    return Vm::compile(code, filename);
}

CompileResult Engine::compileModule(const std::string& code, const std::string& filename) {
    return impl_->vm.compileModule(code, filename);
}

std::unique_ptr<Promise> Engine::createPromise() {
    auto ip = impl_->promises.create();
    if (!ip) {
        return nullptr;
    }
    return std::unique_ptr<Promise>(new Promise(&impl_->promises, ip.release()));
}

void Engine::setHostImpl(TypeKey key, void* ptr) {
    if (!impl_) {
        return;
    }
    impl_->state.hostStorage[key] = ptr;
}

void* Engine::hostImpl(TypeKey key) const {
    if (!impl_) {
        return nullptr;
    }
    auto it = impl_->state.hostStorage.find(key);
    return it == impl_->state.hostStorage.end() ? nullptr : it->second;
}

bool Engine::isJobPending() const {
    return impl_->vm.isJobPending();
}

void Engine::pumpMicrotasks() {
    executePendingJobs();
}

void Engine::executePendingJobs() {
    impl_->vm.executePendingJobs();
}

namespace detail {

JSContext* EngineAccess::ctx(Engine& engine) {
    return engine.impl_->vm.ctx();
}

const JSContext* EngineAccess::ctx(const Engine& engine) {
    return engine.impl_->vm.ctx();
}

} // namespace detail

Value Engine::undefined() {
    return detail::makeValue(detail::engineJsContext(*this), JS_UNDEFINED);
}

Value Engine::nullValue() {
    return detail::makeValue(detail::engineJsContext(*this), JS_NULL);
}

Value Engine::string(std::string_view value) {
    JSContext* c = detail::engineJsContext(*this);
    return detail::makeValue(c, JS_NewStringLen(c, value.data(), value.size()));
}

Value Engine::int64(int64_t value) {
    JSContext* c = detail::engineJsContext(*this);
    return detail::makeValue(c, JS_NewInt64(c, value));
}

Value Engine::float64(double value) {
    JSContext* c = detail::engineJsContext(*this);
    return detail::makeValue(c, JS_NewFloat64(c, value));
}

Value Engine::boolValue(bool value) {
    JSContext* c = detail::engineJsContext(*this);
    return detail::makeValue(c, JS_NewBool(c, value));
}

Value Engine::arrayBuffer(const uint8_t* data, size_t len) {
    JSContext* c = detail::engineJsContext(*this);
    static const uint8_t empty{};
    const uint8_t* src = (len == 0) ? &empty : data;
    return detail::makeValue(c, JS_NewArrayBufferCopy(c, src, len));
}

ObjectBuilder Engine::object() {
    return ObjectBuilder(*this);
}

ArrayBuilder Engine::array() {
    return ArrayBuilder(*this);
}

Result<Value> Engine::call(const Value& fn, const Value& arg0, const Value& arg1) {
    const Value* const argv[] = {&arg0, &arg1};
    return call(fn, argv, 2);
}

Result<Value> Engine::call(const Value& fn, const Value* const* args, size_t argc) {
    JSContext* c = detail::engineJsContext(*this);
    if (!fn.valid() || !fn.isFunction()) {
        return Result<Value>::fail(ErrorInfo{"call target is not a function", {}, {}});
    }
    std::vector<JSValue> argv;
    argv.reserve(argc);
    for (size_t i = 0; i < argc; ++i) {
        argv.push_back(detail::ValueAccess::impl(const_cast<Value&>(*args[i]))->dup());
    }
    JSValue callee = detail::ValueAccess::impl(const_cast<Value&>(fn))->dup();
    JSValue ret = JS_Call(c, callee, JS_UNDEFINED, (int)argv.size(), argv.data());
    JS_FreeValue(c, callee);
    for (JSValue v : argv) {
        JS_FreeValue(c, v);
    }
    if (JS_IsException(ret)) {
        impl_->errors.dump(c);
        return Result<Value>::fail(ErrorInfo{"call threw", {}, {}});
    }
    return Result<Value>::ok(detail::makeValue(c, ret));
}

Result<Value> Engine::getGlobal(std::string_view name) {
    JSContext* c = detail::engineJsContext(*this);
    JSValue g = JS_GetGlobalObject(c);
    JSValue v = JS_GetPropertyStr(c, g, std::string(name).c_str());
    JS_FreeValue(c, g);
    if (JS_IsException(v)) {
        return Result<Value>::fail(ErrorInfo{"getGlobal failed", {}, {}});
    }
    return Result<Value>::ok(detail::makeValue(c, v));
}

} // namespace qjs
