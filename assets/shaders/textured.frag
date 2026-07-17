#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

struct DirLight {
    vec3 direction;
    vec3 color;
    int enabled;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
    int enabled;
};

uniform vec3 u_CameraPos;
uniform DirLight u_DirLight;
uniform PointLight u_PointLight;

uniform int u_UseAlbedoTex;
uniform sampler2D u_AlbedoTex;
uniform vec3 u_AlbedoColor;
uniform vec3 u_SpecularColor;
uniform float u_Shininess;
uniform int u_DebugViewMode;

void main() {
    vec3 albedo = u_UseAlbedoTex != 0 ? texture(u_AlbedoTex, TexCoord).rgb : u_AlbedoColor;
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(u_CameraPos - FragPos);
    
    vec3 ambientResult = vec3(0.0);
    vec3 diffuseResult = vec3(0.0);
    vec3 specularResult = vec3(0.0);
    
    // Directional Light
    if (u_DirLight.enabled != 0) {
        vec3 lightDir = normalize(-u_DirLight.direction);
        
        float ambient = 0.1;
        vec3 ambientColor = ambient * albedo * u_DirLight.color;
        
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuseColor = diff * albedo * u_DirLight.color;
        
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Shininess);
        vec3 specularColor = spec * u_SpecularColor * u_DirLight.color;
        
        ambientResult += ambientColor;
        diffuseResult += diffuseColor;
        specularResult += specularColor;
    }
    
    // Point Light
    if (u_PointLight.enabled != 0) {
        vec3 lightDir = normalize(u_PointLight.position - FragPos);
        float distance = length(u_PointLight.position - FragPos);
        float attenuation = 1.0 / (u_PointLight.constant + u_PointLight.linear * distance + u_PointLight.quadratic * (distance * distance));
        
        float ambient = 0.1;
        vec3 ambientColor = ambient * albedo * u_PointLight.color;
        
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuseColor = diff * albedo * u_PointLight.color;
        
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Shininess);
        vec3 specularColor = spec * u_SpecularColor * u_PointLight.color;
        
        ambientResult += ambientColor * attenuation;
        diffuseResult += diffuseColor * attenuation;
        specularResult += specularColor * attenuation;
    }

    vec3 result = ambientResult + diffuseResult + specularResult;
    if (u_DebugViewMode == 1) {
        result = albedo;
    } else if (u_DebugViewMode == 2) {
        result = norm * 0.5 + 0.5;
    } else if (u_DebugViewMode == 3) {
        result = diffuseResult;
    } else if (u_DebugViewMode == 4) {
        result = specularResult;
    }

    FragColor = vec4(result, 1.0);
}
