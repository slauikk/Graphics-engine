#include "ui_text.h"
#include "font_data.h"
#include "shader.h"
#include "core/resource_manager.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <string_view>

namespace {

constexpr std::uint32_t kReplacementCodePoint = 0xfffd;
constexpr int kFirstBakedGlyph = 32;
constexpr int kLastBakedGlyph = 126;
constexpr std::size_t kBakedGlyphCount =
    static_cast<std::size_t>(kLastBakedGlyph - kFirstBakedGlyph + 1);
constexpr int kTrueTypeAtlasSize = 512;
constexpr float kTrueTypePixelHeight = 17.0f;

struct FontGlyph {
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
    float xOffset = 0.0f;
    float yOffset = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float advance = 8.0f;
};

std::array<FontGlyph, kBakedGlyphCount> g_fontGlyphs;
bool g_usingTrueTypeFont = false;
float g_fontAscent = 12.0f;
float g_fontLineHeight = 14.0f;

std::size_t bakedGlyphIndex(char character) {
    unsigned int code = static_cast<unsigned char>(character);
    if (code < kFirstBakedGlyph || code > kLastBakedGlyph) {
        code = static_cast<unsigned int>('?');
    }
    return static_cast<std::size_t>(code - kFirstBakedGlyph);
}

float glyphAdvance(char character) {
    if (!g_usingTrueTypeFont) {
        return 8.0f;
    }
    return g_fontGlyphs[bakedGlyphIndex(character)].advance;
}

std::vector<std::filesystem::path> fontCandidates() {
    std::vector<std::filesystem::path> candidates;
#ifdef _WIN32
    char* windowsDirectory = nullptr;
    std::size_t windowsDirectoryLength = 0;
    if (_dupenv_s(
            &windowsDirectory,
            &windowsDirectoryLength,
            "WINDIR") == 0 &&
        windowsDirectory != nullptr && windowsDirectoryLength > 1) {
        const std::filesystem::path fonts =
            std::filesystem::path(windowsDirectory) / "Fonts";
        candidates.push_back(fonts / "segoeui.ttf");
        candidates.push_back(fonts / "SegUIVar.ttf");
        candidates.push_back(fonts / "arial.ttf");
    }
    std::free(windowsDirectory);
#elif defined(__APPLE__)
    candidates.emplace_back("/System/Library/Fonts/SFNS.ttf");
    candidates.emplace_back("/System/Library/Fonts/Helvetica.ttc");
#else
    candidates.emplace_back(
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    candidates.emplace_back(
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf");
#endif
    return candidates;
}

std::vector<unsigned char> readBinaryFile(
    const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        return {};
    }

    const std::streamoff size = stream.tellg();
    if (size <= 0) {
        return {};
    }
    std::vector<unsigned char> bytes(static_cast<std::size_t>(size));
    stream.seekg(0, std::ios::beg);
    if (!stream.read(
            reinterpret_cast<char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()))) {
        return {};
    }
    return bytes;
}

bool bakeTrueTypeAtlas(
    std::vector<unsigned char>& atlas,
    std::filesystem::path& loadedFont) {
    for (const std::filesystem::path& candidate : fontCandidates()) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(candidate, error) || error) {
            continue;
        }

        const std::vector<unsigned char> fontBytes = readBinaryFile(candidate);
        if (fontBytes.empty()) {
            continue;
        }
        const int fontOffset = stbtt_GetFontOffsetForIndex(fontBytes.data(), 0);
        stbtt_fontinfo fontInfo{};
        if (fontOffset < 0 ||
            stbtt_InitFont(&fontInfo, fontBytes.data(), fontOffset) == 0) {
            continue;
        }

