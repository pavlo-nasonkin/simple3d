#include "PBRLightingModel.h"
#include <string>
#include "Scene3D.h"
#include "camera/Camera.h"

const std::string PBRLightingModel::_declarationsCode = R"(
const float PI = 3.14159265359;

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform vec3 viewPos;
uniform Light light;

float DistributionGGX(float NdotH, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

float GeometrySchlickGGX(float NdotX, float k) {
    return NdotX / (NdotX * (1.0 - k) + k);
}

float GeometrySmith(float NdotV, float NdotL, float roughness) {
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;  // k для direct
    return GeometrySchlickGGX(NdotV, k) * GeometrySchlickGGX(NdotL, k);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
)";

const std::string PBRLightingModel::_lightingCode = R"(
    vec3 V = normalize(viewPos - FragPos);
    vec3 L = normalize(light.position - FragPos);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    vec3 F0 = mix(vec3(0.04), BASE_COLOR, M);

    float D = DistributionGGX(NdotH, R);
    float G = GeometrySmith(NdotV, NdotL, R);
    vec3  F = FresnelSchlick(HdotV, F0);

    vec3 specular = (D * G * F) / (4.0 * NdotV * NdotL + 0.0001);

    vec3 kD = (vec3(1.0) - F) * (1.0 - M);
    vec3 diffuse = kD * BASE_COLOR / PI;

    vec3 radiance = light.diffuse;
    vec3 Lo = (diffuse + specular) * radiance * NdotL;

    vec3 ambient = light.ambient * BASE_COLOR * AO;
    vec3 outColor = ambient + Lo + EMISSIVE;

    outColor = outColor / (outColor + vec3(1.0));   // Reinhard
    outColor = pow(outColor, vec3(1.0 / 2.2));        // gamma
    color = vec4(outColor, 1.0);

)";

const std::string& PBRLightingModel::GetDeclarations() const
{
    return _declarationsCode;
}

const std::string & PBRLightingModel::GetLightingCode() const {
    return _lightingCode;
}

void PBRLightingModel::Bind(GLuint firstTextureUnit, const RenderContext &ctx) {
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

void PBRLightingModel::OnProgramBuild(GLuint program) {
    ILightingModel::OnProgramBuild(program);

    _uniformCache.Reset(program);
}
