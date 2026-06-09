#include "AnimatorBehaviour.h"

#include <cassert>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#include "utils/AssimpGlm.h"

void AnimatorBehaviour::Configure(const aiScene* scene,
                                  const glm::mat4& globalInverse,
                                  std::map<std::string, unsigned int> boneMapping,
                                  std::vector<BoneInfo> boneInfos,
                                  std::shared_ptr<std::vector<glm::mat4>> transforms)
{
    _scene = scene;
    _globalInverse = globalInverse;
    _boneMapping = std::move(boneMapping);
    _boneInfos = std::move(boneInfos);
    _transforms = std::move(transforms);

    // До первого тика — единичные матрицы (bind pose), чтобы меш не схлопнулся,
    // если анимация ещё не проигрывалась (напр. в editor-режиме).
    if (_transforms) {
        _transforms->assign(_boneInfos.size(), glm::mat4(1.0f));
    }
}

void AnimatorBehaviour::OnUpdate(float deltaTime)
{
    _time += deltaTime;
    BoneTransform(_time);
}

void AnimatorBehaviour::BoneTransform(float timeSec)
{
    if (!_scene || _scene->mNumAnimations == 0 || !_transforms) {
        return;
    }
    glm::mat4 identity(1.0f);

    float ticksPerSecond = _scene->mAnimations[0]->mTicksPerSecond != 0 ?
                            _scene->mAnimations[0]->mTicksPerSecond : 25.0f;

    float timeInTicks = timeSec * ticksPerSecond;
    float animationTime = fmod(timeInTicks, _scene->mAnimations[0]->mDuration);

    ReadNodeHierarchy(animationTime, _scene->mRootNode, identity);

    _transforms->resize(_boneInfos.size());
    for (unsigned int i = 0; i < _boneInfos.size(); i++) {
        (*_transforms)[i] = _boneInfos[i].FinalTransformation;
    }
}

void AnimatorBehaviour::ReadNodeHierarchy(float t, const aiNode* pNode, const glm::mat4& parentTransform)
{
    std::string NodeName(pNode->mName.data);

    const aiAnimation* pAnimation = _scene->mAnimations[0];

    glm::mat4 nodeTransformation = ToGlm(pNode->mTransformation);

    if (const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName)) {
        // Interpolate scaling and generate scaling transformation matrix
        glm::vec3 scaling = CalcInterpolatedScaling(t, pNodeAnim);
        glm::mat4 scalingM = glm::scale(glm::mat4(1.0f), scaling);

        // Interpolate rotation and generate rotation transformation matrix
        glm::mat4 rotationM = glm::mat4_cast(CalcInterpolatedRotation(t, pNodeAnim));

        // Interpolate translation and generate translation transformation matrix
        glm::vec3 translation = CalcInterpolatedPosition(t, pNodeAnim);
        glm::mat4 translationM = glm::translate(glm::mat4(1.0f), translation);

        // Combine the above transformations
        nodeTransformation = translationM * rotationM * scalingM;
    }

    glm::mat4 globalTransformation = parentTransform * nodeTransformation;

    if (_boneMapping.find(NodeName) != _boneMapping.end()) {
        unsigned int boneIndex = _boneMapping[NodeName];
        _boneInfos[boneIndex].FinalTransformation = _globalInverse * globalTransformation *
                                                    _boneInfos[boneIndex].BoneOffset;
    }

    for (unsigned int i = 0 ; i < pNode->mNumChildren ; i++) {
        ReadNodeHierarchy(t, pNode->mChildren[i], globalTransformation);
    }
}

unsigned int AnimatorBehaviour::FindRotation(float t, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumRotationKeys > 0);

    for (unsigned int i = 0 ; i < pNodeAnim->mNumRotationKeys - 1 ; i++) {
        if (t < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
            return i;
        }
    }

    assert(0);
    return 0;
}

