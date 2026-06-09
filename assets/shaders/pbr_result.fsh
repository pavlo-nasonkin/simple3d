#version 330 core

out vec4 color;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords0;
in vec2 TexCoords1;
in mat3 TBN;
in vec4 Color;

uniform vec4 uBaseColor;


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


uniform sampler2D uSampler0;
vec4 textureMapFilter0()
{
    return texture(uSampler0, TexCoords0);
}

uniform sampler2D uSampler1;
vec4 textureMapFilter1()
{
    return texture(uSampler1, TexCoords0);
}

uniform sampler2D uSampler2;
vec3 normalMapFilter2()
{
    vec3 ts = texture(uSampler2, TexCoords0).rgb * 2.0 - 1.0;
    return normalize(TBN * ts);
}

uniform sampler2D uSampler3;
vec4 textureMapFilter3()
{
    return texture(uSampler3, TexCoords0);
}

uniform sampler2D uSampler4;
vec4 textureMapFilter4()
{
    return texture(uSampler4, TexCoords0);
}

uniform sampler2D uSampler5;
vec4 textureMapFilter5()
{
    return texture(uSampler5, TexCoords0);
}

// __FRAGMENT_DECLS__

void main()
{
    vec3 BASE_COLOR    = uBaseColor.rgb;
    vec3 SPEC_STRENGTH = vec3(1.0);
    vec3 N             = normalize(Normal);
    vec3 EMISSIVE      = vec3(0.0);

    float M  = 0.0;
    float R  = 0.5;
    float AO = 1.0;

    BASE_COLOR *= textureMapFilter0().rgb;
    // __APPLY_BASE_COLOR_FILTERS__
    SPEC_STRENGTH *= textureMapFilter1().rgb;
    // __APPLY_SPECULAR_FILTERS__
    N = normalMapFilter2();
    // __APPLY_NORMAL_FILTERS__

    M = textureMapFilter3().r;
    // __APPLY_METALLIC_FILTERS__
    R = textureMapFilter4().r;
    // __APPLY_ROUGHNESS_FILTERS__
    AO = textureMapFilter5().r;
    // __APPLY_AO_FILTERS__


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




    // __APPLY_OVERLAY_FILTERS__
}