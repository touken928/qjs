#pragma once

#include <memory>
#include <optional>
#include <string>

namespace qjs {

/** Resolves bare module names to ES module source. */
class IModuleResolver {
public:
    virtual ~IModuleResolver() = default;
    virtual std::optional<std::string> load(const std::string& moduleName) = 0;
};

/** Default: read `<name>.js` from the filesystem (appends `.js` when missing). */
class FileModuleResolver final : public IModuleResolver {
public:
    std::optional<std::string> load(const std::string& moduleName) override;
};

} // namespace qjs
