#pragma once

#include <memory>
#include <string>

class TextureCube;

class HDRLoader
{
public:
    // Загружает equirectangular Radiance .hdr и запекает его в cubemap за один
    // render-pass. Возвращает nullptr при ошибке.
    // Поддерживается формат .hdr (32-bit_rle_rgbe). .exr НЕ поддерживается.
    static std::shared_ptr<TextureCube> EquirectFileToCubemap(const std::string& path, int faceSize = 512);
};
