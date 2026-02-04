#version 330 core

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform float uAlpha;
uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform vec3 uLightColor;
uniform float uAmbientStrength;

// Attenuation parameters for point light
uniform float uConstant;    // Usually 1.0
uniform float uLinear;      // Distance-based falloff
uniform float uQuadratic;   // Distance-squared falloff

void main()
{
    // Calculate distance from fragment to light
    float distance = length(uLightPos - FragPos);
    
    // Calculate attenuation (light falloff with distance)
    float attenuation = 1.0 / (uConstant + uLinear * distance + uQuadratic * distance * distance);
    
    // Ambient - minimal ambient for realistic dark areas
    vec3 ambient = uAmbientStrength * uLightColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor * attenuation;
    
    // Combine
    vec4 texColor = texture(uTexture, TexCoords);
    vec3 result = (ambient + diffuse) * texColor.rgb;
    
    FragColor = vec4(result, texColor.a * uAlpha);
}
