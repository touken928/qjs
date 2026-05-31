#pragma once

#include <qjs/result.h>
#include <quickjs.h>

#include <cstdint>
#include <string>

namespace qjs {

class EngineState;
class ErrorReporter;
class ModuleInstaller;

class Vm {
public:
    void bind(EngineState* state, ErrorReporter* errors, ModuleInstaller* installer);

    bool initialize();
    void installModulesOnce();
    void cleanup();
    bool isInitialized() const { return ctx_ != nullptr; }
    bool isCleanedUp() const;

    void executePendingJobs();
    bool isJobPending() const;

    Status evalModule(const std::string& virtualName, const std::string& code);
    Status evalScript(const std::string& virtualName, const std::string& code);
    Status runBytecode(const uint8_t* buf, size_t bufLen);

    static CompileResult compile(const std::string& code, const std::string& filename);
    CompileResult compileModule(const std::string& code, const std::string& filename);

    JSRuntime* rt() const { return rt_; }
    JSContext* ctx() const { return ctx_; }

private:
    Status evalImpl(const std::string& virtualName, const std::string& code, int evalFlags);
    Status failException();

    EngineState* state_ = nullptr;
    ErrorReporter* errors_ = nullptr;
    ModuleInstaller* installer_ = nullptr;

    JSRuntime* rt_ = nullptr;
    JSContext* ctx_ = nullptr;
};

} // namespace qjs
