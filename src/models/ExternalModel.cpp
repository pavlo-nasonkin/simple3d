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
#include "materials/filters/NormalMapFilter.h"
#include "utils/Math3d.h"

ExternalModel::ExternalModel(const std::string& path):
    _path(path)
{
    _name = "ExternalModel";
    _transforms = std::make_shared<std::vector<Matrix4f>>();
}

void ExternalModel::Render(const RenderContext& ctx, MaterialBase* material /*= nullptr*/)
{
    if (_scene) {
        BoneTransform(Engine::GetInstance().GetTimerSec(), _transforms);
    }
    Pivot3D::Render(ctx, material);
}

ExternalModel::~ExternalModel()
{

}

void ExternalModel::Init()
{
    LoadModel(_path);
}

void ExternalModel::LoadModel(const std::string& path)
{
    _scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!_scene || _scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !_scene->mRootNode)
	{
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
		return;
	}
    _directory = path.substr(0, path.find_last_of('/'));
    m_GlobalInverseTransform = _scene->mRootNode->mTransformation;
    m_GlobalInverseTransform.Inverse();

    ProcessNode(_scene->mRootNode, _scene);
}

void ExternalModel::ProcessNode(aiNode * node, const aiScene * scene)
{
    // Process all the node's meshes (if any)
    for (GLuint i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        AddChild(ProcessMesh(mesh, scene));
    }
    // Then do the same for each of its children
    for (GLuint i = 0; i < node->mNumChildren; i++)
    {
        this->ProcessNode(node->mChildren[i], scene);
    }
}

std::shared_ptr<Mesh> ExternalModel::ProcessMesh(aiMesh * mesh, const aiScene * scene)
{
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<std::shared_ptr<Texture2D>> textures;
    std::vector<VertexBoneData> bones;
    bool hasBones = mesh->mNumBones > 0;

	for (GLuint i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;
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
       mat = std::make_shared<SkinnedMaterial3D>("../assets/shaders/shader_skin.vs", "../assets/shaders/defaultColorLight.fs");
       auto skinnedMat = std::dynamic_pointer_cast<SkinnedMaterial3D>(mat);
       skinnedMat->transforms = _transforms;
    }
    else {
        mat = std::make_shared<Material3D>("../assets/shaders/shader.vs", "../assets/shaders/defaultColorLight.fs");
    }


    std::string name = "Material ";
    name.append(std::to_string(mat->GetId()));
    mat->SetName(name);

	// Process material
    if (scene->mNumMaterials > 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        static const std::string kTextureDiffuse = "texture_diffuse";
        auto diffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE, kTextureDiffuse);

        for (const auto& tex : diffuseMaps)
        {
            auto textureFilter = std::make_shared<TextureMapFilter>(tex);
            textureFilter->SetSlot(Filter3D::FilterSlot::BaseColor);
            textureFilter->SetBlendMode(Filter3D::BlendMode::MULTIPLY);
            mat->AddFilter(textureFilter);
        }

        static const std::string kTextureSpecular = "texture_specular";
        auto specularMaps = this->LoadMaterialTextures(material, aiTextureType_SPECULAR, kTextureSpecular);
        for (const auto& tex : specularMaps) {
            auto specFilter = std::make_shared<TextureMapFilter>(tex);
            specFilter->SetSlot(Filter3D::FilterSlot::Specular);
            specFilter->SetBlendMode(Filter3D::BlendMode::MULTIPLY);
            mat->AddFilter(specFilter);
        }

        static const std::string kTextureNormal = "texture_normal";
        auto normalMaps = this->LoadMaterialTextures(material, aiTextureType_NORMALS, kTextureNormal);
        for (const auto& tex : normalMaps) {
            auto normalFilter = std::make_shared<NormalMapFilter>(tex);
            normalFilter->SetSlot(Filter3D::FilterSlot::Normal);
            normalFilter->SetBlendMode(Filter3D::BlendMode::NORMAL);
            mat->AddFilter(normalFilter);
        }
	}

    if (hasBones)
    {
        //process bones
        bones.resize(mesh->mNumVertices);
        LoadBones(0, mesh, bones);
    }

    mat->Build();
    auto m = std::make_shared<Mesh>(vertices, indices, mat, bones);
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
        auto texture = Engine::GetInstance().GetTextureManager()->getTexture(texturePath.C_Str(), typeName, _directory);
		if (texture != nullptr) {
			textures.push_back(texture);
		}
	}
	return textures;
}

