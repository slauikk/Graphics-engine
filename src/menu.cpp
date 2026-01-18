#include "menu.h"
#include "ui_text.h"
#include <filesystem>
#include <algorithm>

bool Menu::m_isOpen = false;
Menu::MenuState Menu::m_currentState = MAIN_MENU;
int Menu::m_selectedIndex = 0;
std::vector<Menu::TextureOption> Menu::m_textures;
bool Menu::m_needsReload = false;
std::string Menu::m_lastSelectedPath = "";
Menu::MovementState Menu::m_movementState = MOVEMENT_STOPPED;
bool Menu::m_needsMovementUpdate = false;

void Menu::init() {
    scanTextures();
}

void Menu::scanTextures() {
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

void Menu::update() {
    if (m_isOpen && m_textures.empty()) {
        scanTextures();
    }
}

void Menu::render() {
    if (!m_isOpen) return;
    
    switch (m_currentState) {
        case MAIN_MENU:
            renderMainMenu();
            break;
        case TEXTURES:
            renderTexturesMenu();
            break;
        case MOVEMENT:
            renderMovementMenu();
            break;
    }
}

void Menu::renderMainMenu() {
    float x = 200.0f;
    float y = 200.0f;
    float lineHeight = 14.0f * 1.5f;
    
    std::string header = "Main Menu:\n\n";
    UIText::renderText(header, x, y, 1.5f);
    y += lineHeight * 2;
    
    std::vector<std::string> mainMenuItems = {"Textures", "Movement"};
    
    for (size_t i = 0; i < mainMenuItems.size(); i++) {
        std::string itemText;
        if (static_cast<int>(i) == m_selectedIndex) {
            itemText = "> " + mainMenuItems[i];
            UIText::renderTextWithColor(itemText, x, y, 1.5f, 1.0f, 1.0f, 0.0f);
        } else {
            itemText = "  " + mainMenuItems[i];
            UIText::renderText(itemText, x, y, 1.5f);
        }
        y += lineHeight;
    }
    
    y += lineHeight;
    std::string instructions = "\nArrow Keys: Navigate\nEnter: Select\nESC: Back/Close\nF8: Close";
    UIText::renderText(instructions, x, y, 1.5f);
}

void Menu::renderTexturesMenu() {
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
    std::string instructions = "\nArrow Keys: Navigate\nEnter: Select\nESC: Back\nF8: Close";
    UIText::renderText(instructions, x, y, 1.5f);
}

void Menu::renderMovementMenu() {
    float x = 200.0f;
    float y = 200.0f;
    float lineHeight = 14.0f * 1.5f;
    
    std::string header = "Movement Control:\n\n";
    UIText::renderText(header, x, y, 1.5f);
    y += lineHeight * 2;
    
    std::vector<std::string> movementItems = {"Reset", "Spin", "Stop"};
    
    for (size_t i = 0; i < movementItems.size(); i++) {
        std::string itemText;
        if (static_cast<int>(i) == m_selectedIndex) {
            itemText = "> " + movementItems[i];
            UIText::renderTextWithColor(itemText, x, y, 1.5f, 1.0f, 1.0f, 0.0f);
        } else {
            itemText = "  " + movementItems[i];
            UIText::renderText(itemText, x, y, 1.5f);
        }
        y += lineHeight;
    }
    
    y += lineHeight;
    std::string instructions = "\nArrow Keys: Navigate\nEnter: Select\nESC: Back\nF8: Close";
    UIText::renderText(instructions, x, y, 1.5f);
}

bool Menu::isOpen() {
    return m_isOpen;
}

void Menu::toggle() {
    m_isOpen = !m_isOpen;
    if (m_isOpen) {
        m_currentState = MAIN_MENU;
        m_selectedIndex = 0;
        scanTextures();
    }
}

void Menu::processKey(int key) {
    if (!m_isOpen) return;
    
    if (key == 265) { // GLFW_KEY_UP
        int maxItems = 0;
        if (m_currentState == MAIN_MENU) {
            maxItems = 2;
        } else if (m_currentState == TEXTURES) {
            maxItems = static_cast<int>(m_textures.size());
        } else if (m_currentState == MOVEMENT) {
            maxItems = 3;
        }
        
        m_selectedIndex--;
        if (m_selectedIndex < 0) {
            m_selectedIndex = maxItems - 1;
        }
    } else if (key == 264) { // GLFW_KEY_DOWN
        int maxItems = 0;
        if (m_currentState == MAIN_MENU) {
            maxItems = 2;
        } else if (m_currentState == TEXTURES) {
            maxItems = static_cast<int>(m_textures.size());
        } else if (m_currentState == MOVEMENT) {
            maxItems = 3;
        }
        
        m_selectedIndex++;
        if (m_selectedIndex >= maxItems) {
            m_selectedIndex = 0;
        }
    } else if (key == 257) { // GLFW_KEY_ENTER
        if (m_currentState == MAIN_MENU) {
            if (m_selectedIndex == 0) {
                m_currentState = TEXTURES;
                m_selectedIndex = 0;
            } else if (m_selectedIndex == 1) {
                m_currentState = MOVEMENT;
                m_selectedIndex = 0;
            }
        } else if (m_currentState == TEXTURES) {
            if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_textures.size())) {
                m_lastSelectedPath = m_textures[m_selectedIndex].path;
                m_needsReload = true;
            }
        } else if (m_currentState == MOVEMENT) {
            if (m_selectedIndex == 0) {
                m_movementState = MOVEMENT_RESET;
                m_needsMovementUpdate = true;
            } else if (m_selectedIndex == 1) {
                m_movementState = MOVEMENT_SPINNING;
                m_needsMovementUpdate = true;
            } else if (m_selectedIndex == 2) {
                m_movementState = MOVEMENT_STOPPED;
                m_needsMovementUpdate = true;
            }
        }
    } else if (key == 256) { // GLFW_KEY_ESC
        if (m_currentState == MAIN_MENU) {
            m_isOpen = false;
        } else {
            m_currentState = MAIN_MENU;
            m_selectedIndex = 0;
        }
    }
}

std::string Menu::getSelectedTexturePath() {
    return m_lastSelectedPath;
}

bool Menu::needsReload() {
    return m_needsReload;
}

void Menu::markReloaded() {
    m_needsReload = false;
}

Menu::MovementState Menu::getMovementState() {
    return m_movementState;
}

bool Menu::needsMovementUpdate() {
    return m_needsMovementUpdate;
}

void Menu::markMovementUpdated() {
    m_needsMovementUpdate = false;
}
