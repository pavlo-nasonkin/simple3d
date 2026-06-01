#include "PBRLightingModel.h"
#include <string>
#include <glm/gtc/type_ptr.hpp>
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

// учитывает roughness — для ambient/IBL (на шероховатых поверхностях fresnel слабее)
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

uniform bool        uHasIBL;
uniform samplerCube irradianceMap;   // diffuse IBL
uniform samplerCube prefilterMap;    // specular IBL (mip = roughness)
uniform sampler2D   brdfLUT;         // scale/bias для F0
uniform float       uMaxReflectionLod;

uniform bool      uHasShadows;
uniform sampler2D shadowMap;
uniform mat4      uLightSpaceMatrix;
uniform float     uShadowStrength;  // насколько тень дополнительно гасит ambient/IBL (0..1)

// Доля затенения [0..1] для directional света. PCF 3x3 + slope-scaled bias.
float ShadowFactor(vec3 fragPos, vec3 N, vec3 L) {
    vec4 fragPosLS = uLightSpaceMatrix * vec4(fragPos, 1.0);
    vec3 proj = fragPosLS.xyz / fragPosLS.w;
    proj = proj * 0.5 + 0.5;               // [-1,1] -> [0,1]
    if (proj.z > 1.0) return 0.0;          // за far-плоскостью света тени нет

    float bias = max(0.0015 * (1.0 - dot(N, L)), 0.0005);
    float currentDepth = proj.z;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, proj.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
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
    // Тень гасит только прямой свет; ambient/IBL не затрагивается.
    float shadow = uHasShadows ? ShadowFactor(FragPos, N, L) : 0.0;
    vec3 Lo = (diffuse + specular) * radiance * NdotL * (1.0 - shadow);

    vec3 ambient;
    if (uHasIBL) {
        // Indirect lighting из окружения: diffuse irradiance + specular reflection.
        vec3 Famb = FresnelSchlickRoughness(NdotV, F0, R);
        vec3 kDamb = (vec3(1.0) - Famb) * (1.0 - M);

        vec3 irradiance = texture(irradianceMap, N).rgb;
        vec3 diffuseIBL = irradiance * BASE_COLOR;

        vec3 Rrefl = reflect(-V, N);
        vec3 prefiltered = textureLod(prefilterMap, Rrefl, R * uMaxReflectionLod).rgb;
        vec2 brdf = texture(brdfLUT, vec2(NdotV, R)).rg;
        vec3 specularIBL = prefiltered * (Famb * brdf.x + brdf.y);

        ambient = (kDamb * diffuseIBL + specularIBL) * AO;
    } else {
        // Плоский ambient-заглушка. (1 - M) убирает диффузный ambient у металлов.
        ambient = ambientLight * BASE_COLOR * (1.0 - M) * AO;
    }
    // Тень дополнительно притеняет ambient (контраст). uShadowStrength=0 — выкл.
    ambient *= (1.0 - shadow * uShadowStrength);
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

    // Плоский ambient окружения (fallback, когда IBL нет), независим от солнца.
    const glm::vec3* ambient = ctx.scene3D->GetLightAmbient();
    glUniform3f(_uniformCache.GetUniformLocation("ambientLight"), ambient->x, ambient->y, ambient->z);

    // IBL: irradiance + prefiltered specular + BRDF LUT на юнитах после фильтров материала.
    const SceneEnvironment& env = ctx.scene3D->GetEnvironment();
    const bool hasIBL = env.IsValid();
    glUniform1i(_uniformCache.GetUniformLocation("uHasIBL"), hasIBL ? 1 : 0);

    const GLuint irradianceUnit = firstTextureUnit;
    const GLuint prefilterUnit  = firstTextureUnit + 1;
    const GLuint brdfUnit       = firstTextureUnit + 2;
    // Назначаем сэмплерам уникальные юниты всегда (чтобы не алиаситься на юнит 0 фильтра).
    glUniform1i(_uniformCache.GetUniformLocation("irradianceMap"), static_cast<GLint>(irradianceUnit));
    glUniform1i(_uniformCache.GetUniformLocation("prefilterMap"),  static_cast<GLint>(prefilterUnit));
    glUniform1i(_uniformCache.GetUniformLocation("brdfLUT"),       static_cast<GLint>(brdfUnit));

    if (hasIBL) {
        env.Irradiance()->Bind(irradianceUnit);
        env.PrefilteredSpec()->Bind(prefilterUnit);
        glActiveTexture(GL_TEXTURE0 + brdfUnit);
        glBindTexture(GL_TEXTURE_2D, env.BrdfLUT()->id);
        glUniform1f(_uniformCache.GetUniformLocation("uMaxReflectionLod"), 4.0f); // mipLevels - 1
    }

    // Shadow map: тень применяется только если карта есть И узел её принимает.
    const bool useShadows = ctx.hasShadows && ctx.receiveShadows;
    const GLuint shadowUnit = firstTextureUnit + 3;
    glUniform1i(_uniformCache.GetUniformLocation("shadowMap"), static_cast<GLint>(shadowUnit));
    glUniform1i(_uniformCache.GetUniformLocation("uHasShadows"), useShadows ? 1 : 0);
    glUniform1f(_uniformCache.GetUniformLocation("uShadowStrength"), ctx.scene3D->GetShadowStrength());
    if (useShadows) {
        glUniformMatrix4fv(_uniformCache.GetUniformLocation("uLightSpaceMatrix"),
                           1, GL_FALSE, glm::value_ptr(ctx.lightSpaceMatrix));
        glActiveTexture(GL_TEXTURE0 + shadowUnit);
        glBindTexture(GL_TEXTURE_2D, ctx.shadowMap);
    }
}

void PBRLightingModel::OnProgramBuild(GLuint program) {
    ILightingModel::OnProgramBuild(program);

    _uniformCache.Reset(program);
}
