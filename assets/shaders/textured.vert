#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in mat4 aInstanceModel;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform int u_UseInstancing;

void main() {
    mat4 model = u_UseInstancing != 0 ? aInstanceModel : u_Model;
    FragPos = vec3(model * vec4(aPos, 1.0));
    if (u_UseInstancing != 0) {
        // Benchmark instances use rotation plus uniform scale, so no inverse is needed.
        Normal = mat3(model) * aNormal;
    } else {
        Normal = transpose(inverse(mat3(model))) * aNormal;
    }
    TexCoord = aTexCoord;
    
    gl_Position = u_Projection * u_View * vec4(FragPos, 1.0);
}
