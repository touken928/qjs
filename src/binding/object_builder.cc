#include <qjs/object.h>
#include <qjs/engine.h>

#include "../engine/engine_access.h"
#include "../engine/state.h"
#include "../module/installer.h"
#include "module_binding.h"
#include "native_function_class.h"
#include "value_bridge.h"

#include <quickjs.h>

namespace qjs {

struct ObjectBuilder::Impl {
    Engine* engine = nullptr;
    JSContext* ctx = nullptr;
    JSValue obj = JS_UNDEFINED;
    bool built = false;
    std::vector<std::unique_ptr<detail::FuncHolder>> funcs;
};

struct ArrayBuilder::Impl {
    Engine* engine = nullptr;
    JSContext* ctx = nullptr;
    JSValue arr = JS_UNDEFINED;
    uint32_t index = 0;
    bool built = false;
};

ObjectBuilder::ObjectBuilder(Engine& engine)
    : impl_(std::make_unique<Impl>()) {
    impl_->engine = &engine;
    impl_->ctx = detail::engineJsContext(engine);
    impl_->obj = JS_NewObject(impl_->ctx);
}

ObjectBuilder::~ObjectBuilder() {
    if (impl_ && impl_->ctx && !impl_->built && !JS_IsUndefined(impl_->obj)) {
        JS_FreeValue(impl_->ctx, impl_->obj);
    }
}

ObjectBuilder::ObjectBuilder(ObjectBuilder&& other) noexcept : impl_(std::move(other.impl_)) {}

ObjectBuilder& ObjectBuilder::operator=(ObjectBuilder&& other) noexcept {
    impl_ = std::move(other.impl_);
    return *this;
}

ObjectBuilder& ObjectBuilder::set(std::string_view name, Value value) {
    if (!impl_ || impl_->built) {
        return *this;
    }
    JSValue v = detail::raw(value);
    JS_SetPropertyStr(impl_->ctx, impl_->obj, std::string(name).c_str(), JS_DupValue(impl_->ctx, v));
    return *this;
}

ObjectBuilder& ObjectBuilder::setString(std::string_view name, std::string_view value) {
    if (!impl_ || impl_->built) {
        return *this;
    }
    JSValue v = JS_NewStringLen(impl_->ctx, value.data(), value.size());
    JS_SetPropertyStr(impl_->ctx, impl_->obj, std::string(name).c_str(), v);
    return *this;
}

ObjectBuilder& ObjectBuilder::setInt64(std::string_view name, int64_t value) {
    if (!impl_ || impl_->built) {
        return *this;
    }
    JSValue v = JS_NewInt64(impl_->ctx, value);
    JS_SetPropertyStr(impl_->ctx, impl_->obj, std::string(name).c_str(), v);
    return *this;
}

ObjectBuilder& ObjectBuilder::setDouble(std::string_view name, double value) {
    if (!impl_ || impl_->built) {
        return *this;
    }
    JSValue v = JS_NewFloat64(impl_->ctx, value);
    JS_SetPropertyStr(impl_->ctx, impl_->obj, std::string(name).c_str(), v);
    return *this;
}

ObjectBuilder& ObjectBuilder::setBool(std::string_view name, bool value) {
    if (!impl_ || impl_->built) {
        return *this;
    }
    JSValue v = JS_NewBool(impl_->ctx, value);
    JS_SetPropertyStr(impl_->ctx, impl_->obj, std::string(name).c_str(), v);
    return *this;
}

ObjectBuilder& ObjectBuilder::funcDynamic(std::string_view name, int min_argc, int max_argc, NativeDynamicFunction fn) {
    if (!impl_ || impl_->built || !impl_->engine) {
        return *this;
    }
    auto wrap =
        std::make_unique<detail::DynamicFuncWrap>(min_argc, max_argc, std::move(fn));
    detail::FuncHolder* holder = wrap.get();
    impl_->funcs.push_back(std::move(wrap));

    JSValue funcData = JS_NewObjectClass(impl_->ctx, funcClassId());
    JS_SetOpaque(funcData, holder);
    JSValue jfn = JS_NewCFunctionData(impl_->ctx, ModuleInstaller::callFuncTrampoline, 0, 0, 1, &funcData);
    JS_FreeValue(impl_->ctx, funcData);
    JS_SetPropertyStr(impl_->ctx, impl_->obj, std::string(name).c_str(), jfn);
    return *this;
}

Value ObjectBuilder::build() {
    if (!impl_ || impl_->built) {
        return {};
    }
    impl_->built = true;
    if (impl_->engine) {
        auto* state = static_cast<EngineState*>(JS_GetContextOpaque(impl_->ctx));
        if (state) {
            for (auto& f : impl_->funcs) {
                state->persistent_holders.push_back(std::move(f));
            }
            impl_->funcs.clear();
        }
    }
    return detail::makeValue(impl_->ctx, impl_->obj);
}

ArrayBuilder::ArrayBuilder(Engine& engine) : impl_(std::make_unique<Impl>()) {
    impl_->engine = &engine;
    impl_->ctx = detail::engineJsContext(engine);
    impl_->arr = JS_NewArray(impl_->ctx);
}

ArrayBuilder::~ArrayBuilder() {
    if (impl_ && impl_->ctx && !impl_->built && !JS_IsUndefined(impl_->arr)) {
        JS_FreeValue(impl_->ctx, impl_->arr);
    }
}

ArrayBuilder::ArrayBuilder(ArrayBuilder&& other) noexcept : impl_(std::move(other.impl_)) {}

ArrayBuilder& ArrayBuilder::operator=(ArrayBuilder&& other) noexcept {
    impl_ = std::move(other.impl_);
    return *this;
}

ArrayBuilder& ArrayBuilder::push(Value value) {
    if (!impl_ || impl_->built) {
        return *this;
    }
    JSValue v = detail::raw(value);
    JS_SetPropertyUint32(impl_->ctx, impl_->arr, impl_->index++, JS_DupValue(impl_->ctx, v));
    return *this;
}

ArrayBuilder& ArrayBuilder::pushString(std::string_view value) {
    if (!impl_ || impl_->built) {
        return *this;
    }
    JSValue v = JS_NewStringLen(impl_->ctx, value.data(), value.size());
    JS_SetPropertyUint32(impl_->ctx, impl_->arr, impl_->index++, v);
    return *this;
}

ArrayBuilder& ArrayBuilder::pushInt64(int64_t value) {
    if (!impl_ || impl_->built) {
        return *this;
    }
    JSValue v = JS_NewInt64(impl_->ctx, value);
    JS_SetPropertyUint32(impl_->ctx, impl_->arr, impl_->index++, v);
    return *this;
}

ArrayBuilder& ArrayBuilder::pushDouble(double value) {
    if (!impl_ || impl_->built) {
        return *this;
    }
    JSValue v = JS_NewFloat64(impl_->ctx, value);
    JS_SetPropertyUint32(impl_->ctx, impl_->arr, impl_->index++, v);
    return *this;
}

ArrayBuilder& ArrayBuilder::pushBool(bool value) {
    if (!impl_ || impl_->built) {
        return *this;
    }
    JSValue v = JS_NewBool(impl_->ctx, value);
    JS_SetPropertyUint32(impl_->ctx, impl_->arr, impl_->index++, v);
    return *this;
}

Value ArrayBuilder::build() {
    if (!impl_ || impl_->built) {
        return {};
    }
    impl_->built = true;
    return detail::makeValue(impl_->ctx, impl_->arr);
}

} // namespace qjs
