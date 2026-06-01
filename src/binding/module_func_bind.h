#pragma once

#include "module_binding.h"

#include <functional>
#include <memory>

namespace qjs::detail {

template <typename Ret, typename... Args>
std::unique_ptr<FuncHolder> makeFuncHolder(std::function<Ret(Args...)> f) {
    return std::make_unique<FuncWrap<Ret, Args...>>(std::move(f));
}

} // namespace qjs::detail