        std::array<stbtt_bakedchar, kBakedGlyphCount> bakedGlyphs{};
        atlas.assign(
            static_cast<std::size_t>(
                kTrueTypeAtlasSize * kTrueTypeAtlasSize),
            0);
        const int bakeResult = stbtt_BakeFontBitmap(
            fontBytes.data(), fontOffset, kTrueTypePixelHeight,
            atlas.data(), kTrueTypeAtlasSize, kTrueTypeAtlasSize,
            kFirstBakedGlyph, static_cast<int>(kBakedGlyphCount),
            bakedGlyphs.data());
        if (bakeResult <= 0) {
            continue;
        }

        for (std::size_t index = 0; index < bakedGlyphs.size(); ++index) {
            const stbtt_bakedchar& source = bakedGlyphs[index];
            FontGlyph& glyph = g_fontGlyphs[index];
            glyph.u0 = static_cast<float>(source.x0) /
                static_cast<float>(kTrueTypeAtlasSize);
            glyph.v0 = static_cast<float>(source.y0) /
                static_cast<float>(kTrueTypeAtlasSize);
            glyph.u1 = static_cast<float>(source.x1) /
                static_cast<float>(kTrueTypeAtlasSize);
            glyph.v1 = static_cast<float>(source.y1) /
                static_cast<float>(kTrueTypeAtlasSize);
            glyph.xOffset = source.xoff;
            glyph.yOffset = source.yoff;
            glyph.width = static_cast<float>(source.x1 - source.x0);
            glyph.height = static_cast<float>(source.y1 - source.y0);
            glyph.advance = source.xadvance;
        }

        int ascent = 0;
        int descent = 0;
        int lineGap = 0;
        stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
        const float fontScale =
            stbtt_ScaleForPixelHeight(&fontInfo, kTrueTypePixelHeight);
        g_fontAscent = static_cast<float>(ascent) * fontScale;
        g_fontLineHeight = static_cast<float>(
            ascent - descent + lineGap) * fontScale;
        loadedFont = candidate;
        return true;
    }
    return false;
}

bool isUtf8Continuation(unsigned char byte) {
    return (byte & 0xc0U) == 0x80U;
}

std::uint32_t decodeNextUtf8(std::string_view text, std::size_t& offset) {
    const auto byte = [&text](std::size_t index) {
        return static_cast<unsigned char>(text[index]);
    };

    const unsigned char lead = byte(offset);
    if (lead <= 0x7fU) {
        ++offset;
        return lead;
    }

    if (lead >= 0xc2U && lead <= 0xdfU && offset + 1 < text.size() &&
        isUtf8Continuation(byte(offset + 1))) {
        const std::uint32_t codePoint =
            (static_cast<std::uint32_t>(lead & 0x1fU) << 6U) |
            static_cast<std::uint32_t>(byte(offset + 1) & 0x3fU);
        offset += 2;
        return codePoint;
    }

    if (lead >= 0xe0U && lead <= 0xefU && offset + 2 < text.size()) {
        const unsigned char second = byte(offset + 1);
        const unsigned char third = byte(offset + 2);
        const bool validSecond = isUtf8Continuation(second) &&
            (lead != 0xe0U || second >= 0xa0U) &&
            (lead != 0xedU || second <= 0x9fU);
        if (validSecond && isUtf8Continuation(third)) {
            const std::uint32_t codePoint =
                (static_cast<std::uint32_t>(lead & 0x0fU) << 12U) |
                (static_cast<std::uint32_t>(second & 0x3fU) << 6U) |
                static_cast<std::uint32_t>(third & 0x3fU);
            offset += 3;
            return codePoint;
        }
    }

    if (lead >= 0xf0U && lead <= 0xf4U && offset + 3 < text.size()) {
        const unsigned char second = byte(offset + 1);
        const unsigned char third = byte(offset + 2);
        const unsigned char fourth = byte(offset + 3);
        const bool validSecond = isUtf8Continuation(second) &&
            (lead != 0xf0U || second >= 0x90U) &&
            (lead != 0xf4U || second <= 0x8fU);
        if (validSecond && isUtf8Continuation(third) && isUtf8Continuation(fourth)) {
            const std::uint32_t codePoint =
                (static_cast<std::uint32_t>(lead & 0x07U) << 18U) |
                (static_cast<std::uint32_t>(second & 0x3fU) << 12U) |
                (static_cast<std::uint32_t>(third & 0x3fU) << 6U) |
                static_cast<std::uint32_t>(fourth & 0x3fU);
            offset += 4;
            return codePoint;
        }
    }

    ++offset;
    return kReplacementCodePoint;
}

