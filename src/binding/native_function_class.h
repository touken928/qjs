#pragma once

#include <quickjs.h>

namespace qjs {

JSClassID& funcClassId();
void ensureFuncClassRegistered(JSRuntime* rt);

} // namespace qjs
