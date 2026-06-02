#include "ExternalModel.h"

#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "resources/Texture2D.h"
#include "resources/TextureManager.h"
#include "Engine.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>
#include "materials/filters/TextureMapFilter.h"
#include "Scene3D.h"
#include "glm/gtc/matrix_transform.hpp"
#include "materials/SkinnedMaterial3D.h"
#include "materials/filters/NormalMapFilter.h"
#include "render/VertexLayoutPresets.h"
#include "render/Geometry.h"
#include "render/GeometryRegistry.h"
#include "utils/AssimpGlm.h"
#include "materials/filters/Filter3d.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

#include "lighting/PBRLightingModel.h"

namespace {
    std::string NormalizeTexturePath(const std::string& raw) {
        std::string p = raw;
        for (auto& c : p) if (c == '\\') c = '/';
        if (p.size() >= 2 && p[0] == '.' && p[1] == '/') {
            p.erase(0, 2);
        }
        return p;
    }

    std::string ToLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    // Declarative description of how each logical material slot is sourced.
    //   assimpTypes        — assimp texture types tried in order (primary first, then legacy fallbacks).
    //   conventionSuffixes — file-name suffixes (case-insensitive, after the last '_') used when assimp
    //                        yields nothing; resolved in priority order, first match wins.
    struct SlotConfig {
        Filter3D::FilterSlot slot;
        std::string typeName;
        Filter3D::BlendMode blend;
        bool normalMap;
        std::vector<aiTextureType> assimpTypes;
        std::vector<std::string> conventionSuffixes;
    };

    const std::vector<SlotConfig> kSlotConfigs = {
        { Filter3D::FilterSlot::BaseColor, "texture_diffuse",   Filter3D::BlendMode::MULTIPLY, false,
          { aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE },
          { "BaseColor", "Albedo", "Diffuse" } },

        { Filter3D::FilterSlot::Specular,  "texture_specular",  Filter3D::BlendMode::MULTIPLY, false,
          { aiTextureType_SPECULAR },
          { "Specular" } },

        { Filter3D::FilterSlot::Normal,    "texture_normal",    Filter3D::BlendMode::NORMAL,   true,
          { aiTextureType_NORMALS, aiTextureType_HEIGHT },
          { "Normal", "NormalGL", "NormalDX" } },

        { Filter3D::FilterSlot::Metallic,  "texture_metalness", Filter3D::BlendMode::NORMAL,   false,
          { aiTextureType_METALNESS },
          { "Metalness", "Metallic" } },

        { Filter3D::FilterSlot::Roughness, "texture_roughness", Filter3D::BlendMode::NORMAL,   false,
          { aiTextureType_DIFFUSE_ROUGHNESS, aiTextureType_SHININESS },
          { "Roughness" } },

        { Filter3D::FilterSlot::AO,        "texture_ao",        Filter3D::BlendMode::NORMAL,   false,
          { aiTextureType_AMBIENT_OCCLUSION, aiTextureType_LIGHTMAP },
          { "AO", "AmbientOcclusion", "Occlusion" } },
    };

    bool IsImageExtension(const std::string& ext) {
        const std::string e = ToLower(ext);
        return e == ".jpg" || e == ".jpeg" || e == ".png" ||
               e == ".tga" || e == ".bmp"  || e == ".psd";
    }
}

ExternalModel::ExternalModel(const std::string& path):
    _path(path)
{
    _name = "ExternalModel";
    _transforms = std::make_shared<std::vector<glm::mat4>>();
}

void ExternalModel::Render(const RenderContext& ctx, MaterialBase* material /*= nullptr*/)
{
    if (_scene) {
        BoneTransform(Engine::GetInstance().GetTimerSec(), _transforms);
    }
    Pivot3D::Render(ctx, material);
}

void ExternalModel::Init()
{
    LoadModel(_path);
}

void ExternalModel::LoadModel(const std::string& path)
{
    unsigned int flags = aiProcess_Triangulate | aiProcess_CalcTangentSpace;
    if (_flipUVs) {
        flags |= aiProcess_FlipUVs;
    }
    _scene = _import.ReadFile(path, flags);

    if (!_scene || _scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !_scene->mRootNode)
	{
        std::cout << "ERROR::ASSIMP::" << _import.GetErrorString() << std::endl;
		return;
	}
    _directory = path.substr(0, path.find_last_of('/'));
    m_GlobalInverseTransform = ToGlm(_scene->mRootNode->mTransformation);
    m_GlobalInverseTransform = glm::inverse(m_GlobalInverseTransform);

    aiMatrix4x4 identity;
    ProcessNode(_scene->mRootNode, _scene, identity);
}

