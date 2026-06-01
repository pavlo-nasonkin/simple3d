#pragma once

#include <memory>

class TextureCube;
class Texture2D;

// Прекомпиляция IBL-карт из environment cubemap'а (один раз при загрузке окружения).
// Все методы требуют активного GL-контекста.
class IBLBaker
{
public:
    // Diffuse irradiance: свёртка окружения по полусфере. Маленькое разрешение достаточно.
    static std::shared_ptr<TextureCube> BakeIrradiance(const TextureCube& env, int faceSize = 32);

    // Specular IBL: предфильтрованное окружение, roughness растёт по мип-уровням.
    // NB: генерирует мипы на самом env (нужно для качественной выборки).
    static std::shared_ptr<TextureCube> BakePrefiltered(const TextureCube& env,
                                                        int baseFaceSize = 128,
                                                        int mipLevels = 5);

    // BRDF integration LUT (scale/bias для F0), независим от окружения — RG16F.
    static std::shared_ptr<Texture2D> BakeBRDFLUT(int size = 512);
};
