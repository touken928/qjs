#include <gtest/gtest.h>

#include <qjs/engine.h>
#include <qjs/plugin.h>
#include <qjs/resolver.h>
#include <qjs/value.h>

#include <filesystem>
#include <fstream>
#include <random>
#include <string>

namespace {

std::filesystem::path makeTempJsFile(const std::string& content) {
    namespace fs = std::filesystem;
    static std::mt19937_64 rng{std::random_device{}()};
    auto path = fs::temp_directory_path() /
                (std::string("qjs_engine_test_") + std::to_string(rng()) + ".js");
    {
        std::ofstream out(path);
        out << content;
    }
    return path;
}

bool status_ok(const qjs::Status& s) {
    return s.success;
}

} // namespace

TEST(JsEngine, RunEmptyModule) {
    qjs::Engine engine;
    EXPECT_TRUE(status_ok(engine.evalModule("empty.js", "export {};\n")));
}

TEST(JsEngine, EvalScript) {
    qjs::Engine engine;
    EXPECT_TRUE(status_ok(engine.evalScript("stmt.js", "void 0;\n")));
}

TEST(JsEngine, CompileStaticSuccess) {
    auto r = qjs::Engine::compile("export const x = 1;\n", "mod.js");
    EXPECT_TRUE(r.success);
    EXPECT_FALSE(r.value.empty());
    EXPECT_TRUE(r.error.message.empty());
}

TEST(JsEngine, CompileStaticFailure) {
    auto r = qjs::Engine::compile("this is not javascript {{{\n", "bad.js");
    EXPECT_FALSE(r.success);
    EXPECT_TRUE(r.value.empty());
    EXPECT_FALSE(r.error.message.empty());
}

TEST(JsEngine, CompileModuleAfterInit) {
    qjs::Engine engine;
    auto r = engine.compileModule("export const n = 2;\n", "m.js");
    EXPECT_TRUE(r.success);
    EXPECT_FALSE(r.value.empty());
}

TEST(JsEngine, CompileModuleSyntaxError) {
    qjs::Engine engine;
    auto r = engine.compileModule("export {{{\n", "bad.js");
    EXPECT_FALSE(r.success);
    EXPECT_FALSE(r.error.message.empty());
}

TEST(JsEngine, RunBytecodeRoundTrip) {
    auto compiled = qjs::Engine::compile("export {};\n", "bc.js");
    ASSERT_TRUE(compiled.success);
    ASSERT_FALSE(compiled.value.empty());

    qjs::Engine engine;
    EXPECT_TRUE(status_ok(engine.runBytecode(compiled.value.data(), compiled.value.size())));
}

