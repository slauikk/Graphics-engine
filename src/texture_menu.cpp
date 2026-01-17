#include "texture_menu.h"
#include "ui_text.h"
#include <filesystem>
#include <algorithm>

bool TextureMenu::m_isOpen = false;
int TextureMenu::m_selectedIndex = 0;
std::vector<TextureMenu::TextureOption> TextureMenu::m_textures;
bool TextureMenu::m_needsReload = false;
std::string TextureMenu::m_lastSelectedPath = "";

void TextureMenu::init() {
    scanTextures();
}

void TextureMenu::scanTextures() {
    m_textures.clear();
    
    TextureOption gridOption;
    gridOption.name = "Grid (Generated)";
    gridOption.path = "GENERATED_GRID";
    gridOption.isGenerated = true;
    m_textures.push_back(gridOption);
    
    std::string texturesDir = "../assets/textures/";
    
    try {
        if (std::filesystem::exists(texturesDir) && std::filesystem::is_directory(texturesDir)) {
            for (const auto& entry : std::filesystem::directory_iterator(texturesDir)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    
                    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
                        TextureOption option;
                        option.name = entry.path().filename().string();
                        option.path = entry.path().string();
                        option.isGenerated = false;
                        m_textures.push_back(option);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
    }
    
    if (m_textures.empty()) {
        TextureOption gridOption;
        gridOption.name = "Grid (Generated)";
        gridOption.path = "GENERATED_GRID";
        gridOption.isGenerated = true;
        m_textures.push_back(gridOption);
    }
    
    if (m_selectedIndex >= static_cast<int>(m_textures.size())) {
        m_selectedIndex = 0;
    }
}

void TextureMenu::update() {
    if (m_isOpen && m_textures.empty()) {
        scanTextures();
    }
}

void TextureMenu::render() {
    if (!m_isOpen) return;
    
    float x = 200.0f;
    float y = 200.0f;
    float lineHeight = 14.0f * 1.5f;
    
    std::string header = "Texture Selection:\n\n";
    UIText::renderText(header, x, y, 1.5f);
    y += lineHeight * 2;
    
    for (size_t i = 0; i < m_textures.size(); i++) {
        std::string itemText;
        if (static_cast<int>(i) == m_selectedIndex) {
            itemText = "> " + m_textures[i].name;
            UIText::renderTextWithColor(itemText, x, y, 1.5f, 1.0f, 1.0f, 0.0f);
        } else {
            itemText = "  " + m_textures[i].name;
            UIText::renderText(itemText, x, y, 1.5f);
        }
        y += lineHeight;
    }
    
    y += lineHeight;
    std::string instructions = "\nArrow Keys: Navigate\nEnter: Select\nF8: Close";
    UIText::renderText(instructions, x, y, 1.5f);
}

bool TextureMenu::isOpen() {
    return m_isOpen;
}

void TextureMenu::toggle() {
    m_isOpen = !m_isOpen;
    if (m_isOpen) {
        scanTextures();
    }
}

void TextureMenu::processKey(int key) {
    if (!m_isOpen) return;
    
    if (key == 265) { // GLFW_KEY_UP
        m_selectedIndex--;
        if (m_selectedIndex < 0) {
            m_selectedIndex = static_cast<int>(m_textures.size()) - 1;
        }
    } else if (key == 264) { // GLFW_KEY_DOWN
        m_selectedIndex++;
        if (m_selectedIndex >= static_cast<int>(m_textures.size())) {
            m_selectedIndex = 0;
        }
    } else if (key == 257) { // GLFW_KEY_ENTER
        if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_textures.size())) {
            m_lastSelectedPath = m_textures[m_selectedIndex].path;
            m_needsReload = true;
            m_isOpen = false;
        }
    }
}

std::string TextureMenu::getSelectedTexturePath() {
    return m_lastSelectedPath;
}

bool TextureMenu::needsReload() {
    return m_needsReload;
}

void TextureMenu::markReloaded() {
    m_needsReload = false;
}
