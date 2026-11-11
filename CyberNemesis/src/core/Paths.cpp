#include "core/Paths.h"
#include "core/Config.h"
#include <cstdlib>
#include <filesystem>
#include <system_error>

namespace Paths {

std::string SaveDir() {
    static std::string dir;
    if (!dir.empty()) return dir;

    std::string base;
    const char* forced = std::getenv("CYBERNEMESIS_SAVE_DIR");
    if (forced && *forced) {
        base = forced;
    } else {
#if defined(_WIN32)
        const char* a = std::getenv("APPDATA");
        if (a && *a) base = std::string(a) + "\\CyberNemesis";
#elif defined(__APPLE__)
        const char* h = std::getenv("HOME");
        if (h && *h) base = std::string(h) + "/Library/Application Support/CyberNemesis";
#else
        const char* x = std::getenv("XDG_DATA_HOME");
        const char* h = std::getenv("HOME");
        if (x && *x)      base = std::string(x) + "/CyberNemesis";
        else if (h && *h) base = std::string(h) + "/.local/share/CyberNemesis";
#endif
    }
    if (base.empty()) base = ".";

    std::error_code ec;
    std::filesystem::create_directories(base, ec);
    if (ec) base = ".";
    dir = base;
    return dir;
}

std::string BrainPath() {
#if defined(_WIN32)
    const char sep = '\\';
#else
    const char sep = '/';
#endif
    return SaveDir() + sep + Cfg::BRAIN_FILE;
}

}
