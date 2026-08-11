#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;

out vec3 RectColor;

uniform mat4 projection;

void main() {
    RectColor = aColor;
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
}
