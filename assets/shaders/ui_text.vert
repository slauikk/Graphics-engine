#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aColor;

out vec2 TexCoord;
out vec3 TextColor;

uniform mat4 projection;

void main() {
    TexCoord = aTexCoord;
    TextColor = aColor;
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
}
