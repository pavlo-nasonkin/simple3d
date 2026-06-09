#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <assimp/scene.h>

#include "Behaviour.h"

struct BoneInfo
{
    glm::mat4 BoneOffset = glm::mat4(1.0f);
    glm::mat4 FinalTransformation = glm::mat4(1.0f);
};

// Проигрывает скелетную анимацию ассета: каждый кадр сэмплирует первую анимацию
// aiScene и пишет финальные матрицы костей в общий буфер, который читает
// SkinnedMaterial3D. Раньше это делал ExternalModel::Render — теперь логика живёт
// в OnUpdate (не в рендере).
//
// ВНИМАНИЕ: держит «сырой» указатель на aiScene; его владелец — ExternalModel
// (через Assimp::Importer). Behaviour не переживает владельца, поэтому указатель
// валиден на всём протяжении OnUpdate. OnDetach/деструктор aiScene НЕ трогают.
// TODO: заменить живой aiScene на компактные извлечённые кейфреймы
// (DEVELOP_PLAN 9.10 / REFACTORING_PLAN 8.5).
class AnimatorBehaviour : public Behaviour
{
public:
    // Передаётся ExternalModel'ом после загрузки: сцена, глобальная инверсия,
    // карта костей, их offset-матрицы и общий буфер финальных матриц.
    void Configure(const aiScene* scene,
                   const glm::mat4& globalInverse,
                   std::map<std::string, unsigned int> boneMapping,
                   std::vector<BoneInfo> boneInfos,
                   std::shared_ptr<std::vector<glm::mat4>> transforms);

    std::string GetTypeName() const override { return "Animator"; }

protected:
    void OnUpdate(float deltaTime) override;

private:
    void BoneTransform(float timeSec);
    void ReadNodeHierarchy(float t, const aiNode* node, const glm::mat4& parentTransform);
    const aiNodeAnim* FindNodeAnim(const aiAnimation* animation, const std::string& nodeName);
    unsigned int FindRotation(float t, const aiNodeAnim* nodeAnim);
    unsigned int FindScaling(float t, const aiNodeAnim* nodeAnim);
    unsigned int FindPosition(float t, const aiNodeAnim* nodeAnim);
    glm::quat CalcInterpolatedRotation(float t, const aiNodeAnim* nodeAnim);
    glm::vec3 CalcInterpolatedScaling(float t, const aiNodeAnim* nodeAnim);
    glm::vec3 CalcInterpolatedPosition(float t, const aiNodeAnim* nodeAnim);

    const aiScene* _scene = nullptr;
    glm::mat4 _globalInverse { 1.0f };
    std::map<std::string, unsigned int> _boneMapping;
    std::vector<BoneInfo> _boneInfos;
    std::shared_ptr<std::vector<glm::mat4>> _transforms;
    float _time = 0.0f; // накопленное время анимации (кадронезависимо)
};