class ScopedTextRenderState {
public:
    ScopedTextRenderState()
        : m_depthTestEnabled(glIsEnabled(GL_DEPTH_TEST)),
          m_blendEnabled(glIsEnabled(GL_BLEND)) {
        glGetIntegerv(GL_CURRENT_PROGRAM, &m_program);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &m_vertexArray);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &m_arrayBuffer);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &m_activeTexture);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_texture2D);
        glGetIntegerv(GL_BLEND_SRC_RGB, &m_blendSrcRgb);
        glGetIntegerv(GL_BLEND_DST_RGB, &m_blendDstRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &m_blendSrcAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &m_blendDstAlpha);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    ~ScopedTextRenderState() {
        glUseProgram(static_cast<GLuint>(m_program));
        glBindVertexArray(static_cast<GLuint>(m_vertexArray));
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(m_arrayBuffer));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(m_texture2D));
        glActiveTexture(static_cast<GLenum>(m_activeTexture));
        glBlendFuncSeparate(m_blendSrcRgb, m_blendDstRgb, m_blendSrcAlpha, m_blendDstAlpha);

        if (m_blendEnabled) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }

        if (m_depthTestEnabled) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
    }

private:
    GLboolean m_depthTestEnabled;
    GLboolean m_blendEnabled;
    GLint m_program = 0;
    GLint m_vertexArray = 0;
    GLint m_arrayBuffer = 0;
    GLint m_activeTexture = GL_TEXTURE0;
    GLint m_texture2D = 0;
    GLint m_blendSrcRgb = GL_ONE;
    GLint m_blendDstRgb = GL_ZERO;
    GLint m_blendSrcAlpha = GL_ONE;
    GLint m_blendDstAlpha = GL_ZERO;
};

} // namespace

GLuint UIText::VAO = 0;
GLuint UIText::VBO = 0;
GLuint UIText::fontTexture = 0;
std::shared_ptr<Shader> UIText::shader = nullptr;
bool UIText::initialized = false;
bool UIText::frameBatchActive = false;
int UIText::windowWidth = 1280;
int UIText::windowHeight = 720;
std::vector<float> UIText::frameVertices;

bool UIText::createTextShader() {
    shader = ResourceManager::getShader("shaders/ui_text.vert", "shaders/ui_text.frag");
    return shader && shader->m_id != 0;
}

bool UIText::createFontAtlas() {
    constexpr int glyphWidth = 8;
    constexpr int glyphHeight = 12;
    constexpr int glyphsPerRow = 16;
    int atlasWidth = kTrueTypeAtlasSize;
    int atlasHeight = kTrueTypeAtlasSize;
    std::vector<unsigned char> atlas;
    std::filesystem::path loadedFont;
    g_usingTrueTypeFont = bakeTrueTypeAtlas(atlas, loadedFont);

    if (!g_usingTrueTypeFont) {
        atlasWidth = glyphWidth * glyphsPerRow;
        atlasHeight = glyphHeight * glyphsPerRow;
        atlas.assign(
            static_cast<std::size_t>(atlasWidth * atlasHeight), 0);
        g_fontAscent = 12.0f;
        g_fontLineHeight = 14.0f;

        for (const auto& [character, bitmap] : fontData) {
            const unsigned int code = static_cast<unsigned char>(character);
            const int atlasX = static_cast<int>(code % glyphsPerRow) * glyphWidth;
            const int atlasY = static_cast<int>(code / glyphsPerRow) * glyphHeight;

            for (int row = 0; row < glyphHeight; ++row) {
                for (int sourceColumn = 0;
                     sourceColumn < glyphWidth;
                     ++sourceColumn) {
                    if ((bitmap[row] & (0x80 >> sourceColumn)) == 0) {
                        continue;
                    }

                    const int screenColumn = glyphWidth - 1 - sourceColumn;
                    atlas[static_cast<std::size_t>(
                        (atlasY + row) * atlasWidth +
                        atlasX + screenColumn)] = 255;
                }
            }
        }
    }

    GLint previousTexture = 0;
    GLint previousUnpackAlignment = 4;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);

    glGenTextures(1, &fontTexture);
    if (fontTexture == 0) {
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlasWidth, atlasHeight, 0,
                 GL_RED, GL_UNSIGNED_BYTE, atlas.data());
    const GLint filter = g_usingTrueTypeFont ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    if (g_usingTrueTypeFont) {
        std::cout << "[UIText] Loaded font: " << loadedFont.string() << "\n";
    } else {
        std::cerr << "[UIText] System font unavailable; using bitmap fallback\n";
    }
    return true;
}