TEST(JsEngine, EvalModuleFromFileContent) {
    auto path = makeTempJsFile("export const ok = true;\n");
    std::ifstream in(path);
    std::string code((std::istreambuf_iterator<char>(in)), {});
    qjs::Engine engine;
    EXPECT_TRUE(status_ok(engine.evalModule(path.string(), code)));
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(JsEngine, HostPointerRoundTrip) {
    int host = 42;
    qjs::Engine engine;
    engine.setHost<int>(&host);
    EXPECT_EQ(engine.host<int>(), &host);
    EXPECT_EQ(*engine.host<int>(), 42);
}

TEST(JsEngine, ContextExposesEngine) {
    int x = 3;
    qjs::Engine engine;
    engine.setHost<int>(&x);
    EXPECT_EQ(engine.context().engine().host<int>(), &x);
}

TEST(JsEngine, NativeModuleFuncBinding) {
    qjs::Engine engine;
    engine.modules().module("math").func("twice", std::function<int(int)>([](int n) { return n * 2; }));
    EXPECT_TRUE(status_ok(engine.evalModule(
        "t.js",
        "import * as m from 'math';\n"
        "if (m.twice(5) !== 10) throw new Error('twice');\n"
        "export {};\n")));
}

TEST(JsEngine, NativeModuleValueBinding) {
    qjs::Engine engine;
    engine.modules().module("cfg").value("label", std::string("hello"));
    EXPECT_TRUE(status_ok(engine.evalModule(
        "t.js",
        "import { label } from 'cfg';\n"
        "if (label !== 'hello') throw new Error('label');\n"
        "export {};\n")));
}

TEST(JsEngine, NestedNativeModuleBinding) {
    qjs::Engine engine;
    engine.modules().module("outer").module("inner").func("id", std::function<int(int)>([](int x) { return x; }));
    EXPECT_TRUE(status_ok(engine.evalModule(
        "t.js",
        "import * as o from 'outer';\n"
        "if (o.inner.id(7) !== 7) throw new Error('nested');\n"
        "export {};\n")));
}

TEST(JsEngine, PluginRegistryInstallAll) {
    struct LabelPlugin : qjs::IPlugin {
        const char* name() const override { return "label_plugin"; }
        void install(qjs::Context&, qjs::Module& root) override {
            root.module("labels").value("ID", 99);
        }
    };

    qjs::PluginRegistry reg;
    reg.emplace<LabelPlugin>();

    qjs::Engine engine;
    reg.installAll(engine.context(), engine.modules());
    EXPECT_TRUE(status_ok(engine.evalModule(
        "p.js",
        "import { ID } from 'labels';\n"
        "if (ID !== 99) throw new Error('ID');\n"
        "export {};\n")));
}

TEST(JsEngine, EvalSyntaxErrorInvokesCallback) {
    qjs::Engine engine;
    std::string captured;
    engine.setErrorCallback([&captured](const qjs::ErrorInfo& info) { captured = info.message; });
    EXPECT_FALSE(status_ok(engine.evalScript("bad.js", "let x = {{{\n")));
    EXPECT_FALSE(captured.empty());
}

TEST(JsEngine, MoveConstructorKeepsWorkingEngine) {
    qjs::Engine a;
    qjs::Engine b = std::move(a);
    EXPECT_TRUE(status_ok(b.evalModule("m.js", "export {};\n")));
}

TEST(JsEngine, PumpMicrotasksAndJobPending) {
    qjs::Engine engine;
    ASSERT_TRUE(status_ok(engine.evalScript("x.js", "void 0;\n")));
    engine.pumpMicrotasks();
    EXPECT_FALSE(engine.isJobPending());
}

TEST(JsEngine, PromiseCreateResolveVoid) {
    qjs::Engine engine;
    auto p = engine.createPromise();
    ASSERT_NE(p, nullptr);
    p->resolveVoid();
}

TEST(JsEngine, EvalModuleSyntaxError) {
    qjs::Engine engine;
    EXPECT_FALSE(status_ok(engine.evalModule("badmod.js", "export {{{\n")));
}

TEST(JsEngine, FileModuleImportAppendsJsSuffix) {
    namespace fs = std::filesystem;
    static std::mt19937_64 rng{std::random_device{}()};
    const fs::path dir = fs::temp_directory_path() / ("qjs_import_dir_" + std::to_string(rng()));
    fs::create_directories(dir);
    const fs::path dep = dir / "depmod.js";
    const fs::path main = dir / "main.js";
    {
        std::ofstream(dep) << "export const v = 21;\n";
        std::ofstream(main) << "import { v } from 'depmod';\n"
                                "if (v !== 21) throw new Error('v');\n"
                                "export {};\n";
    }
    const fs::path prev = fs::current_path();
    fs::current_path(dir);
    std::ifstream in("main.js");
    std::string code((std::istreambuf_iterator<char>(in)), {});
    qjs::Engine engine;
    EXPECT_TRUE(status_ok(engine.evalModule("main.js", code)));
    fs::current_path(prev);
    std::error_code ec;
    fs::remove_all(dir, ec);
}

TEST(JsEngine, ImportMissingModuleFails) {
    qjs::Engine engine;
    EXPECT_FALSE(status_ok(engine.evalModule(
        "m.js",
        "import 'definitely_missing_module_xyz_12345';\n"
        "export {};\n")));
}

TEST(JsEngine, PromiseResolveStringReject) {
    qjs::Engine engine;
    auto p = engine.createPromise();
    ASSERT_NE(p, nullptr);
    p->resolveString("ok");
    auto p2 = engine.createPromise();
    p2->reject("fail", "EFAIL");
}

TEST(JsEngine, PromiseResolveInt64) {
    qjs::Engine engine;
    auto p = engine.createPromise();
    ASSERT_NE(p, nullptr);
    p->resolveInt64(9001);
}

TEST(JsEngine, ValueOpaqueInt64) {
    qjs::Engine engine;
    qjs::Value v = engine.int64(7);
    ASSERT_TRUE(v);
    auto n = v.toInt64();
    ASSERT_TRUE(n.success);
    EXPECT_EQ(n.value, 7);
}

TEST(JsEngine, CustomModuleResolver) {
    struct MemResolver : qjs::IModuleResolver {
        std::optional<std::string> load(const std::string& name) override {
            if (name == "memmod") {
                return "export const x = 11;\n";
            }
            return std::nullopt;
        }
    };

    qjs::EngineOptions opts;
    opts.resolver = std::make_shared<MemResolver>();
    qjs::Engine engine(std::move(opts));
    EXPECT_TRUE(status_ok(engine.evalModule(
        "t.js",
        "import { x } from 'memmod';\n"
        "if (x !== 11) throw new Error('x');\n"
        "export {};\n")));
}

TEST(JsEngine, CompileModuleRoundTripRunBytecode) {
    qjs::Engine engine;
    auto r = engine.compileModule("export const z = 3;\n", "z.js");
    ASSERT_TRUE(r.success);
    ASSERT_FALSE(r.value.empty());
    EXPECT_TRUE(status_ok(engine.runBytecode(r.value.data(), r.value.size())));
}

TEST(JsEngine, ContextPumpMicrotasks) {
    qjs::Engine engine;
    ASSERT_TRUE(status_ok(engine.evalScript("x.js", "void 0;\n")));
    engine.context().pumpMicrotasks();
    EXPECT_FALSE(engine.context().isJobPending());
}
