#pragma once

// GL Includes
#include <GL/glew.h>
#include <glm/glm.hpp>
//Assimp
#include <assimp/scene.h>

#include "Mesh.h"
#include "resources/Texture2D.h"
#include "Pivot3D.h"
#include "materials/MaterialBase.h"
#include <memory>
#include <map>
#include <assimp/Importer.hpp>

namespace VertexTypes {
	struct VertexBoneData;
}

struct BoneInfo
{
    glm::mat4 BoneOffset = glm::mat4(1.0f);
    glm::mat4 FinalTransformation = glm::mat4(1.0f);
};

class ExternalModel: public Pivot3D
{
public:
    explicit ExternalModel(const std::string& path);
    void Render(const RenderContext &ctx, MaterialBase* material) override;
	~ExternalModel() override = default;
    void Init() override;

private:

	void LoadModel(const std::string& path);
	void ProcessNode(aiNode* node, const aiScene* scene, const aiMatrix4x4& parentTransform);
	std::shared_ptr<Mesh> ProcessMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<std::shared_ptr<Texture2D>> LoadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName);
	void BoneTransform(float timeSec, const std::shared_ptr<std::vector<glm::mat4>>& transforms);
	void ReadNodeHierarchy(float t, const aiNode* pNode, const glm::mat4& parentTransform);

	unsigned int FindRotation(float t, const aiNodeAnim* pNodeAnim);
	void LoadBones(unsigned int meshIndex, const aiMesh* pMesh, std::vector<VertexTypes::VertexBoneData>& bones);
	const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const std::string& nodeName);
	unsigned int FindScaling(float t, const aiNodeAnim* pNodeAnim);
	unsigned int FindPosition(float t, const aiNodeAnim* pNodeAnim);

	glm::quat CalcInterpolatedRotation(float t, const aiNodeAnim* pNodeAnim);
	glm::vec3 CalcInterpolatedScaling(float t, const aiNodeAnim* pNodeAnim);
	glm::vec3 CalcInterpolatedPosition(float t, const aiNodeAnim* pNodeAnim);

	void AddBoneData(VertexTypes::VertexBoneData& data, unsigned int boneID, float weight);

	/*  Model Data  */
    const aiScene* _scene;
    std::string _directory;
    std::string _path;
    glm::mat4 m_GlobalInverseTransform;
    std::shared_ptr<std::vector<glm::mat4>> _transforms;
    unsigned int _numBones = 0;
    std::map<std::string, unsigned int> _boneMapping;
    std::vector<BoneInfo> _boneInfos;

    Assimp::Importer _import;
};