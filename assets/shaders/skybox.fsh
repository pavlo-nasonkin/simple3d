#version 330 core
out vec4 color;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{
    vec3 hdr = texture(skybox, TexCoords).rgb;
    // cubemap хранит линейный HDR — приводим к экрану так же, как PBR-шейдер.
    vec3 mapped = hdr / (hdr + vec3(1.0));      // Reinhard
    mapped = pow(mapped, vec3(1.0 / 2.2));      // gamma
    color = vec4(mapped, 1.0);
}
