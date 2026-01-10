#version 330 core

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec4 uColor;
uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform float uAmbientStrength;

void main()
{
    // Ambient
    vec3 ambient = uAmbientStrength * uLightColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor;
    
    vec3 result = (ambient + diffuse) * uColor.rgb;
    FragColor = vec4(result, uColor.a);
}
