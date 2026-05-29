#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord0;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec2 texCoord1;
layout (location = 5) in vec4 color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;  
out vec3 Normal;
out vec2 TexCoords0;
out vec2 TexCoords1;
out mat3 TBN;
out vec4 Color;
  
void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f);
    FragPos = vec3(model * vec4(position, 1.0f));

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 N = normalize(normalMatrix * normal);
    vec3 T = normalize(normalMatrix * tangent);
    T      = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    TBN    = mat3(T, B, N);
    
    Normal    = N;

    TexCoords0 = texCoord0;
    TexCoords1 = texCoord1;
    Color      = color;
}