#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D u_Texture;

void main() {
    vec4 texColor = texture(u_Texture, TexCoord);
    // Add a slight tint to show shaders are loaded from files
    FragColor = vec4(texColor.rgb * 1.5, texColor.a);
}