void ExternalModel::ProcessNode(aiNode * node, const aiScene * scene, const aiMatrix4x4& parentTransform)
{
    aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;

    for (GLuint i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        auto m = ProcessMesh(mesh, scene);
        m->SetNodeMatrix(ToGlm(nodeTransform));
        AddChild(m);
    }
    for (GLuint i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene, nodeTransform);
    }
}

std::shared_ptr<Mesh> ExternalModel::ProcessMesh(aiMesh * mesh, const aiScene * scene)
{
    std::vector<VertexTypes::Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<std::shared_ptr<Texture2D>> textures;
    std::vector<VertexTypes::VertexBoneData> bones;
    bool hasBones = mesh->mNumBones > 0;

	for (GLuint i = 0; i < mesh->mNumVertices; i++)
	{
        VertexTypes::Vertex vertex;
		// Process vertex positions, normals and texture coordinates
		glm::vec3 vector;
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.Position = vector;
		vector.x = mesh->mNormals[i].x;
		vector.y = mesh->mNormals[i].y;
		vector.z = mesh->mNormals[i].z;
		vertex.Normal = vector;

		if (mesh->mTextureCoords[0]) // Does the mesh contain texture coordinates?
		{
			glm::vec2 vec;
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vertex.TexCoords = vec;
		}
		else
		{
			vertex.TexCoords = glm::vec2(0.0f, 0.0f);
		}

	    if (mesh->mTangents) {
	        vertex.Tangent.x = mesh->mTangents[i].x;
	        vertex.Tangent.y = mesh->mTangents[i].y;
	        vertex.Tangent.z = mesh->mTangents[i].z;
	    } else {
	        vertex.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);  // fallback
	    }
			
		vertices.push_back(vertex);
	}
	// Process indices
	for (GLuint i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (GLuint j = 0; j < face.mNumIndices; j++) 
		{
			indices.push_back(face.mIndices[j]);
		}
	}
    std::shared_ptr<Material3D> mat;
    if (hasBones)
    {
       mat = std::make_shared<SkinnedMaterial3D>("../assets/shaders/shader_skin.vsh", "../assets/shaders/defaultColorLight.fsh");
       auto skinnedMat = std::dynamic_pointer_cast<SkinnedMaterial3D>(mat);
       skinnedMat->transforms = _transforms;
    }
    else {
        mat = std::make_shared<Material3D>("../assets/shaders/shader.vsh", "../assets/shaders/defaultColorLight.fsh");
        mat->SetLightingModel(std::make_unique<PBRLightingModel>());
    }


    std::string name = "Material ";
    name.append(std::to_string(mat->GetId()));
    mat->SetName(name);

	// Process material
    if (scene->mNumMaterials > 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        aiString aiMatName;
        material->Get(AI_MATKEY_NAME, aiMatName);
        const std::string materialName = aiMatName.C_Str();

        for (const auto& cfg : kSlotConfigs)
        {
            // 1. Try assimp's material texture slots (primary type + legacy fallbacks).
            auto maps = LoadMaterialTextures(material, cfg.assimpTypes, cfg.typeName);
            // 2. Fallback for Megascans/Quixel-style assets whose FBX materials embed no
            //    texture paths: resolve sibling files by their naming convention, scoped to
            //    this material's name so multi-part assets don't pull in every part's maps.
            if (maps.empty()) {
                maps = ResolveTexturesByConvention(cfg.conventionSuffixes, cfg.typeName, materialName);
            }

            // Every slot here is single-valued (the shader assigns, it does not accumulate):
            // a second texture would just overwrite the first, so only the first map is used.
            if (!maps.empty()) {
                const auto& tex = maps.front();
                std::shared_ptr<Filter3D> filter = cfg.normalMap
                    ? std::static_pointer_cast<Filter3D>(std::make_shared<NormalMapFilter>(tex))
                    : std::static_pointer_cast<Filter3D>(std::make_shared<TextureMapFilter>(tex));
                filter->SetSlot(cfg.slot);
                filter->SetBlendMode(cfg.blend);
                mat->AddFilter(filter);
            }
        }
	}

    if (hasBones)
    {
        //process bones
        bones.resize(mesh->mNumVertices);
        LoadBones(0, mesh, bones);
    }

    mat->Build();

    // Ключ кэша: путь к файлу + индекс материала + имя меша (в одном файле много частей).
    const std::string geometryKey = _path + "#" + std::to_string(mesh->mMaterialIndex)
                                   + "#" + std::string(mesh->mName.C_Str());
    auto geometry = Engine::GetInstance().GetGeometryRegistry().GetOrCreate(
        geometryKey,
        [&] {
            Geometry g(VertexLayouts::Standard(), vertices, indices);
            if (!bones.empty()) {
                g.AddSecondaryBuffer(VertexLayouts::Skinning(), std::as_bytes(std::span(bones)));
            }
            return g;
        });

    auto m = std::make_shared<Mesh>(geometry, mat);
    m->SetName(mesh->mName.C_Str());
    return m;
}

