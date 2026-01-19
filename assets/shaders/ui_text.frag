#version 330 core
out vec4 FragColor;

uniform vec3 textColor;

void main() {
    // Slightly brighter text to show shaders are loaded from files
    FragColor = vec4(textColor * 1.1, 1.0);
}