void ExternalModel::AddBoneData(VertexBoneData& data, unsigned int boneID, float weight)
{
    auto size = sizeof(data.IDs)/sizeof(data.IDs[0]);
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

void ExternalModel::LoadBones(unsigned int /*MeshIndex*/, const aiMesh* pMesh, std::vector<VertexBoneData>& bones)
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
        _boneInfos[boneIndex].BoneOffset = pMesh->mBones[i]->mOffsetMatrix;

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

void ExternalModel::BoneTransform(float TimeInSeconds, std::shared_ptr<std::vector<Matrix4f>> transforms)
{
    if (_scene->mNumAnimations == 0) {
        return;
    }
    Matrix4f identity;
    identity.InitIdentity();

    float TicksPerSecond = _scene->mAnimations[0]->mTicksPerSecond != 0 ?
                            _scene->mAnimations[0]->mTicksPerSecond : 25.0f;

    float TimeInTicks = TimeInSeconds * TicksPerSecond;
    float AnimationTime = fmod(TimeInTicks, _scene->mAnimations[0]->mDuration);

    ReadNodeHeirarchy(AnimationTime, _scene->mRootNode, identity);

    transforms->resize(_numBones);

    for (unsigned int i = 0; i < _numBones; i++) {
        transforms->operator [](i) = _boneInfos[i].FinalTransformation;
    }
}

void ExternalModel::ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, const Matrix4f& ParentTransform)
{
    if (_scene->mNumAnimations == 0) {
        return;
    }
    std::string NodeName(pNode->mName.data);

    const aiAnimation* pAnimation = _scene->mAnimations[0];

    Matrix4f NodeTransformation(pNode->mTransformation);

    const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);

    if (pNodeAnim) {
        // Interpolate scaling and generate scaling transformation matrix
        aiVector3D Scaling;
        CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);
        Matrix4f ScalingM;
        ScalingM.InitScaleTransform(Scaling.x, Scaling.y, Scaling.z);

        // Interpolate rotation and generate rotation transformation matrix
        aiQuaternion RotationQ;
        CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);
        Matrix4f RotationM = aiMatrix4x4(RotationQ.GetMatrix());

        // Interpolate translation and generate translation transformation matrix
        aiVector3D Translation;
        CalcInterpolatedPosition(Translation, AnimationTime, pNodeAnim);
        Matrix4f TranslationM;
//        TranslationM.InitIdentity();
        TranslationM.InitTranslationTransform(Translation.x, Translation.y, Translation.z);

        // Combine the above transformations
        NodeTransformation = TranslationM * RotationM * ScalingM;
    }

    Matrix4f GlobalTransformation = ParentTransform * NodeTransformation;

    if (_boneMapping.find(NodeName) != _boneMapping.end()) {
        unsigned int BoneIndex = _boneMapping[NodeName];
        _boneInfos[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation *
                                                    _boneInfos[BoneIndex].BoneOffset;
    }

    for (unsigned int i = 0 ; i < pNode->mNumChildren ; i++) {
        ReadNodeHeirarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
    }
}

void ExternalModel::CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumRotationKeys == 1) {
        Out = pNodeAnim->mRotationKeys[0].mValue;
        return;
    }

    unsigned int RotationIndex = FindRotation(AnimationTime, pNodeAnim);
    unsigned int NextRotationIndex = (RotationIndex + 1);
    assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
    float DeltaTime = pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime;
    float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
    const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
    aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
    Out = Out.Normalize();
}

unsigned int ExternalModel::FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumRotationKeys > 0);

    for (unsigned int i = 0 ; i < pNodeAnim->mNumRotationKeys - 1 ; i++) {
        if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
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

void ExternalModel::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumScalingKeys == 1) {
        Out = pNodeAnim->mScalingKeys[0].mValue;
        return;
    }

    unsigned int ScalingIndex = FindScaling(AnimationTime, pNodeAnim);
    unsigned int NextScalingIndex = (ScalingIndex + 1);
    assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
    float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
    float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
    const aiVector3D& End   = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
    aiVector3D Delta = End - Start;
    Out = Start + Factor * Delta;
}

void ExternalModel::CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumPositionKeys == 1) {
        Out = pNodeAnim->mPositionKeys[0].mValue;
        return;
    }

    unsigned int PositionIndex = FindPosition(AnimationTime, pNodeAnim);
    unsigned int NextPositionIndex = (PositionIndex + 1);
    assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
    float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
    float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
    const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
    aiVector3D Delta = End - Start;
    Out = Start + Factor * Delta;
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


