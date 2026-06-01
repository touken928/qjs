<h1 align="center">qjs</h1>

<p align="center">
  <strong>基于 <a href="https://github.com/bellard/quickjs">QuickJS</a> 的轻量 C++17 封装：<code>qjs::Engine</code>、模块树、ES 模块与字节码、<code>engine.install&lt;Plugin&gt;()</code> — 公开头文件在 <code>include/qjs/</code>。</strong>
</p>

<p align="center">
  <a href="README.md">English</a>
</p>

---

## 公开头文件

使用 `#include <qjs/engine.h>` 等形式；也可用 `#include <qjs/qjs.h>` 聚合包含。

| 头文件 | 说明 |
|--------|------|
| `qjs/engine.h` | `qjs::Engine` — RAII 运行时、eval、编译、Promise |
| `qjs/context.h` | `qjs::Context` |
| `qjs/module.h` | `qjs::Module` — 原生模块绑定树 |
| `qjs/plugin.h` | 插件注册 |
| `qjs/value.h` | `qjs::Value` — 不透明 JS 值句柄 |
| `qjs/call.h` | `qjs::CallContext` — 动态原生参数 |
| `qjs/object.h` | `ObjectBuilder` / `ArrayBuilder` |
| `qjs/promise.h` | `qjs::Promise` |
| `qjs/result.h` | `qjs::Status` / `qjs::Result<T>` |
| `qjs/resolver.h` | 模块解析器 |

公开头文件**不**包含 `quickjs.h`，也不暴露 `JSContext` / `JSValue` / `JSRuntime`；QuickJS 仅出现在 qjs 库的 `src/` 中。

### 非目标

- 不提供 libuv、SDL、argv、embed 格式或宿主 shutdown 策略（由 QianJS 等宿主负责）。
- 除通用 `IPlugin` 协议外不提供内置模块。
- 宿主不得使用 `context().raw()` 或直接调用 QuickJS C ABI；应扩展 qjs 的不透明 API。

## 源码目录

`src/engine`、`src/runtime`、`src/module`、`src/promise`、`src/binding`、`src/resolver`。

## CMake

```cmake
add_subdirectory(third_party/qjs)
target_link_libraries(myapp PRIVATE qjs::qjs)
```

## 示例

在原生模块里注册 **常量**（`value`）、**类型化函数**（`func`）、**动态函数**（`funcDynamic`），以及用 `ObjectBuilder` 构造带方法的 **类式对象**（工厂函数 + 实例方法）。

```cpp
#include <qjs/call.h>
#include <qjs/engine.h>
#include <qjs/object.h>
#include <qjs/plugin.h>

#include <cmath>
#include <string>

struct DemoPlugin : qjs::IPlugin {
    const char* name() const override { return "demo"; }

    void install(qjs::Context&, qjs::Module& root) override {
        qjs::Module& m = root.module("demo");

        m.value("VERSION", std::string("qjs"));
        m.func("twice", std::function<int(int)>([](int n) { return n * 2; }));

        m.funcDynamic("greet", 1, 1, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
            auto name = ctx.stringArg(0);
            if (!name.success) {
                return qjs::Result<qjs::Value>::fail(name.error);
            }
            return qjs::Result<qjs::Value>::ok(ctx.engine().string("Hello, " + name.value));
        });

        m.funcDynamic("Point", 2, 2, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
            auto x = ctx.float64Arg(0);
            auto y = ctx.float64Arg(1);
            if (!x.success || !y.success) {
                return qjs::Result<qjs::Value>::fail(
                    qjs::ErrorInfo{"Point: expected (x, y) numbers", {}, {}});
            }
            const double px = x.value;
            const double py = y.value;

            qjs::ObjectBuilder obj(ctx.engine());
            obj.setDouble("x", px);
            obj.setDouble("y", py);
            obj.funcDynamic("magnitude", 0, 0,
                [px, py](qjs::CallContext& c) -> qjs::Result<qjs::Value> {
                    const double mag = std::sqrt(px * px + py * py);
                    return qjs::Result<qjs::Value>::ok(c.engine().float64(mag));
                });
            return qjs::Result<qjs::Value>::ok(obj.build());
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

`install()` 会立刻把导出挂到模块树，并由 `Engine` 持有插件直到析构。也可不用插件，直接 `engine.modules().module("demo").func(...)` 绑定。

## 构建与测试

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```