// Checks all material textures of a given type and loads the textures if they're not loaded yet.
// The required info is returned as a Texture struct.
std::vector<std::shared_ptr<Texture2D>> ExternalModel::LoadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName)
{
    std::vector<std::shared_ptr<Texture2D>> textures;
	for (GLuint i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString texturePath;
		mat->GetTexture(type, i, &texturePath);
		const std::string normalized = NormalizeTexturePath(texturePath.C_Str());
        auto texture = Engine::GetInstance().GetTextureManager()->getTexture(normalized, typeName, _directory);
		// id == 0 means SOIL failed to load the file (e.g. FBX referencing a non-existent
		// `<model>.fbm/...` embedded-media path). Treat it as "no texture" so the caller's
		// convention-based fallback can resolve the real sibling file instead.
		if (texture != nullptr && texture->id != 0) {
			textures.push_back(texture);
		}
	}
	return textures;
}

std::vector<std::shared_ptr<Texture2D>> ExternalModel::LoadMaterialTextures(aiMaterial* mat, const std::vector<aiTextureType>& types, const std::string& typeName)
{
    for (const auto type : types) {
        auto textures = LoadMaterialTextures(mat, type, typeName);
        if (!textures.empty()) {
            return textures;
        }
    }
    return {};
}

std::vector<std::shared_ptr<Texture2D>> ExternalModel::ResolveTexturesByConvention(const std::vector<std::string>& suffixes, const std::string& typeName, const std::string& materialName)
{
    std::vector<std::shared_ptr<Texture2D>> textures;

    namespace fs = std::filesystem;
    std::error_code ec;
    if (_directory.empty() || !fs::is_directory(_directory, ec)) {
        return textures;
    }

    const std::string matLower = ToLower(materialName);

    // Priority order over suffixes: the first suffix that yields a match wins
    // (so a BaseColor map beats a Diffuse map when both exist).
    for (const auto& suffix : suffixes) {
        const std::string token = "_" + ToLower(suffix);

        // Within a suffix, prefer a file scoped to this material (`<materialName>_<suffix>`)
        // over a loose `*_<suffix>` match. On multi-part assets this picks the right part's
        // map instead of dumping every part's map into one material.
        fs::path scopedMatch;
        fs::path looseMatch;

        for (const auto& entry : fs::directory_iterator(_directory, ec)) {
            if (ec) break;
            if (!entry.is_regular_file(ec)) continue;

            const auto& path = entry.path();
            if (!IsImageExtension(path.extension().string())) continue;

            const std::string stem = ToLower(path.stem().string());
            if (stem.size() < token.size()) continue;
            if (stem.compare(stem.size() - token.size(), token.size(), token) != 0) continue;

            if (!matLower.empty() && stem.rfind(matLower, 0) == 0) {
                scopedMatch = path;
                break; // exact owner found, no need to look further
            }
            if (looseMatch.empty()) {
                looseMatch = path;
            }
        }

        const fs::path& chosen = !scopedMatch.empty() ? scopedMatch : looseMatch;
        if (!chosen.empty()) {
            const std::string filename = chosen.filename().string();
            auto texture = Engine::GetInstance().GetTextureManager()->getTexture(filename, typeName, _directory);
            if (texture != nullptr) {
                textures.push_back(texture);
            }
            break;
        }
    }
    return textures;
}

