#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord0;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec2 texCoord1;
layout (location = 5) in vec4 color;
layout (location = 8) in ivec4 BoneIDs;
layout (location = 9) in vec4 Weights;

const int MAX_BONES = 100;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 gBones[MAX_BONES];

out vec3 FragPos;  
out vec3 Normal;
out vec2 TexCoords0;
out vec2 TexCoords1;
out mat3 TBN;
out vec4 Color;

  
void main()
{
    mat4 BoneTransform = gBones[BoneIDs[0]] * Weights[0];
    BoneTransform += gBones[BoneIDs[1]] * Weights[1];
    BoneTransform += gBones[BoneIDs[2]] * Weights[2];
    BoneTransform += gBones[BoneIDs[3]] * Weights[3];

    vec4 transformedPos = BoneTransform * vec4(position, 1.0);
    vec4 worldPos = model * transformedPos;
    gl_Position = projection * view * worldPos;

    FragPos = vec3(worldPos);

    mat3 boneNormalMatrix = transpose(inverse(mat3(BoneTransform)));
    vec3 skinnedNormal  = boneNormalMatrix * normal;
    vec3 skinnedTangent = boneNormalMatrix * tangent;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 N = normalize(normalMatrix * skinnedNormal);
    vec3 T = normalize(normalMatrix * skinnedTangent);
    T      = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    TBN    = mat3(T, B, N);

    Normal    = N;

    TexCoords0 = texCoord0;
    TexCoords1 = texCoord1;
    Color      = color;
}
