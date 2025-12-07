#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform float uAlpha;

void main()
{
    vec4 texColor = texture(uTexture, TexCoords);
    FragColor = vec4(texColor.rgb, texColor.a * uAlpha);
}