void UIText::init(int width, int height) {
    if (initialized) return;
    
    UIText::windowWidth = width;
    UIText::windowHeight = height;
    
    if (!createTextShader()) {
        std::cerr << "[UIText] Failed to create shader\n";
        return;
    }
    initFontData();
    if (!createFontAtlas()) {
        std::cerr << "[UIText] Failed to create font atlas\n";
        shader.reset();
        return;
    }
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                          reinterpret_cast<void*>(4 * sizeof(float)));
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    initialized = true;
}

void UIText::beginFrame() {
    if (!initialized) {
        return;
    }

    frameVertices.clear();
    frameBatchActive = true;
}

void UIText::flush() {
    if (!frameBatchActive) {
        return;
    }

    frameBatchActive = false;
    drawBatch(frameVertices);
    frameVertices.clear();
}

void UIText::updateWindowSize(int width, int height) {
    windowWidth = width;
    windowHeight = height;
}

void UIText::appendCharVertices(std::vector<float>& vertices, char c, float x, float y,
                                float scale, float r, float g, float b) {
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
    float x0 = x;
    float y0 = y;
    float x1 = x;
    float y1 = y;
    if (g_usingTrueTypeFont) {
        const FontGlyph& glyph = g_fontGlyphs[bakedGlyphIndex(c)];
        if (glyph.width <= 0.0f || glyph.height <= 0.0f) {
            return;
        }
        u0 = glyph.u0;
        v0 = glyph.v0;
        u1 = glyph.u1;
        v1 = glyph.v1;
        x0 = x + glyph.xOffset * scale;
        y0 = y + (g_fontAscent + glyph.yOffset) * scale;
        x1 = x0 + glyph.width * scale;
        y1 = y0 + glyph.height * scale;
    } else {
        if (fontData.find(c) == fontData.end()) {
            c = '?';
        }
        constexpr float glyphsPerRow = 16.0f;
        const unsigned int code = static_cast<unsigned char>(c);
        const float atlasColumn = static_cast<float>(code % 16);
        const float atlasRow = static_cast<float>(code / 16);
        u0 = atlasColumn / glyphsPerRow;
        v0 = atlasRow / glyphsPerRow;
        u1 = (atlasColumn + 1.0f) / glyphsPerRow;
        v1 = (atlasRow + 1.0f) / glyphsPerRow;
        x1 = x + 8.0f * scale;
        y1 = y + 12.0f * scale;
    }

    const float quad[] = {
        x0, y0, u0, v0,
        x1, y0, u1, v0,
        x1, y1, u1, v1,
        x0, y0, u0, v0,
        x1, y1, u1, v1,
        x0, y1, u0, v1
    };

    for (int vertex = 0; vertex < 6; ++vertex) {
        const int offset = vertex * 4;
        vertices.push_back(quad[offset]);
        vertices.push_back(quad[offset + 1]);
        vertices.push_back(quad[offset + 2]);
        vertices.push_back(quad[offset + 3]);
        vertices.push_back(r);
        vertices.push_back(g);
        vertices.push_back(b);
    }
}

