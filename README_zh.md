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

```cpp
#include <qjs/engine.h>

int main() {
    qjs::Engine engine;
    auto status = engine.evalModule("hello.js", "export {};\n");
    return status.success ? 0 : 1;
}
```

## 构建与测试

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```
