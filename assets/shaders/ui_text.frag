#version 330 core
in vec2 TexCoord;
in vec3 TextColor;

out vec4 FragColor;

uniform sampler2D fontAtlas;

void main() {
    float coverage = texture(fontAtlas, TexCoord).r;
    if (coverage <= 0.001) {
        discard;
    }

    FragColor = vec4(TextColor, coverage);
}
