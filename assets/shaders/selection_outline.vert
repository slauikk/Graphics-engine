#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform float u_OutlineWidth;

void main() {
    vec3 worldPosition = vec3(u_Model * vec4(aPos, 1.0));
    mat3 normalMatrix = transpose(inverse(mat3(u_Model)));
    vec3 worldNormal = normalize(normalMatrix * aNormal);
    worldPosition += worldNormal * u_OutlineWidth;
    gl_Position = u_Projection * u_View * vec4(worldPosition, 1.0);
}
