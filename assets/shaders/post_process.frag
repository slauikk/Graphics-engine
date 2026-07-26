#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D u_Scene;
uniform int u_Effect;
uniform float u_Time;

float luminance(vec3 color) {
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

vec3 sobelEdges(vec2 uv) {
    vec2 texel = 1.0 / vec2(textureSize(u_Scene, 0));
    float topLeft = luminance(texture(u_Scene, uv + texel * vec2(-1.0,  1.0)).rgb);
    float top = luminance(texture(u_Scene, uv + texel * vec2(0.0,  1.0)).rgb);
    float topRight = luminance(texture(u_Scene, uv + texel * vec2(1.0,  1.0)).rgb);
    float left = luminance(texture(u_Scene, uv + texel * vec2(-1.0, 0.0)).rgb);
    float right = luminance(texture(u_Scene, uv + texel * vec2(1.0, 0.0)).rgb);
    float bottomLeft = luminance(texture(u_Scene, uv + texel * vec2(-1.0, -1.0)).rgb);
    float bottom = luminance(texture(u_Scene, uv + texel * vec2(0.0, -1.0)).rgb);
    float bottomRight = luminance(texture(u_Scene, uv + texel * vec2(1.0, -1.0)).rgb);

    float gradientX = -topLeft - 2.0 * left - bottomLeft
                      + topRight + 2.0 * right + bottomRight;
    float gradientY = topLeft + 2.0 * top + topRight
                      - bottomLeft - 2.0 * bottom - bottomRight;
    float edge = clamp(length(vec2(gradientX, gradientY)) * 1.5, 0.0, 1.0);
    return vec3(edge);
}

float vignette(vec2 uv, float strength) {
    vec2 centered = uv * 2.0 - 1.0;
    float radius = dot(centered, centered);
    return 1.0 - smoothstep(0.25, 1.15, radius) * strength;
}

vec3 crtColor(vec2 uv) {
    vec2 centered = uv * 2.0 - 1.0;
    vec2 warped = centered * (1.0 + 0.075 * dot(centered, centered));
    vec2 sampleUv = warped * 0.5 + 0.5;
    if (any(lessThan(sampleUv, vec2(0.0))) || any(greaterThan(sampleUv, vec2(1.0)))) {
        return vec3(0.0);
    }

    vec2 texel = 1.0 / vec2(textureSize(u_Scene, 0));
    float red = texture(u_Scene, sampleUv + vec2(texel.x * 1.5, 0.0)).r;
    float green = texture(u_Scene, sampleUv).g;
    float blue = texture(u_Scene, sampleUv - vec2(texel.x * 1.5, 0.0)).b;
    vec3 color = vec3(red, green, blue);

    float scanline = 0.88 + 0.12 * sin(
        sampleUv.y * float(textureSize(u_Scene, 0).y) * 3.14159265 + u_Time * 18.0);
    float phosphor = 0.96 + 0.04 * sin(
        sampleUv.x * float(textureSize(u_Scene, 0).x) * 3.14159265);
    return color * scanline * phosphor * vignette(sampleUv, 0.72);
}

void main() {
    vec3 color = texture(u_Scene, TexCoord).rgb;

    if (u_Effect == 1) {
        color = vec3(luminance(color));
    } else if (u_Effect == 2) {
        color = vec3(1.0) - color;
    } else if (u_Effect == 3) {
        color = sobelEdges(TexCoord);
    } else if (u_Effect == 4) {
        color *= vignette(TexCoord, 0.9);
    } else if (u_Effect == 5) {
        color = crtColor(TexCoord);
    }

    FragColor = vec4(color, 1.0);
}
