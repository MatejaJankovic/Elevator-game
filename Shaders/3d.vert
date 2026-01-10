#version 330 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTexCoords;
layout(location = 2) in vec3 inNormal;

out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    FragPos = vec3(uModel * vec4(inPos, 1.0));
    Normal = mat3(transpose(inverse(uModel))) * inNormal;
    TexCoords = inTexCoords;
    
    gl_Position = uProjection * uView * uModel * vec4(inPos, 1.0);
}
