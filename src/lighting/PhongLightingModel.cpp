#include "PhongLightingModel.h"

#include "Scene3D.h"
#include "camera/Camera.h"

const std::string PhongLightingModel::_declarationsCode =
    "struct Material { float shininess; };\n"
    "struct Light { vec3 position; vec3 ambient; vec3 diffuse; vec3 specular; };\n"
    "uniform vec3 viewPos;\n"
    "uniform Material material;\n"
    "uniform Light light;\n";

const std::string PhongLightingModel::_lightingCode =
    "    vec3 lightDir = normalize(light.position - FragPos);\n"
    "    float diff    = max(dot(N, lightDir), 0.0);\n"
    "\n"
    "    vec3 ambient  = light.ambient  * BASE_COLOR;\n"
    "    vec3 diffuse  = light.diffuse  * diff * BASE_COLOR;\n"
    "\n"
    "    vec3 viewDir    = normalize(viewPos - FragPos);\n"
    "    vec3 reflectDir = reflect(-lightDir, N);\n"
    "    float spec      = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);\n"
    "    vec3 specular   = light.specular * spec * SPEC_STRENGTH;\n"
    "\n"
    "    color = vec4(ambient + diffuse + specular + EMISSIVE, 1.0);\n";

const std::string& PhongLightingModel::GetDeclarations() const
{
    return _declarationsCode;
}

const std::string & PhongLightingModel::GetLightingCode() const {
    return _lightingCode;
}

void PhongLightingModel::Bind(GLuint firstTextureUnit, const RenderContext &ctx) {
    GLint viewPosLoc = _uniformCache.GetUniformLocation("viewPos");
    glUniform3f(viewPosLoc, ctx.camera->Position.x, ctx.camera->Position.y, ctx.camera->Position.z);

    GLint lightPositionLoc = _uniformCache.GetUniformLocation("light.position");
    GLint lightAmbientLoc = _uniformCache.GetUniformLocation("light.ambient");
    GLint lightDiffuseLoc = _uniformCache.GetUniformLocation("light.diffuse");
    GLint lightSpecularLoc = _uniformCache.GetUniformLocation("light.specular");
    const glm::vec3* lightAmbient = ctx.scene3D->getLightAmbient();
    const glm::vec3* lightDiffuse = ctx.scene3D->getLightDiffuse();
    const glm::vec3* lightSpecular = ctx.scene3D->getLightSpecular();
    const glm::vec3* lightPos = ctx.scene3D->getLightPosition();

    glUniform3f(lightAmbientLoc, lightAmbient->x, lightAmbient->y, lightAmbient->z);
    glUniform3f(lightDiffuseLoc, lightDiffuse->x, lightDiffuse->y, lightDiffuse->z);
    glUniform3f(lightSpecularLoc, lightSpecular->x, lightSpecular->y, lightSpecular->z);
    glUniform3f(lightPositionLoc, lightPos->x, lightPos->y, lightPos->z);

    // Also set each mesh's shininess property to a default value (if you want you could extend this to another mesh property and possibly change this value)
    glUniform1f(_uniformCache.GetUniformLocation("material.shininess"), 16.0f);
}

void PhongLightingModel::OnProgramBuild(GLuint program) {
    ILightingModel::OnProgramBuild(program);

    _uniformCache.Reset(program);
}
