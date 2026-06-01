#version 330 core

out vec4 color;

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords0;
in vec2 TexCoords1;
in mat3 TBN;
in vec4 Color;

uniform vec4 uBaseColor;

// __LIGHTING_DECLS__
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

    // __APPLY_BASE_COLOR_FILTERS__
    // __APPLY_SPECULAR_FILTERS__
    // __APPLY_NORMAL_FILTERS__

    // __APPLY_METALLIC_FILTERS__
    // __APPLY_ROUGHNESS_FILTERS__
    // __APPLY_AO_FILTERS__

    // __APPLY_LIGHTING__

    // __APPLY_OVERLAY_FILTERS__
}