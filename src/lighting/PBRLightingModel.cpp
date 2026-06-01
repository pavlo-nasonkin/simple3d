#include "PBRLightingModel.h"
#include <string>
#include "Scene3D.h"
#include "camera/Camera.h"

const std::string PBRLightingModel::_declarationsCode = R"(
const float PI = 3.14159265359;

struct DirLight { vec3 direction; vec3 color; };

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform vec3 ambientLight; // плоский заполняющий свет окружения (до IBL)

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
    vec3 L = normalize(-dirLight.direction);
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

    vec3 radiance = dirLight.color;
    vec3 Lo = (diffuse + specular) * radiance * NdotL;

    // Плоский ambient как заглушка до IBL. Множитель (1 - M) убирает диффузный
    // ambient у металлов (у них нет диффуза) — иначе металл «светится» базовым цветом.
    vec3 ambient = ambientLight * BASE_COLOR * (1.0 - M) * AO;
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

    // Directional light (sun): direction + radiance (color * intensity), без attenuation.
    const glm::vec3* dir = ctx.scene3D->GetDirLightDirection();
    const glm::vec3 radiance = (*ctx.scene3D->GetDirLightColor()) * ctx.scene3D->GetDirLightIntensity();
    glUniform3f(_uniformCache.GetUniformLocation("dirLight.direction"), dir->x, dir->y, dir->z);
    glUniform3f(_uniformCache.GetUniformLocation("dirLight.color"), radiance.x, radiance.y, radiance.z);

    // Плоский ambient окружения (тот же, что использует Phong), независим от солнца.
    const glm::vec3* ambient = ctx.scene3D->GetLightAmbient();
    glUniform3f(_uniformCache.GetUniformLocation("ambientLight"), ambient->x, ambient->y, ambient->z);
}

void PBRLightingModel::OnProgramBuild(GLuint program) {
    ILightingModel::OnProgramBuild(program);

    _uniformCache.Reset(program);
}
