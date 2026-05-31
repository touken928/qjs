#include "vm.h"

#include "../engine/state.h"
#include "error.h"
#include "../binding/native_function_class.h"
#include "../module/installer.h"

namespace qjs {

bool Vm::isCleanedUp() const {
    return state_ ? state_->cleanedUp : true;
}

void Vm::bind(EngineState* state, ErrorReporter* errors, ModuleInstaller* installer) {
    state_ = state;
    errors_ = errors;
    installer_ = installer;
}

bool Vm::initialize() {
    if (rt_) {
        return true;
    }
    if (!state_) {
        return false;
    }
    state_->cleanedUp = false;
    rt_ = JS_NewRuntime();
    ctx_ = JS_NewContext(rt_);
    ensureFuncClassRegistered(rt_);
    if (installer_) {
        installer_->setContext(ctx_);
        installer_->setFuncClassId(funcClassId());
    }
    return ctx_ != nullptr;
}

void Vm::cleanup() {
    if (!rt_ && !ctx_) {
        return;
    }
    if (state_) {
        state_->cleanedUp = true;
    }

    if (ctx_) {
        JSValue global = JS_GetGlobalObject(ctx_);
        JSPropertyEnum* props = nullptr;
        uint32_t propCount = 0;
        if (JS_GetOwnPropertyNames(ctx_, &props, &propCount, global, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) == 0) {
            for (uint32_t i = 0; i < propCount; i++) {
                JS_DeleteProperty(ctx_, global, props[i].atom, JS_PROP_THROW);
            }
            JS_FreePropertyEnum(ctx_, props, propCount);
        }
        JS_FreeValue(ctx_, global);
        JS_FreeContext(ctx_);
        ctx_ = nullptr;
    }

    if (state_) {
        state_->jsModuleCache.clear();
        state_->modData.clear();
        state_->installed = false;
    }

    if (rt_) {
        JS_FreeRuntime(rt_);
        rt_ = nullptr;
    }
}

void Vm::installModulesOnce() {
    if (!ctx_ || !state_ || state_->installed || !installer_) {
        return;
    }
    state_->installed = true;
    installer_->setContext(ctx_);
    installer_->setFuncClassId(funcClassId());
    JSValue g = JS_GetGlobalObject(ctx_);
    installer_->installToObject(g, state_->rootModule);
    JS_FreeValue(ctx_, g);
}

void Vm::executePendingJobs() {
    if (!rt_) {
        return;
    }
    JSContext* jobCtx = nullptr;
    for (;;) {
        int rc = JS_ExecutePendingJob(rt_, &jobCtx);
        if (rc <= 0) {
            break;
        }
    }
}

bool Vm::isJobPending() const {
    return rt_ && JS_IsJobPending(rt_) != 0;
}

Status Vm::failException() {
    if (errors_ && ctx_) {
        ErrorInfo info = errors_->capture(ctx_);
        errors_->dump(info);
        return Status::fail(std::move(info));
    }
    return Status::fail("JavaScript exception");
}

Status Vm::evalImpl(const std::string& virtualName, const std::string& code, int evalFlags) {
    if (!ctx_ || (state_ && state_->cleanedUp)) {
        return Status::fail("engine not initialized");
    }
    installModulesOnce();
    JSValue r = JS_Eval(ctx_, code.c_str(), code.size(), virtualName.c_str(), evalFlags);
    if (JS_IsException(r)) {
        Status s = failException();
        JS_FreeValue(ctx_, r);
        return s;
    }
    JS_FreeValue(ctx_, r);
    executePendingJobs();
    return Status::ok();
}

Status Vm::evalModule(const std::string& virtualName, const std::string& code) {
    return evalImpl(virtualName, code, JS_EVAL_TYPE_MODULE);
}

Status Vm::evalScript(const std::string& virtualName, const std::string& code) {
    return evalImpl(virtualName, code, JS_EVAL_TYPE_GLOBAL);
}

Status Vm::runBytecode(const uint8_t* buf, size_t bufLen) {
    if (!ctx_ || (state_ && state_->cleanedUp) || !installer_) {
        return Status::fail("engine not initialized");
    }
    installModulesOnce();

    JSValue obj = JS_ReadObject(ctx_, buf, bufLen, JS_READ_OBJ_BYTECODE);
    if (JS_IsException(obj)) {
        return failException();
    }

    bool isModule = (JS_VALUE_GET_TAG(obj) == JS_TAG_MODULE);
    JSModuleDef* moduleDef = nullptr;
    if (isModule) {
        moduleDef = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(obj));
        if (JS_ResolveModule(ctx_, obj) < 0) {
            Status s = failException();
            JS_FreeValue(ctx_, obj);
            return s;
        }
    }

