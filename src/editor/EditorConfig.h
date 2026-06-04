#pragma once

#include <string>

// Небольшой конфиг редактора (editor_config.json в рабочем каталоге).
// Пока хранит путь к последней открытой/сохранённой сцене.
class EditorConfig
{
public:
    static std::string GetLastScene();
    static void SetLastScene(const std::string& path);
};
