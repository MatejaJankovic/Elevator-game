#version 330 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inTexCoords;

out vec2 TexCoords;

uniform mat4 uModel;

void main()
{
    gl_Position = uModel * vec4(inPos, 0.0, 1.0);
    TexCoords = inTexCoords;
}
