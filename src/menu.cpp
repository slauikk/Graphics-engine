#include "menu.h"
#include "ui_text.h"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <iomanip>

bool Menu::m_isOpen = false;
Menu::MenuState Menu::m_currentState = MAIN_MENU;
int Menu::m_selectedIndex = 0;
std::vector<Menu::TextureOption> Menu::m_textures;
bool Menu::m_needsReload = false;
std::string Menu::m_lastSelectedPath = "";
Menu::MovementState Menu::m_movementState = MOVEMENT_STOPPED;
bool Menu::m_needsMovementUpdate = false;
Menu::CubeControlAction Menu::m_cubeControlAction = CUBE_NONE;
bool Menu::m_needsCubeUpdate = false;
float Menu::m_cubePosX = 0.0f;
float Menu::m_cubePosY = 0.0f;
float Menu::m_cubePosZ = 0.0f;
int Menu::m_selectedCubeIndex = 0;
Menu::LightControlAction Menu::m_lightControlAction = LIGHT_NONE;
bool Menu::m_needsLightUpdate = false;
float Menu::m_lightPosX = 2.0f;
float Menu::m_lightPosY = 2.0f;
float Menu::m_lightPosZ = 2.0f;
Menu::DirLightControlAction Menu::m_dirLightControlAction = DIRLIGHT_NONE;
bool Menu::m_needsDirLightUpdate = false;

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
                        option.path = texturesDir + option.name;
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
        case MOVEMENT_ROOT:
            renderMovementRootMenu();
            break;
        case MOVEMENT_CUBE:
            renderCubeMenu();
            break;
        case MOVEMENT_LIGHT:
            renderLightMenu();
            break;
        case LIGHTING:
            renderLightingMenu();
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
    
    std::vector<std::string> mainMenuItems = {"Textures", "Movement", "Lighting"};
    
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

void Menu::renderMovementRootMenu() {
    float x = 200.0f;
    float y = 200.0f;
    float lineHeight = 14.0f * 1.5f;
    
    std::string header = "Movement:\n\n";
    UIText::renderText(header, x, y, 1.5f);
    y += lineHeight * 2;
    
    std::vector<std::string> items = {"Cube", "Light"};
    
    for (size_t i = 0; i < items.size(); i++) {
        std::string itemText;
        if (static_cast<int>(i) == m_selectedIndex) {
            itemText = "> " + items[i];
            UIText::renderTextWithColor(itemText, x, y, 1.5f, 1.0f, 1.0f, 0.0f);
        } else {
            itemText = "  " + items[i];
            UIText::renderText(itemText, x, y, 1.5f);
        }
        y += lineHeight;
    }
    
    y += lineHeight;
    std::string instructions = "\nArrow Keys: Navigate\nEnter: Select\nESC: Back\nF8: Close";
    UIText::renderText(instructions, x, y, 1.5f);
}

void Menu::renderCubeMenu() {
    float x = 200.0f;
    float y = 200.0f;
    float lineHeight = 14.0f * 1.5f;
    
    std::string header = "Cube Control:\n\n";
    UIText::renderText(header, x, y, 1.5f);
    y += lineHeight * 2;
    
    std::vector<std::string> items = {
        "Prev Cube",
        "Next Cube",
        "---",
        "Reset",
        "Spin",
        "Stop",
        "---",
        "Move X+",
        "Move X-",
        "Move Y+",
        "Move Y-",
        "Move Z+",
        "Move Z-"
    };
    
    std::ostringstream cubeInfo;
    cubeInfo << "Selected Cube: " << m_selectedCubeIndex;
    UIText::renderText(cubeInfo.str(), x, y, 1.5f);
    y += lineHeight * 2;
    
    for (size_t i = 0; i < items.size(); i++) {
        std::string itemText;
        if (static_cast<int>(i) == m_selectedIndex) {
            itemText = "> " + items[i];
            UIText::renderTextWithColor(itemText, x, y, 1.5f, 1.0f, 1.0f, 0.0f);
        } else {
            itemText = "  " + items[i];
            UIText::renderText(itemText, x, y, 1.5f);
        }
        y += lineHeight;
    }
    
    y += lineHeight;
    std::ostringstream posStr;
    posStr << std::fixed << std::setprecision(1);
    posStr << "\nCube Pos: (" << m_cubePosX << ", " << m_cubePosY << ", " << m_cubePosZ << ")";
    UIText::renderText(posStr.str(), x, y, 1.5f);
    y += lineHeight * 2;
    
    std::string instructions = "\nArrow Keys: Navigate\nEnter: Select\nESC: Back\nF8: Close";
    UIText::renderText(instructions, x, y, 1.5f);
}

