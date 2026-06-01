<h1 align="center">qjs</h1>

<p align="center">
  <strong>Lightweight C++17 bindings for <a href="https://github.com/bellard/quickjs">QuickJS</a>: <code>qjs::Engine</code>, module tree, ES modules &amp; bytecode, and <code>engine.install&lt;Plugin&gt;()</code> — public headers under <code>include/qjs/</code>.</strong>
</p>

<p align="center">
  <a href="https://en.cppreference.com/w/cpp/17"><img src="https://img.shields.io/badge/c++-17-blue.svg?style=for-the-badge&logo=c%2B%2B" alt="C++17"></a>
  <a href="https://cmake.org/"><img src="https://img.shields.io/badge/cmake-3.16+-064F8C.svg?style=for-the-badge&logo=cmake" alt="CMake 3.16+"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-Apache%202.0-blue.svg?style=for-the-badge" alt="License: Apache 2.0"></a>
</p>

<p align="center">
  <a href="README_zh.md">简体中文</a>
</p>

---

## Public headers

| Header | Role |
|--------|------|
| [`qjs/engine.h`](include/qjs/engine.h) | `qjs::Engine` — RAII runtime, eval, compile, promises |
| [`qjs/context.h`](include/qjs/context.h) | `qjs::Context` — non-owning view of JS context + module tree |
| [`qjs/module.h`](include/qjs/module.h) | `qjs::Module` — C++ native module binding tree |
| [`qjs/plugin.h`](include/qjs/plugin.h) | `qjs::IPlugin` |
| [`qjs/value.h`](include/qjs/value.h) | `qjs::Value` — opaque JS value handle |
| [`qjs/call.h`](include/qjs/call.h) | `qjs::CallContext` — low-level arg helpers (prefer typed `func`) |
| [`qjs/object.h`](include/qjs/object.h) | `ObjectBuilder` / `ArrayBuilder` — `set` / `push` (typed); `Context::object()` |
| [`qjs/promise.h`](include/qjs/promise.h) | `qjs::Promise` |
| [`qjs/result.h`](include/qjs/result.h) | `qjs::Status`, `qjs::Result<T>`, `qjs::ErrorInfo` |
| [`qjs/resolver.h`](include/qjs/resolver.h) | `qjs::IModuleResolver`, `qjs::FileModuleResolver` |
| [`qjs/qjs.h`](include/qjs/qjs.h) | Umbrella include |

Public headers **do not** include `quickjs.h` or expose `JSContext` / `JSValue` / `JSRuntime`. QuickJS is confined to `src/` inside the qjs library.

### Non-goals

- No libuv, SDL, argv, embed formats, or host shutdown policy (hosts such as QianJS own those).
- No built-in native modules beyond the generic `IPlugin` protocol.
- Hosts must not call `context().raw()` or touch QuickJS C ABI; extend qjs with new opaque helpers instead.

Include as:

```cpp
#include <qjs/engine.h>
```

## Source layout

```text
src/
  engine/     Engine facade, Context, EngineState
  runtime/    Vm (QuickJS lifecycle), ErrorReporter
  module/     ModuleInstaller, ModuleLoader
  promise/    Promise, PromiseBridge
  binding/    Native function class for C++ callbacks
  resolver/   FileModuleResolver
```

## CMake

```cmake
add_subdirectory(third_party/qjs)
target_link_libraries(myapp PRIVATE qjs::qjs)
```

Linking **`qjs::qjs`** adds `include/` so you use `#include <qjs/...>`.

## Example

Register a native module with **values**, **typed functions** (`func`), and a **class-like** factory (`ObjectBuilder` + `func`).

Supported `func` signatures include primitives (`int`, `int64_t`, `double`, `bool`, `std::string`), `void`, `qjs::Value` (objects, promises, errors via `Engine::throwTypeError`), `std::vector<qjs::Value>` (variadic `console.log`), and trailing `std::optional<…>` (e.g. `process.env(key?)`).

```cpp
#include <qjs/engine.h>
#include <qjs/object.h>
#include <qjs/plugin.h>

#include <cmath>
#include <string>

struct DemoPlugin : qjs::IPlugin {
    const char* name() const override { return "demo"; }

    void install(qjs::Context& ctx, qjs::Module& root) override {
        qjs::Module& m = root.module("demo");

        m.value("VERSION", std::string("qjs"));
        m.func("twice", std::function<int(int)>([](int n) { return n * 2; }));
        m.func("greet", [](std::string name) -> std::string { return "Hello, " + name; });

        m.func("Point", [&ctx](double px, double py) -> qjs::Value {
            return ctx.object()
                .set("x", px)
                .set("y", py)
                .func("magnitude", [px, py]() -> double { return std::sqrt(px * px + py * py); })
                .build();
        });
    }
};

int main() {
    qjs::Engine engine;
    engine.install<DemoPlugin>();

    auto status = engine.evalModule("main.js", R"(
import { VERSION, twice, greet, Point } from 'demo';

if (VERSION !== 'qjs') throw new Error('VERSION');
if (twice(21) !== 42) throw new Error('twice');
if (greet('world') !== 'Hello, world') throw new Error('greet');

const p = Point(3, 4);
if (p.x !== 3 || p.y !== 4) throw new Error('fields');
if (p.magnitude() !== 5) throw new Error('magnitude');

export {};
)");
    return status.success ? 0 : 1;
}
```

`install()` binds exports on the engine module tree immediately and keeps the `IPlugin` alive until the `Engine` is destroyed. You can also call `engine.modules().module("demo").func(...)` directly without a plugin.

## Build & tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## License

Apache 2.0 for this wrapper; QuickJS upstream is MIT.
