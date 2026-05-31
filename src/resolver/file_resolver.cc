#include <qjs/resolver.h>

#include <fstream>
#include <sstream>

namespace qjs {

std::optional<std::string> FileModuleResolver::load(const std::string& moduleName) {
    std::string path = moduleName;
    if (path.find(".js") == std::string::npos) {
        path += ".js";
    }
    std::ifstream f(path);
    if (!f) {
        return std::nullopt;
    }
    std::stringstream buf;
    buf << f.rdbuf();
    return buf.str();
}

} // namespace qjs
