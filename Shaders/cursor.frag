#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform float uAlpha;
uniform int uVentilationActive;
uniform vec3 uVentilationColor;

void main()
{
    vec4 texColor = texture(uTexture, TexCoords);
    
    if (uVentilationActive == 1 && texColor.a > 0.1) {
        float brightness = (texColor.r + texColor.g + texColor.b) / 3.0;
        FragColor = vec4(uVentilationColor * (brightness + 0.5), texColor.a * uAlpha);
    } else {
        FragColor = vec4(texColor.rgb, texColor.a * uAlpha);
    }
}
