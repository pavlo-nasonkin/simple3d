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
#include "render/MeshRenderer.h"
#include "utils/AssimpGlm.h"
#include "utils/AssetPaths.h"
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

    // Скелетные данные собраны в _boneMapping/_boneInfos. Инициализируем буфер
    // костей единичными матрицами (bind pose), а проигрывание анимации выносим в
    // AnimatorBehaviour (тик в OnUpdate вместо логики в Render).
    if (_numBones > 0) {
        _transforms->assign(_numBones, glm::mat4(1.0f));
        if (_scene->mNumAnimations > 0) {
            auto* animator = AddBehaviour<AnimatorBehaviour>();
            animator->Configure(_scene, m_GlobalInverseTransform,
                                std::move(_boneMapping), std::move(_boneInfos),
                                _transforms);
        }
    }
}

void ExternalModel::ProcessNode(aiNode * node, const aiScene * scene, const aiMatrix4x4& parentTransform)
{
    aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;

    for (GLuint i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        auto m = ProcessMesh(mesh, scene);
        m->GetRenderer()->SetNodeMatrix(ToGlm(nodeTransform));
        AddChild(m);
    }
    for (GLuint i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene, nodeTransform);
    }
}

std::shared_ptr<Pivot3D> ExternalModel::ProcessMesh(aiMesh * mesh, const aiScene * scene)
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
       mat = std::make_shared<SkinnedMaterial3D>(AssetPaths::Resolve("shaders/shader_skin.vsh"), AssetPaths::Resolve("shaders/defaultColorLight.fsh"));
       auto skinnedMat = std::dynamic_pointer_cast<SkinnedMaterial3D>(mat);
       skinnedMat->transforms = _transforms;
    }
    else {
        mat = std::make_shared<Material3D>(AssetPaths::Resolve("shaders/shader.vsh"), AssetPaths::Resolve("shaders/defaultColorLight.fsh"));
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

    return MeshRenderer::MakeNode(geometry, mat, mesh->mName.C_Str());
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



