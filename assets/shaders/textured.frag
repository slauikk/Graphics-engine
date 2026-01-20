#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 u_CameraPos;
uniform vec3 u_LightPos;
uniform vec3 u_LightColor;

uniform int u_UseAlbedoTex;
uniform sampler2D u_AlbedoTex;
uniform vec3 u_AlbedoColor;
uniform vec3 u_SpecularColor;
uniform float u_Shininess;

void main() {
    vec3 albedo = u_UseAlbedoTex != 0 ? texture(u_AlbedoTex, TexCoord).rgb : u_AlbedoColor;
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(u_LightPos - FragPos);
    
    float ambient = 0.1;
    vec3 ambientColor = ambient * albedo;
    
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuseColor = diff * albedo * u_LightColor;
    
    vec3 viewDir = normalize(u_CameraPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Shininess);
    vec3 specularColor = spec * u_SpecularColor * u_LightColor;
    
    vec3 result = ambientColor + diffuseColor + specularColor;
    FragColor = vec4(result, 1.0);
}