void UIText::appendTextVertices(std::vector<float>& vertices, const std::string& text,
                                float x, float y, float scale, float offsetX, float offsetY,
                                float r, float g, float b) {
    float currentX = x;
    float currentY = y;

    std::size_t offset = 0;
    while (offset < text.size()) {
        const std::uint32_t codePoint = decodeNextUtf8(text, offset);
        if (codePoint == '\n') {
            currentY += g_fontLineHeight * scale;
            currentX = x;
            continue;
        }

        const char glyph = codePoint <= 0x7fU ? static_cast<char>(codePoint) : '?';
        appendCharVertices(vertices, glyph, currentX + offsetX, currentY + offsetY,
                           scale, r, g, b);
        currentX += glyphAdvance(glyph) * scale;
    }
}

void UIText::drawBatch(const std::vector<float>& vertices) {
    if (!initialized || shader == nullptr || shader->m_id == 0 || vertices.empty()) {
        return;
    }

    ScopedTextRenderState renderState;
    shader->use();

    const glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(windowWidth),
                                            static_cast<float>(windowHeight), 0.0f);
    shader->setMat4("projection", projection);
    shader->setInt("fontAtlas", 0);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 7));
}

void UIText::renderTextInternal(const std::string& text, float x, float y, float scale,
                                float r, float g, float b) {
    if (!initialized || shader == nullptr || shader->m_id == 0 || text.empty()) {
        return;
    }

    constexpr std::size_t floatsPerGlyphQuad = 42;
    std::vector<float> immediateVertices;
    std::vector<float>& vertices = frameBatchActive ? frameVertices : immediateVertices;
    vertices.reserve(vertices.size() + text.size() * 2 * floatsPerGlyphQuad);

    const float shadowOffset = 0.75f * scale;
    appendTextVertices(
        vertices, text, x, y, scale,
        shadowOffset, shadowOffset, 0.0f, 0.0f, 0.0f);
    appendTextVertices(vertices, text, x, y, scale, 0.0f, 0.0f, r, g, b);

    if (!frameBatchActive) {
        drawBatch(immediateVertices);
    }
}

void UIText::renderText(const std::string& text, float x, float y, float scale) {
    renderTextInternal(text, x, y, scale, 1.0f, 1.0f, 1.0f);
}

void UIText::renderTextWithColor(const std::string& text, float x, float y, float scale,
                                 float r, float g, float b) {
    renderTextInternal(text, x, y, scale, r, g, b);
}

float UIText::measureTextWidth(const std::string& text, float scale) {
    float lineWidth = 0.0f;
    float maximumWidth = 0.0f;
    std::size_t offset = 0;
    while (offset < text.size()) {
        const std::uint32_t codePoint = decodeNextUtf8(text, offset);
        if (codePoint == '\n') {
            maximumWidth = (std::max)(maximumWidth, lineWidth);
            lineWidth = 0.0f;
            continue;
        }
        const char glyph = codePoint <= 0x7fU
            ? static_cast<char>(codePoint)
            : '?';
        lineWidth += glyphAdvance(glyph) * scale;
    }
    return (std::max)(maximumWidth, lineWidth);
}

void UIText::cleanup() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (fontTexture != 0) {
        glDeleteTextures(1, &fontTexture);
        fontTexture = 0;
    }
    shader.reset();
    frameVertices.clear();
    frameVertices.shrink_to_fit();
    frameBatchActive = false;
    g_usingTrueTypeFont = false;
    g_fontAscent = 12.0f;
    g_fontLineHeight = 14.0f;
    initialized = false;
}

bool UIText::reloadShaders() {
    if (!initialized) return false;
    const bool reloaded = ResourceManager::reloadAllShaders();
    return reloaded && shader && shader->m_id != 0;
}
