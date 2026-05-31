#pragma once

#include <qjs/module.h>

#include <quickjs.h>

namespace qjs {

namespace detail {
struct FuncBase;
}

class ExportSink {
public:
    virtual ~ExportSink() = default;
    virtual int set(const char* name, JSValue value) = 0;
};

class ModuleInstaller {
public:
    void setContext(JSContext* ctx) { ctx_ = ctx; }
    void setFuncClassId(JSClassID id) { funcClassId_ = id; }

    JSValue createJSFunction(detail::FuncHolder* wrapper);
    void installToObject(JSValue obj, Module& mod);
    bool installModuleExports(JSContext* c, JSModuleDef* m, Module& mod);
    void registerModuleExportsToGlobal(JSValue moduleNS);

    static JSValue callFuncTrampoline(JSContext* c, JSValue this_val, int argc, JSValue* argv, int magic, JSValue* data);

private:
    bool installRecursive(JSContext* c, Module& mod, ExportSink& sink, bool strict);

    JSContext* ctx_ = nullptr;
    JSClassID funcClassId_ = 0;
};

} // namespace qjs
