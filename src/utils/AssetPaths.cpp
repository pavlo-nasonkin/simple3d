#include "AssetPaths.h"

#include <filesystem>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

std::string AssetPaths::_root;

namespace {
    fs::path executableDir(const char* argv0)
    {
#ifdef _WIN32
        char buf[MAX_PATH];
        const DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
        if (n > 0 && n < MAX_PATH) {
            return fs::path(std::string(buf, n)).parent_path();
        }
#else
        std::error_code ec;
        const fs::path self = fs::read_symlink("/proc/self/exe", ec);
        if (!ec) {
            return self.parent_path();
        }
#endif
        if (argv0 && *argv0) {
            std::error_code ec;
            const fs::path p = fs::absolute(argv0, ec);
            if (!ec) {
                return p.parent_path();
            }
        }
        std::error_code ec;
        return fs::current_path(ec);
    }
}

void AssetPaths::Init(const char* argv0)
{
    std::error_code ec;
    const std::vector<fs::path> bases = { executableDir(argv0), fs::current_path(ec) };

    for (const fs::path& base : bases) {
        fs::path dir = base;
        for (int level = 0; level < 6; ++level) {
            const fs::path candidate = dir / "assets";
            if (fs::is_directory(candidate, ec)) {
                _root = candidate.generic_string();
                return;
            }
            if (!dir.has_parent_path() || dir.parent_path() == dir) {
                break;
            }
            dir = dir.parent_path();
        }
    }

    // Запасной вариант — прежнее относительное поведение.
    _root = fs::path("../assets").generic_string();
}

const std::string& AssetPaths::Root()
{
    return _root;
}

std::string AssetPaths::Resolve(std::string_view relativeToAssets)
{
    return (fs::path(_root) / std::string(relativeToAssets)).generic_string();
}