void ExternalModel::AddBoneData(VertexTypes::VertexBoneData& data, unsigned int boneID, float weight)
{
    auto size = std::size(data.IDs);
    for (unsigned int i = 0 ; i < size ; i++) {
        if (data.Weights[i] == 0.0) {
            data.IDs[i] = boneID;
            data.Weights[i] = weight;
            return;
        }
    }

    // should never get here - more bones than we have space for
    assert(0);
}

void ExternalModel::LoadBones(unsigned int /*MeshIndex*/, const aiMesh* pMesh, std::vector<VertexTypes::VertexBoneData>& bones)
{
    for (unsigned int i = 0 ; i < pMesh->mNumBones; i++) {
        unsigned int boneIndex = 0;
        std::string boneName(pMesh->mBones[i]->mName.data);

        if (_boneMapping.find(boneName) == _boneMapping.end()) {
            boneIndex = _numBones;
            _numBones++;
            BoneInfo boneInfo;
            _boneInfos.push_back(boneInfo);
        }
        else {
            boneIndex = _boneMapping[boneName];
        }

        _boneMapping[boneName] = boneIndex;
        _boneInfos[boneIndex].BoneOffset = ToGlm(pMesh->mBones[i]->mOffsetMatrix);

        for (unsigned int j = 0 ; j < pMesh->mBones[i]->mNumWeights ; j++) {
            //m_Entries? BaseVertex
            //http://ogldev.atspace.co.uk/www/tutorial32/tutorial32.html
            //unsigned int vertexID = m_Entries[MeshIndex].BaseVertex + pMesh->mBones[i]->mWeights[j].mVertexId;
            unsigned int vertexID = pMesh->mBones[i]->mWeights[j].mVertexId;
            float weight = pMesh->mBones[i]->mWeights[j].mWeight;
            AddBoneData(bones[vertexID], boneIndex, weight);
        }
    }
}

void ExternalModel::BoneTransform(float timeSec, const std::shared_ptr<std::vector<glm::mat4>>& transforms)
{
    if (_scene->mNumAnimations == 0) {
        return;
    }
    glm::mat4 identity(1.0f);

    float ticksPerSecond = _scene->mAnimations[0]->mTicksPerSecond != 0 ?
                            _scene->mAnimations[0]->mTicksPerSecond : 25.0f;

    float timeInTicks = timeSec * ticksPerSecond;
    float animationTime = fmod(timeInTicks, _scene->mAnimations[0]->mDuration);

    ReadNodeHierarchy(animationTime, _scene->mRootNode, identity);

    transforms->resize(_numBones);

    for (unsigned int i = 0; i < _numBones; i++) {
        transforms->operator [](i) = _boneInfos[i].FinalTransformation;
    }
}

void ExternalModel::ReadNodeHierarchy(float t, const aiNode* pNode, const glm::mat4& parentTransform)
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
        _boneInfos[boneIndex].FinalTransformation = m_GlobalInverseTransform * globalTransformation *
                                                    _boneInfos[boneIndex].BoneOffset;
    }

    for (unsigned int i = 0 ; i < pNode->mNumChildren ; i++) {
        ReadNodeHierarchy(t, pNode->mChildren[i], globalTransformation);
    }
}

unsigned int ExternalModel::FindRotation(float t, const aiNodeAnim* pNodeAnim)
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

const aiNodeAnim* ExternalModel::FindNodeAnim(const aiAnimation* pAnimation, const std::string& nodeName)
{
    for (unsigned int i = 0 ; i < pAnimation->mNumChannels ; i++) {
        const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

        if (std::string(pNodeAnim->mNodeName.data) == nodeName) {
            return pNodeAnim;
        }
    }

    return nullptr;
}

glm::quat ExternalModel::CalcInterpolatedRotation(float t, const aiNodeAnim* pNodeAnim)
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

glm::vec3 ExternalModel::CalcInterpolatedScaling(float t, const aiNodeAnim* pNodeAnim)
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

glm::vec3 ExternalModel::CalcInterpolatedPosition(float t, const aiNodeAnim* pNodeAnim)
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

unsigned int ExternalModel::FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
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

unsigned int ExternalModel::FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    for (unsigned int i = 0 ; i < pNodeAnim->mNumPositionKeys - 1 ; i++) {
        if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
            return i;
        }
    }

    assert(0);

    return 0;
}


