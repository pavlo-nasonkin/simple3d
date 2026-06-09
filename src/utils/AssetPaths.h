#pragma once

#include <string>
#include <string_view>

// Единый резолвер путей к ассетам. Корень (каталог "assets") определяется один раз
// от расположения исполняемого файла (а не от CWD), поэтому запуск из IDE, из
// каталога сборки или из инсталляции находит ассеты одинаково.
//
// Использование:
//   AssetPaths::Init(argv[0]);                       // один раз в начале main()
//   mat = Material3D(AssetPaths::Resolve("shaders/shader.vsh"), ...);
class AssetPaths
{
public:
    // Находит корень ассетов: идёт вверх от каталога exe (и от CWD как запасной
    // вариант) в поисках папки "assets". argv0 — запасной источник пути к exe.
    static void Init(const char* argv0);

    // Абсолютный путь к каталогу "assets".
    static const std::string& Root();

    // Путь относительно корня ассетов: Resolve("shaders/shader.vsh").
    static std::string Resolve(std::string_view relativeToAssets);

private:
    static std::string _root;
};
