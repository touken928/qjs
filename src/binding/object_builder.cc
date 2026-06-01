#include <qjs/object.h>
#include <qjs/context.h>
#include <qjs/engine.h>

#include "../engine/engine_access.h"
#include "../engine/state.h"
#include "../module/installer.h"
#include "module_binding.h"
#include "module_func_bind.h"
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

ObjectBuilder::ObjectBuilder(Context& ctx) : ObjectBuilder(ctx.engine()) {}

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

ObjectBuilder& ObjectBuilder::setStringProperty(std::string_view name, std::string_view value) {
    if (!impl_ || impl_->built) {
        return *this;
    }
    JSValue v = JS_NewStringLen(impl_->ctx, value.data(), value.size());
    JS_SetPropertyStr(impl_->ctx, impl_->obj, std::string(name).c_str(), v);
    return *this;
}

ObjectBuilder& ObjectBuilder::setInt64Property(std::string_view name, int64_t value) {
    if (!impl_ || impl_->built) {
        return *this;
    }
    JSValue v = JS_NewInt64(impl_->ctx, value);
    JS_SetPropertyStr(impl_->ctx, impl_->obj, std::string(name).c_str(), v);
    return *this;
}

ObjectBuilder& ObjectBuilder::setDoubleProperty(std::string_view name, double value) {
    if (!impl_ || impl_->built) {
        return *this;
    }
    JSValue v = JS_NewFloat64(impl_->ctx, value);
    JS_SetPropertyStr(impl_->ctx, impl_->obj, std::string(name).c_str(), v);
    return *this;
}

ObjectBuilder& ObjectBuilder::setBoolProperty(std::string_view name, bool value) {
    if (!impl_ || impl_->built) {
        return *this;
    }
    JSValue v = JS_NewBool(impl_->ctx, value);
    JS_SetPropertyStr(impl_->ctx, impl_->obj, std::string(name).c_str(), v);
    return *this;
}

#define QJS_OBJECT_BIND_FUNC(SIG, RET, ...)                                                                            \
    ObjectBuilder& ObjectBuilder::func(std::string_view name, std::function<SIG> f) {                                  \
        if (!impl_ || impl_->built || !impl_->engine) {                                                                \
            return *this;                                                                                              \
        }                                                                                                              \
        auto wrap = detail::makeFuncHolder<RET, ##__VA_ARGS__>(std::move(f));                                         \
        detail::FuncHolder* holder = wrap.get();                                                                       \
        impl_->funcs.push_back(std::move(wrap));                                                                       \
        JSValue funcData = JS_NewObjectClass(impl_->ctx, funcClassId());                                             \
        JS_SetOpaque(funcData, holder);                                                                                \
        JSValue jfn = JS_NewCFunctionData(impl_->ctx, ModuleInstaller::callFuncTrampoline, 0, 0, 1, &funcData);       \
        JS_FreeValue(impl_->ctx, funcData);                                                                            \
        JS_SetPropertyStr(impl_->ctx, impl_->obj, std::string(name).c_str(), jfn);                                   \
        return *this;                                                                                                  \
    }

QJS_OBJECT_BIND_FUNC(void(), void)
QJS_OBJECT_BIND_FUNC(void(double), void, double)
QJS_OBJECT_BIND_FUNC(void(double, double), void, double, double)
QJS_OBJECT_BIND_FUNC(void(double, double, double, double), void, double, double, double, double)
QJS_OBJECT_BIND_FUNC(void(std::string), void, std::string)
QJS_OBJECT_BIND_FUNC(void(std::string, double, double), void, std::string, double, double)
QJS_OBJECT_BIND_FUNC(void(Value), void, Value)
QJS_OBJECT_BIND_FUNC(double(), double)
QJS_OBJECT_BIND_FUNC(Value(), Value)
QJS_OBJECT_BIND_FUNC(Value(std::string), Value, std::string)
QJS_OBJECT_BIND_FUNC(Value(double), Value, double)

#undef QJS_OBJECT_BIND_FUNC

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

ArrayBuilder::ArrayBuilder(Context& ctx) : ArrayBuilder(ctx.engine()) {}

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

ArrayBuilder& ArrayBuilder::pushStringElement(std::string_view value) {
    if (!impl_ || impl_->built) {
        return *this;
    }
    JSValue v = JS_NewStringLen(impl_->ctx, value.data(), value.size());
    JS_SetPropertyUint32(impl_->ctx, impl_->arr, impl_->index++, v);
    return *this;
}

ArrayBuilder& ArrayBuilder::pushInt64Element(int64_t value) {
    if (!impl_ || impl_->built) {
        return *this;
    }
    JSValue v = JS_NewInt64(impl_->ctx, value);
    JS_SetPropertyUint32(impl_->ctx, impl_->arr, impl_->index++, v);
    return *this;
}

ArrayBuilder& ArrayBuilder::pushDoubleElement(double value) {
    if (!impl_ || impl_->built) {
        return *this;
    }
    JSValue v = JS_NewFloat64(impl_->ctx, value);
    JS_SetPropertyUint32(impl_->ctx, impl_->arr, impl_->index++, v);
    return *this;
}

ArrayBuilder& ArrayBuilder::pushBoolElement(bool value) {
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