    JSValue result = JS_EvalFunction(ctx_, obj);
    if (JS_IsException(result)) {
        Status s = failException();
        JS_FreeValue(ctx_, result);
        return s;
    }
    JS_FreeValue(ctx_, result);

    if (isModule && moduleDef) {
        JSValue moduleNS = JS_GetModuleNamespace(ctx_, moduleDef);
        if (!JS_IsException(moduleNS)) {
            installer_->registerModuleExportsToGlobal(moduleNS);
            JS_FreeValue(ctx_, moduleNS);
        }
    }
    executePendingJobs();
    return Status::ok();
}

CompileResult Vm::compile(const std::string& code, const std::string& filename) {
    JSRuntime* rt = JS_NewRuntime();
    if (!rt) {
        return CompileResult::fail(Status::fail("Failed to create JS runtime").error);
    }
    JSContext* ctx = JS_NewContext(rt);
    if (!ctx) {
        JS_FreeRuntime(rt);
        return CompileResult::fail(Status::fail("Failed to create JS context").error);
    }

    JSValue obj = JS_Eval(ctx, code.c_str(), code.size(), filename.c_str(),
        JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    if (JS_IsException(obj)) {
        ErrorReporter reporter;
        auto err = reporter.capture(ctx);
        JS_FreeValue(ctx, obj);
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
        return CompileResult::fail(std::move(err));
    }

    size_t outSize = 0;
    uint8_t* outBuf = JS_WriteObject(ctx, &outSize, obj, JS_WRITE_OBJ_BYTECODE);
    JS_FreeValue(ctx, obj);

    if (!outBuf) {
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
        return CompileResult::fail(Status::fail("Failed to serialize bytecode").error);
    }

    Bytecode bytecode(outBuf, outBuf + outSize);
    js_free(ctx, outBuf);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return CompileResult::ok(std::move(bytecode));
}

CompileResult Vm::compileModule(const std::string& code, const std::string& filename) {
    if (!ctx_ || (state_ && state_->cleanedUp)) {
        return CompileResult::fail(Status::fail("engine not initialized").error);
    }
    installModulesOnce();

    JSValue obj = JS_Eval(ctx_, code.c_str(), code.size(), filename.c_str(),
        JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    if (JS_IsException(obj)) {
        ErrorInfo err = errors_ ? errors_->capture(ctx_) : ErrorInfo{};
        if (err.message.empty()) {
            err.message = "compile failed";
        }
        JS_FreeValue(ctx_, obj);
        return CompileResult::fail(std::move(err));
    }

    size_t outSize = 0;
    uint8_t* outBuf = JS_WriteObject(ctx_, &outSize, obj, JS_WRITE_OBJ_BYTECODE);
    JS_FreeValue(ctx_, obj);

    if (!outBuf) {
        return CompileResult::fail(Status::fail("Failed to serialize bytecode").error);
    }

    Bytecode bytecode(outBuf, outBuf + outSize);
    js_free(ctx_, outBuf);
    return CompileResult::ok(std::move(bytecode));
}

} // namespace qjs
