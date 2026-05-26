#version 330 core

struct Material {
	//sampler2D diffuse;
    //vec3 specular;    
    float shininess;
}; 

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};



out vec4 color;

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords;
in mat3 TBN;

uniform vec3 viewPos;

uniform Material material;
uniform Light light;
uniform vec4 uBaseColor;

// __FRAGMENT_DECLS__

void main()
{
    vec3 BASE_COLOR    = uBaseColor.rgb;
    vec3 SPEC_STRENGTH = vec3(1.0);
    vec3 N             = normalize(Normal);
    vec3 EMISSIVE      = vec3(0.0);

    // __APPLY_BASE_COLOR_FILTERS__
    // __APPLY_SPECULAR_FILTERS__
    // __APPLY_NORMAL_FILTERS__

    vec3 lightDir = normalize(light.position - FragPos);
    float diff    = max(dot(N, lightDir), 0.0);

    vec3 ambient  = light.ambient  * BASE_COLOR;
    vec3 diffuse  = light.diffuse  * diff * BASE_COLOR;

    vec3 viewDir    = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, N);
    float spec      = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular   = light.specular * spec * SPEC_STRENGTH;

    color = vec4(ambient + diffuse + specular + EMISSIVE, 1.0);
    // __APPLY_OVERLAY_FILTERS__
}