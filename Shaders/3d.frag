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

// Button lights (small point lights when buttons are pressed)
#define MAX_BUTTON_LIGHTS 12
uniform vec3 uButtonLightPos[MAX_BUTTON_LIGHTS];
uniform int uButtonLightActive[MAX_BUTTON_LIGHTS];
uniform int uNumButtonLights;

vec3 calculatePointLight(vec3 lightPos, vec3 lightColor, float constant, float linear, float quadratic, float intensityBoost)
{
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (constant + linear * distance + quadratic * distance * distance);
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    
    return diff * lightColor * attenuation * intensityBoost;
}

void main()
{
    // Ambient - moderate base lighting
    vec3 ambient = uAmbientStrength * uLightColor;
    
    // Main light diffuse
    vec3 diffuse = calculatePointLight(uLightPos, uLightColor, uConstant, uLinear, uQuadratic, 3.5);
    
    // Button lights contribution (strong local glow)
    vec3 buttonLightColor = vec3(1.0, 0.9, 0.5);  // Warm yellow color
    for (int i = 0; i < uNumButtonLights; i++) {
        if (uButtonLightActive[i] == 1) {
            // Short range but strong intensity - bright glow around the button
            diffuse += calculatePointLight(uButtonLightPos[i], buttonLightColor, 1.0, 2.5, 8.0, 4.5);
        }
    }
    
    // Combine
    vec4 texColor = texture(uTexture, TexCoords);
    vec3 result = (ambient + diffuse) * texColor.rgb;
    
    FragColor = vec4(result, texColor.a * uAlpha);
}
