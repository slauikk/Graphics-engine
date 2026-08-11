#version 330 core

in vec3 RectColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(RectColor, 1.0);
}
