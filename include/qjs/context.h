#pragma once

#include <qjs/module.h>

namespace qjs {

class ArrayBuilder;
class Engine;
class ObjectBuilder;

/** Non-owning view of the active JS context and module tree. */
class Context {
public:
    Module& modules();
    const Module& modules() const;

    void pumpMicrotasks();
    bool isJobPending() const;

    Engine& engine() { return *engine_; }
    const Engine& engine() const { return *engine_; }

    /** New plain object builder (same as `engine().object()`). */
    ObjectBuilder object();
    ArrayBuilder array();

private:
    friend class Engine;
    Context(Engine* engine, Module* root);

    Engine* engine_ = nullptr;
    Module* root_ = nullptr;
};

} // namespace qjs
