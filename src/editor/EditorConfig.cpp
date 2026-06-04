#include "EditorConfig.h"

#include <nlohmann/json.hpp>
#include <fstream>

namespace {
    constexpr const char* kConfigPath = "editor_config.json";
}

std::string EditorConfig::GetLastScene()
{
    std::ifstream file(kConfigPath);
    if (!file) {
        return {};
    }
    try {
        nlohmann::json j;
        file >> j;
        return j.value("lastScene", std::string());
    } catch (const std::exception&) {
        return {};
    }
}

void EditorConfig::SetLastScene(const std::string& path)
{
    nlohmann::json j;
    {
        std::ifstream in(kConfigPath);
        if (in) {
            try { in >> j; } catch (const std::exception&) { j = nlohmann::json::object(); }
        }
    }
    j["lastScene"] = path;

    std::ofstream out(kConfigPath);
    if (out) {
        out << j.dump(2);
    }
}
