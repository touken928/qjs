#include <qjs/context.h>
#include <qjs/engine.h>

namespace qjs {

Context::Context(Engine* engine, Module* root) : engine_(engine), root_(root) {}

Module& Context::modules() {
    return *root_;
}

const Module& Context::modules() const {
    return *root_;
}

void Context::pumpMicrotasks() {
    if (engine_) {
        engine_->executePendingJobs();
    }
}

bool Context::isJobPending() const {
    return engine_ && engine_->isJobPending();
}

} // namespace qjs