const aiNodeAnim* AnimatorBehaviour::FindNodeAnim(const aiAnimation* pAnimation, const std::string& nodeName)
{
    for (unsigned int i = 0 ; i < pAnimation->mNumChannels ; i++) {
        const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

        if (std::string(pNodeAnim->mNodeName.data) == nodeName) {
            return pNodeAnim;
        }
    }

    return nullptr;
}

glm::quat AnimatorBehaviour::CalcInterpolatedRotation(float t, const aiNodeAnim* pNodeAnim)
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumRotationKeys == 1) {
        return ToGlm(pNodeAnim->mRotationKeys[0].mValue);
    }

    unsigned int rotationIndex = FindRotation(t, pNodeAnim);
    unsigned int nextRotationIndex = (rotationIndex + 1);
    assert(nextRotationIndex < pNodeAnim->mNumRotationKeys);
    auto deltaTime = pNodeAnim->mRotationKeys[nextRotationIndex].mTime - pNodeAnim->mRotationKeys[rotationIndex].mTime;
    auto factor = (t - (pNodeAnim->mRotationKeys[rotationIndex].mTime)) / deltaTime;
    assert(factor >= 0.0 && factor <= 1.0);
    glm::quat start = ToGlm(pNodeAnim->mRotationKeys[rotationIndex].mValue);
    glm::quat end   = ToGlm(pNodeAnim->mRotationKeys[nextRotationIndex].mValue);
    return glm::normalize(glm::slerp(start, end, static_cast<float>(factor)));
}

glm::vec3 AnimatorBehaviour::CalcInterpolatedScaling(float t, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumScalingKeys == 1) {
        return ToGlm(pNodeAnim->mScalingKeys[0].mValue);
    }

    unsigned int scalingIndex = FindScaling(t, pNodeAnim);
    unsigned int nextScalingIndex = (scalingIndex + 1);
    assert(nextScalingIndex < pNodeAnim->mNumScalingKeys);
    auto deltaTime = (pNodeAnim->mScalingKeys[nextScalingIndex].mTime - pNodeAnim->mScalingKeys[scalingIndex].mTime);
    auto factor = (t - pNodeAnim->mScalingKeys[scalingIndex].mTime) / deltaTime;
    assert(factor >= 0.0f && factor <= 1.0f);
    glm::vec3 start = ToGlm(pNodeAnim->mScalingKeys[scalingIndex].mValue);
    glm::vec3 end   = ToGlm(pNodeAnim->mScalingKeys[nextScalingIndex].mValue);
    return glm::mix(start, end, factor);
}

glm::vec3 AnimatorBehaviour::CalcInterpolatedPosition(float t, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumPositionKeys == 1) {
        return ToGlm(pNodeAnim->mPositionKeys[0].mValue);
    }

    unsigned int positionIndex = FindPosition(t, pNodeAnim);
    unsigned int nextPositionIndex = (positionIndex + 1);
    assert(nextPositionIndex < pNodeAnim->mNumPositionKeys);
    auto deltaTime = pNodeAnim->mPositionKeys[nextPositionIndex].mTime - pNodeAnim->mPositionKeys[positionIndex].mTime;
    auto factor = (t - pNodeAnim->mPositionKeys[positionIndex].mTime) / deltaTime;
    assert(factor >= 0.0f && factor <= 1.0f);
    glm::vec3 start = ToGlm(pNodeAnim->mPositionKeys[positionIndex].mValue);
    glm::vec3 end   = ToGlm(pNodeAnim->mPositionKeys[nextPositionIndex].mValue);
    return glm::mix(start, end, factor);
}

unsigned int AnimatorBehaviour::FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumScalingKeys > 0);

    for (unsigned int i = 0 ; i < pNodeAnim->mNumScalingKeys - 1 ; i++) {
        if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
            return i;
        }
    }

    assert(0);
    return 0;
}

unsigned int AnimatorBehaviour::FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    for (unsigned int i = 0 ; i < pNodeAnim->mNumPositionKeys - 1 ; i++) {
        if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
            return i;
        }
    }

    assert(0);
    return 0;
}