void Menu::renderLightMenu() {
    float x = 200.0f;
    float y = 200.0f;
    float lineHeight = 14.0f * 1.5f;
    
    std::string header = "Light Control:\n\n";
    UIText::renderText(header, x, y, 1.5f);
    y += lineHeight * 2;
    
    std::vector<std::string> items = {
        "Reset",
        "Spin",
        "Stop",
        "---",
        "Move X+",
        "Move X-",
        "Move Y+",
        "Move Y-",
        "Move Z+",
        "Move Z-",
        "---",
        "XY Plane: Up",
        "XY Plane: Down",
        "XY Plane: Left",
        "XY Plane: Right",
        "---",
        "XZ Plane: Forward",
        "XZ Plane: Back",
        "XZ Plane: Left",
        "XZ Plane: Right",
        "---",
        "YZ Plane: Up",
        "YZ Plane: Down",
        "YZ Plane: Forward",
        "YZ Plane: Back"
    };
    
    for (size_t i = 0; i < items.size(); i++) {
        std::string itemText;
        if (static_cast<int>(i) == m_selectedIndex) {
            itemText = "> " + items[i];
            UIText::renderTextWithColor(itemText, x, y, 1.5f, 1.0f, 1.0f, 0.0f);
        } else {
            itemText = "  " + items[i];
            UIText::renderText(itemText, x, y, 1.5f);
        }
        y += lineHeight;
    }
    
    y += lineHeight;
    std::ostringstream lightPosStr;
    lightPosStr << std::fixed << std::setprecision(1);
    lightPosStr << "\nLight Pos: (" << m_lightPosX << ", " << m_lightPosY << ", " << m_lightPosZ << ")";
    UIText::renderText(lightPosStr.str(), x, y, 1.5f);
    y += lineHeight * 2;
    
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
    
    if (key == GLFW_KEY_UP) {
        int maxItems = 0;
        if (m_currentState == MAIN_MENU) {
            maxItems = 2;
        } else if (m_currentState == TEXTURES) {
            maxItems = static_cast<int>(m_textures.size());
        } else if (m_currentState == MOVEMENT_ROOT) {
            maxItems = 2;
        } else if (m_currentState == MOVEMENT_CUBE) {
            maxItems = 10;
        } else if (m_currentState == MOVEMENT_LIGHT) {
            maxItems = 25;
        } else if (m_currentState == LIGHTING) {
            maxItems = 6;
        }
        
        m_selectedIndex--;
        if (m_selectedIndex < 0) {
            m_selectedIndex = maxItems - 1;
        }
    } else if (key == GLFW_KEY_DOWN) {
        int maxItems = 0;
        if (m_currentState == MAIN_MENU) {
            maxItems = 3;
        } else if (m_currentState == TEXTURES) {
            maxItems = static_cast<int>(m_textures.size());
        } else if (m_currentState == MOVEMENT_ROOT) {
            maxItems = 2;
        } else if (m_currentState == MOVEMENT_CUBE) {
            maxItems = 10;
        } else if (m_currentState == MOVEMENT_LIGHT) {
            maxItems = 25;
        } else if (m_currentState == LIGHTING) {
            maxItems = 6;
        }
        
        m_selectedIndex++;
        if (m_selectedIndex >= maxItems) {
            m_selectedIndex = 0;
        }
    } else if (key == GLFW_KEY_ENTER) {
        if (m_currentState == MAIN_MENU) {
            if (m_selectedIndex == 0) {
                m_currentState = TEXTURES;
                m_selectedIndex = 0;
            } else if (m_selectedIndex == 1) {
                m_currentState = MOVEMENT_ROOT;
                m_selectedIndex = 0;
            } else if (m_selectedIndex == 2) {
                m_currentState = LIGHTING;
                m_selectedIndex = 0;
            }
        } else if (m_currentState == TEXTURES) {
            if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_textures.size())) {
                m_lastSelectedPath = m_textures[m_selectedIndex].path;
                m_needsReload = true;
            }
        } else if (m_currentState == MOVEMENT_ROOT) {
            if (m_selectedIndex == 0) {
                m_currentState = MOVEMENT_CUBE;
                m_selectedIndex = 0;
            } else if (m_selectedIndex == 1) {
                m_currentState = MOVEMENT_LIGHT;
                m_selectedIndex = 0;
            }
        } else if (m_currentState == MOVEMENT_CUBE) {
            if (m_selectedIndex == 0) {
                m_cubeControlAction = CUBE_PREV;
                m_needsCubeUpdate = true;
            } else if (m_selectedIndex == 1) {
                m_cubeControlAction = CUBE_NEXT;
                m_needsCubeUpdate = true;
            } else if (m_selectedIndex == 3) {
                m_cubeControlAction = CUBE_RESET;
                m_needsCubeUpdate = true;
            } else if (m_selectedIndex == 4) {
                m_cubeControlAction = CUBE_SPIN;
                m_needsCubeUpdate = true;
            } else if (m_selectedIndex == 5) {
                m_cubeControlAction = CUBE_STOP;
                m_needsCubeUpdate = true;
            } else if (m_selectedIndex == 7) {
                m_cubeControlAction = CUBE_X_INC;
                m_needsCubeUpdate = true;
            } else if (m_selectedIndex == 8) {
                m_cubeControlAction = CUBE_X_DEC;
                m_needsCubeUpdate = true;
            } else if (m_selectedIndex == 9) {
                m_cubeControlAction = CUBE_Y_INC;
                m_needsCubeUpdate = true;
            } else if (m_selectedIndex == 10) {
                m_cubeControlAction = CUBE_Y_DEC;
                m_needsCubeUpdate = true;
            } else if (m_selectedIndex == 11) {
                m_cubeControlAction = CUBE_Z_INC;
                m_needsCubeUpdate = true;
            } else if (m_selectedIndex == 12) {
                m_cubeControlAction = CUBE_Z_DEC;
                m_needsCubeUpdate = true;
            }
        } else if (m_currentState == MOVEMENT_LIGHT) {
            if (m_selectedIndex == 0) {
                m_lightControlAction = LIGHT_RESET;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 1) {
                m_lightControlAction = LIGHT_SPIN;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 2) {
                m_lightControlAction = LIGHT_STOP;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 4) {
                m_lightControlAction = LIGHT_X_INC;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 5) {
                m_lightControlAction = LIGHT_X_DEC;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 6) {
                m_lightControlAction = LIGHT_Y_INC;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 7) {
                m_lightControlAction = LIGHT_Y_DEC;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 8) {
                m_lightControlAction = LIGHT_Z_INC;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 9) {
                m_lightControlAction = LIGHT_Z_DEC;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 11) {
                m_lightControlAction = LIGHT_XY_UP;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 12) {
                m_lightControlAction = LIGHT_XY_DOWN;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 13) {
                m_lightControlAction = LIGHT_XY_LEFT;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 14) {
                m_lightControlAction = LIGHT_XY_RIGHT;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 16) {
                m_lightControlAction = LIGHT_XZ_FORWARD;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 17) {
                m_lightControlAction = LIGHT_XZ_BACK;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 18) {
                m_lightControlAction = LIGHT_XZ_LEFT;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 19) {
                m_lightControlAction = LIGHT_XZ_RIGHT;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 21) {
                m_lightControlAction = LIGHT_YZ_UP;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 22) {
                m_lightControlAction = LIGHT_YZ_DOWN;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 23) {
                m_lightControlAction = LIGHT_YZ_FORWARD;
                m_needsLightUpdate = true;
            } else if (m_selectedIndex == 24) {
                m_lightControlAction = LIGHT_YZ_BACK;
                m_needsLightUpdate = true;
            }
        } else if (m_currentState == LIGHTING) {
            if (m_selectedIndex == 0) {
                m_dirLightControlAction = DIRLIGHT_TOGGLE;
                m_needsDirLightUpdate = true;
            } else if (m_selectedIndex == 1) {
                m_dirLightControlAction = DIRLIGHT_ROTATE_LEFT;
                m_needsDirLightUpdate = true;
            } else if (m_selectedIndex == 2) {
                m_dirLightControlAction = DIRLIGHT_ROTATE_RIGHT;
                m_needsDirLightUpdate = true;
            } else if (m_selectedIndex == 3) {
                m_dirLightControlAction = DIRLIGHT_TILT_UP;
                m_needsDirLightUpdate = true;
            } else if (m_selectedIndex == 4) {
                m_dirLightControlAction = DIRLIGHT_TILT_DOWN;
                m_needsDirLightUpdate = true;
            } else if (m_selectedIndex == 5) {
                // Toggle Point Light
                m_dirLightControlAction = POINTLIGHT_TOGGLE;
                m_needsDirLightUpdate = true;
            }
        }
    } else if (key == GLFW_KEY_ESCAPE) {
        if (m_currentState == MAIN_MENU) {
            m_isOpen = false;
        } else if (m_currentState == TEXTURES || m_currentState == MOVEMENT_ROOT) {
            m_currentState = MAIN_MENU;
            m_selectedIndex = 0;
        } else if (m_currentState == MOVEMENT_CUBE || m_currentState == MOVEMENT_LIGHT) {
            m_currentState = MOVEMENT_ROOT;
            m_selectedIndex = 0;
        } else if (m_currentState == LIGHTING) {
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

Menu::CubeControlAction Menu::getCubeControlAction() {
    return m_cubeControlAction;
}

bool Menu::needsCubeUpdate() {
    return m_needsCubeUpdate;
}

void Menu::markCubeUpdated() {
    m_needsCubeUpdate = false;
    m_cubeControlAction = CUBE_NONE;
}

Menu::LightControlAction Menu::getLightControlAction() {
    return m_lightControlAction;
}

bool Menu::needsLightUpdate() {
    return m_needsLightUpdate;
}

void Menu::markLightUpdated() {
    m_needsLightUpdate = false;
    m_lightControlAction = LIGHT_NONE;
}

void Menu::setLightPosition(float x, float y, float z) {
    m_lightPosX = x;
    m_lightPosY = y;
    m_lightPosZ = z;
}

void Menu::getLightPosition(float& x, float& y, float& z) {
    x = m_lightPosX;
    y = m_lightPosY;
    z = m_lightPosZ;
}

void Menu::renderLightingMenu() {
    float x = 200.0f;
    float y = 200.0f;
    float lineHeight = 14.0f * 1.5f;
    
    std::string header = "Lighting Control:\n\n";
    UIText::renderText(header, x, y, 1.5f);
    y += lineHeight * 2;
    
    std::vector<std::string> items = {
        "Toggle Directional",
        "DirLight Rotate Left",
        "DirLight Rotate Right",
        "DirLight Tilt Up",
        "DirLight Tilt Down",
        "Toggle Point"
    };
    
    for (size_t i = 0; i < items.size(); i++) {
        std::string itemText;
        if (static_cast<int>(i) == m_selectedIndex) {
            itemText = "> " + items[i];
            UIText::renderTextWithColor(itemText, x, y, 1.5f, 1.0f, 1.0f, 0.0f);
        } else {
            itemText = "  " + items[i];
            UIText::renderText(itemText, x, y, 1.5f);
        }
        y += lineHeight;
    }
    
    y += lineHeight;
    std::string instructions = "\nArrow Keys: Navigate\nEnter: Select\nESC: Back\nF8: Close";
    UIText::renderText(instructions, x, y, 1.5f);
}

Menu::DirLightControlAction Menu::getDirLightControlAction() {
    return m_dirLightControlAction;
}

bool Menu::needsDirLightUpdate() {
    return m_needsDirLightUpdate;
}

void Menu::markDirLightUpdated() {
    m_needsDirLightUpdate = false;
    m_dirLightControlAction = DIRLIGHT_NONE;
}

void Menu::setCubePosition(float x, float y, float z) {
    m_cubePosX = x;
    m_cubePosY = y;
    m_cubePosZ = z;
}

void Menu::getCubePosition(float& x, float& y, float& z) {
    x = m_cubePosX;
    y = m_cubePosY;
    z = m_cubePosZ;
}

void Menu::setSelectedCubeIndex(int index) {
    m_selectedCubeIndex = index;
}

int Menu::getSelectedCubeIndex() {
    return m_selectedCubeIndex;
}
