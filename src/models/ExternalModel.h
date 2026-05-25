#ifndef EXTERNAL_MODEL_H
#define EXTERNAL_MODEL_H

// GL Includes
#include <GL/glew.h> // Contains all the necessery OpenGL includes
#include <glm/glm.hpp>
//Assimp
#include <assimp/scene.h>

#include "../Shader.h"
#include "Mesh.h"
#include <SOIL/SOIL2.h>
#include "../resources/Texture2D.h"
#include "../Pivot3D.h"
#include "../materials/MaterialBase.h"
#include <memory>
#include <map>
#include "../utils/Math3d.h"
#include "../materials/SkinnedMaterial3D.h"
#include <assimp/Importer.hpp>

struct BoneInfo
{
    Matrix4f BoneOffset;
    Matrix4f FinalTransformation;
};

class ExternalModel: public Pivot3D
{
public:
	/*  Functions   */
    ExternalModel(const std::string& path);
    void Render(const RenderContext &ctx, MaterialBase* material) override;
	~ExternalModel() override;
    void Init() override;

private:
	/*  Model Data  */
    const aiScene* _scene;
    std::string _directory;
    std::string _path;
    Matrix4f m_GlobalInverseTransform;
    std::shared_ptr<std::vector<Matrix4f>> _transforms;
    unsigned int _numBones = 0;
    std::map<std::string, int> _boneMapping;
    std::vector<BoneInfo> _boneInfos;
	/*  Functions   */
    void loadModel(std::string path);
	void processNode(aiNode* node, const aiScene* scene);
    std::shared_ptr<Mesh> processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<std::shared_ptr<Texture2D>> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
    //skinned mesh

    Assimp::Importer import;


    void BoneTransform(float TimeInSeconds, std::shared_ptr<std::vector<Matrix4f>> transforms);
    void ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, const Matrix4f& ParentTransform);
    void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    unsigned int FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
    void loadBones(unsigned int MeshIndex, const aiMesh* pMesh, std::vector<VertexBoneData>& bones);
    const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const std::string& nodeName);
    void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    unsigned int FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
    unsigned int FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
    void addBoneData(VertexBoneData& data, unsigned int boneID, float weight);


};



#endif//EXTERNAL_MODEL_H
