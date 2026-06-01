#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 view;       // ожидается без translation (mat3 от view)
uniform mat4 projection;

out vec3 TexCoords;

void main()
{
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    // z = w => после перспективного деления depth = 1.0 (максимум),
    // skybox проходит только там, где сцена ничего не нарисовала (depth func LEQUAL).
    gl_Position = pos.xyww;
}
