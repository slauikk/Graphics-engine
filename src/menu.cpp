#include "menu.h"
#include "ui_text.h"
#include "core/asset_paths.h"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <utility>

namespace {

const std::vector<std::string> MAIN_MENU_ITEMS = {
    "Textures", "Models", "Movement", "Lighting"
};

const std::vector<std::string> MOVEMENT_ROOT_ITEMS = {
    "Object", "Light"
};

const std::vector<std::string> CUBE_MENU_ITEMS = {
    "Previous Object",
    "Next Object",
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

const std::vector<std::string> LIGHT_MENU_ITEMS = {
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

const std::vector<std::string> LIGHTING_MENU_ITEMS = {
    "Toggle Directional",
    "DirLight Rotate Left",
    "DirLight Rotate Right",
    "DirLight Tilt Up",
    "DirLight Tilt Down",
    "Toggle Point"
};

constexpr std::size_t kVisibleModelRows = 12;
constexpr float kDockTextScale = 1.0f;
constexpr float kDockTitleScale = 1.05f;
constexpr float kDockHeaderHeight = 28.0f;
constexpr float kDockFooterHeight = 24.0f;
constexpr float kDockRowHeight = 22.0f;

const char* titleForState(Menu::MenuState state) {
    switch (state) {
        case Menu::MAIN_MENU: return "Content & Tools";
        case Menu::TEXTURES: return "Textures";
        case Menu::MODELS: return "Models";
        case Menu::MOVEMENT_ROOT: return "Movement";
        case Menu::MOVEMENT_CUBE: return "Object Control";
        case Menu::MOVEMENT_LIGHT: return "Point Light Control";
        case Menu::LIGHTING: return "Scene Lighting";
    }
    return "Content & Tools";
}

std::size_t dockVisibleRowCapacity(float height) {
    const float available = height - kDockHeaderHeight - kDockFooterHeight;
    return available >= kDockRowHeight
        ? static_cast<std::size_t>(available / kDockRowHeight)
        : 0;
}

std::size_t dockFirstVisibleItem(
    std::size_t itemCount,
    int selectedIndex,
    std::size_t capacity) {
    if (itemCount == 0 || capacity == 0 || itemCount <= capacity) {
        return 0;
    }
    const std::size_t selected = static_cast<std::size_t>(std::clamp(
        selectedIndex, 0, static_cast<int>(itemCount - 1)));
    const std::size_t halfWindow = capacity / 2;
    const std::size_t centered = selected > halfWindow
        ? selected - halfWindow
        : 0;
    return (std::min)(centered, itemCount - capacity);
}

std::string ellipsizeMenuText(
    const std::string& value,
    float maximumWidth,
    float scale = kDockTextScale) {
    if (maximumWidth <= 0.0f ||
        UIText::measureTextWidth(value, scale) <= maximumWidth) {
        return maximumWidth > 0.0f ? value : std::string{};
    }

    constexpr std::string_view suffix = "...";
    if (UIText::measureTextWidth(std::string(suffix), scale) > maximumWidth) {
        return {};
    }
    std::size_t end = value.size();
    while (end > 0) {
        --end;
        while (end > 0 &&
               (static_cast<unsigned char>(value[end]) & 0xc0U) == 0x80U) {
            --end;
        }
        std::string candidate = value.substr(0, end) + std::string(suffix);
        if (UIText::measureTextWidth(candidate, scale) <= maximumWidth) {
            return candidate;
        }
    }
    return std::string(suffix);
}

int itemCountForState(Menu::MenuState state, int optionCount) {
    switch (state) {
        case Menu::MAIN_MENU:
            return static_cast<int>(MAIN_MENU_ITEMS.size());
        case Menu::TEXTURES:
        case Menu::MODELS:
            return optionCount;
        case Menu::MOVEMENT_ROOT:
            return static_cast<int>(MOVEMENT_ROOT_ITEMS.size());
        case Menu::MOVEMENT_CUBE:
            return static_cast<int>(CUBE_MENU_ITEMS.size());
        case Menu::MOVEMENT_LIGHT:
            return static_cast<int>(LIGHT_MENU_ITEMS.size());
        case Menu::LIGHTING:
            return static_cast<int>(LIGHTING_MENU_ITEMS.size());
    }

    return 0;
}

} // namespace

bool Menu::m_isOpen = false;
Menu::MenuState Menu::m_currentState = MAIN_MENU;
int Menu::m_selectedIndex = 0;
float Menu::m_renderX = 200.0f;
float Menu::m_renderY = 200.0f;
float Menu::m_renderWidth = 480.0f;
float Menu::m_renderHeight = 460.0f;
std::vector<Menu::TextureOption> Menu::m_textures;
std::vector<Menu::ModelOption> Menu::m_models;
bool Menu::m_needsReload = false;
std::string Menu::m_lastSelectedPath = "";
bool Menu::m_needsModelLoad = false;
std::string Menu::m_lastSelectedModelPath = "";
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
    scanModels();
}

void Menu::scanTextures() {
    m_textures.clear();
    
    TextureOption gridOption;
    gridOption.name = "Grid (Generated)";
    gridOption.path = "textures/generated_grid";
    gridOption.isGenerated = true;
    m_textures.push_back(gridOption);
    
    std::filesystem::path texturesDir = core::assetPath("textures");
    
    try {
        if (std::filesystem::exists(texturesDir) && std::filesystem::is_directory(texturesDir)) {
            for (const auto& entry : std::filesystem::directory_iterator(texturesDir)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char character) {
                        return static_cast<char>(std::tolower(character));
                    });
                    
                    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
                        TextureOption option;
                        option.name = entry.path().filename().string();
                        option.path = "textures/" + option.name;
                        option.isGenerated = false;
                        m_textures.push_back(option);
                    }
                }
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "[Menu] Failed to scan textures: " << error.what() << "\n";
    }
    
    if (m_selectedIndex >= static_cast<int>(m_textures.size())) {
        m_selectedIndex = 0;
    }
}

void Menu::scanModels() {
    m_models.clear();

    const std::filesystem::path modelsDirectory = core::assetPath("models");
    try {
        if (std::filesystem::exists(modelsDirectory) &&
            std::filesystem::is_directory(modelsDirectory)) {
            for (const auto& entry : std::filesystem::directory_iterator(modelsDirectory)) {
                if (!entry.is_regular_file()) {
                    continue;
                }

                std::string extension = entry.path().extension().string();
                std::transform(
                    extension.begin(), extension.end(), extension.begin(),
                    [](unsigned char character) {
                        return static_cast<char>(std::tolower(character));
                    });
                if (extension != ".obj" && extension != ".gltf" && extension != ".glb") {
                    continue;
                }

                ModelOption option;
                option.name = entry.path().filename().string();
                option.path = "models/" + option.name;
                m_models.push_back(std::move(option));
            }

            std::sort(
                m_models.begin(), m_models.end(),
                [](const ModelOption& left, const ModelOption& right) {
                    return left.name < right.name;
                });
        }
    } catch (const std::exception& error) {
        std::cerr << "[Menu] Failed to scan models: " << error.what() << "\n";
    }

    if (m_selectedIndex >= static_cast<int>(m_models.size())) {
        m_selectedIndex = 0;
    }
}

void Menu::update() {
    if (m_isOpen) {
        if (m_textures.empty()) {
            scanTextures();
        }
    }
}

void Menu::render() {
    if (!m_isOpen || m_renderWidth <= 0.0f || m_renderHeight <= 0.0f) {
        return;
    }

    const int optionCount = m_currentState == MODELS
        ? static_cast<int>(m_models.size())
        : static_cast<int>(m_textures.size());
    const std::size_t itemCount = static_cast<std::size_t>((std::max)(
        0, itemCountForState(m_currentState, optionCount)));
    const auto itemLabel = [](MenuState state, std::size_t index)
        -> const std::string& {
        static const std::string empty;
        switch (state) {
            case MAIN_MENU:
                return index < MAIN_MENU_ITEMS.size()
                    ? MAIN_MENU_ITEMS[index] : empty;
            case TEXTURES:
                return index < m_textures.size()
                    ? m_textures[index].name : empty;
            case MODELS:
                return index < m_models.size()
                    ? m_models[index].name : empty;
            case MOVEMENT_ROOT:
                return index < MOVEMENT_ROOT_ITEMS.size()
                    ? MOVEMENT_ROOT_ITEMS[index] : empty;
            case MOVEMENT_CUBE:
                return index < CUBE_MENU_ITEMS.size()
                    ? CUBE_MENU_ITEMS[index] : empty;
            case MOVEMENT_LIGHT:
                return index < LIGHT_MENU_ITEMS.size()
                    ? LIGHT_MENU_ITEMS[index] : empty;
            case LIGHTING:
                return index < LIGHTING_MENU_ITEMS.size()
                    ? LIGHTING_MENU_ITEMS[index] : empty;
        }
        return empty;
    };

    UIText::renderTextWithColor(
        titleForState(m_currentState),
        m_renderX,
        m_renderY,
        kDockTitleScale,
        0.96f, 0.61f, 0.20f);

    std::ostringstream context;
    switch (m_currentState) {
        case TEXTURES:
            context << m_textures.size() << " available  |  Enter applies";
            break;
        case MODELS:
            context << m_models.size() << " available  |  Enter imports";
            break;
        case MOVEMENT_CUBE:
            context << "Object " << m_selectedCubeIndex + 1 << "  |  Pos "
                    << std::fixed << std::setprecision(1)
                    << m_cubePosX << ", " << m_cubePosY << ", " << m_cubePosZ;
            break;
        case MOVEMENT_LIGHT:
            context << "Pos " << std::fixed << std::setprecision(1)
                    << m_lightPosX << ", " << m_lightPosY << ", " << m_lightPosZ;
            break;
        default:
            break;
    }
    if (!context.str().empty() && m_renderWidth >= 520.0f) {
        UIText::renderTextWithColor(
            ellipsizeMenuText(context.str(), m_renderWidth - 310.0f, 0.9f),
            m_renderX + 190.0f,
            m_renderY + 1.0f,
            0.9f,
            0.58f, 0.62f, 0.69f);
    }

    const std::size_t capacity = dockVisibleRowCapacity(m_renderHeight);
    const std::size_t firstVisible = dockFirstVisibleItem(
        itemCount, m_selectedIndex, capacity);
    const std::size_t visibleCount = (std::min)(
        capacity, itemCount - (std::min)(firstVisible, itemCount));
    const std::size_t finalVisible = firstVisible + visibleCount;
    if (itemCount > capacity && capacity > 0) {
        std::ostringstream range;
        range << firstVisible + 1 << "-" << finalVisible
              << " / " << itemCount;
        const std::string rangeText = range.str();
        const float rangeWidth = UIText::measureTextWidth(rangeText, 0.9f);
        UIText::renderTextWithColor(
            rangeText,
            m_renderX + m_renderWidth - rangeWidth,
            m_renderY + 1.0f,
            0.9f,
            0.58f, 0.62f, 0.69f);
    }

    for (std::size_t index = firstVisible; index < finalVisible; ++index) {
        const bool selected = static_cast<int>(index) == m_selectedIndex;
        const std::string& rawLabel = itemLabel(m_currentState, index);
        const bool separator = rawLabel == "---";
        const std::string prefix = selected && !separator ? ">  " : "   ";
        const std::string label = separator
            ? "--------------------------------"
            : prefix + rawLabel;
        const float y = m_renderY + kDockHeaderHeight +
            static_cast<float>(index - firstVisible) * kDockRowHeight;
        UIText::renderTextWithColor(
            ellipsizeMenuText(label, m_renderWidth, kDockTextScale),
            m_renderX,
            y,
            kDockTextScale,
            selected && !separator ? 0.96f : (separator ? 0.36f : 0.78f),
            selected && !separator ? 0.61f : (separator ? 0.39f : 0.81f),
            selected && !separator ? 0.20f : (separator ? 0.45f : 0.86f));
    }

    constexpr std::string_view footer =
        "UP/DOWN NAVIGATE  |  ENTER SELECT  |  ESC BACK  |  F8 CLOSE";
    UIText::renderTextWithColor(
        ellipsizeMenuText(
            std::string(footer), m_renderWidth, 0.9f),
        m_renderX,
        m_renderY + m_renderHeight - 18.0f,
        0.9f,
        0.58f, 0.62f, 0.69f);
}

void Menu::renderMainMenu() {
    float x = m_renderX;
    float y = m_renderY;
    float lineHeight = 14.0f * 1.5f;
    
    std::string header = "Main Menu:\n\n";
    UIText::renderText(header, x, y, 1.5f);
    y += lineHeight * 2;
    
    for (size_t i = 0; i < MAIN_MENU_ITEMS.size(); i++) {
        std::string itemText;
        if (static_cast<int>(i) == m_selectedIndex) {
            itemText = "> " + MAIN_MENU_ITEMS[i];
            UIText::renderTextWithColor(itemText, x, y, 1.5f, 1.0f, 1.0f, 0.0f);
        } else {
            itemText = "  " + MAIN_MENU_ITEMS[i];
            UIText::renderText(itemText, x, y, 1.5f);
        }
        y += lineHeight;
    }
    
    y += lineHeight;
    std::string instructions = "\nArrows/W/S: Navigate\nEnter: Select\nESC: Back/Close\nF8: Close";
    UIText::renderText(instructions, x, y, 1.5f);
}

void Menu::renderTexturesMenu() {
    float x = m_renderX;
    float y = m_renderY;
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
    std::string instructions = "\nArrows/W/S: Navigate\nEnter: Select\nESC: Back\nF8: Close";
    UIText::renderText(instructions, x, y, 1.5f);
}

void Menu::renderModelsMenu() {
    float x = m_renderX;
    float y = m_renderY;
    const float lineHeight = 14.0f * 1.5f;

    UIText::renderText("Model Selection:\n\n", x, y, 1.5f);
    y += lineHeight * 2;

    if (m_models.empty()) {
        UIText::renderText("  No .obj/.gltf/.glb models found", x, y, 1.5f);
        y += lineHeight;
    } else {
        std::size_t firstVisible = 0;
        if (m_models.size() > kVisibleModelRows) {
            const std::size_t selected = static_cast<std::size_t>(std::clamp(
                m_selectedIndex, 0, static_cast<int>(m_models.size() - 1)));
            if (selected > kVisibleModelRows / 2) {
                firstVisible = selected - kVisibleModelRows / 2;
            }
            firstVisible = std::min(
                firstVisible, m_models.size() - kVisibleModelRows);
        }
        const std::size_t lastVisible = std::min(
            firstVisible + kVisibleModelRows, m_models.size());

        if (firstVisible > 0) {
            UIText::renderText("  ...", x, y, 1.5f);
            y += lineHeight;
        }
        for (std::size_t index = firstVisible; index < lastVisible; ++index) {
            const bool selected = static_cast<int>(index) == m_selectedIndex;
            const std::string itemText = (selected ? "> " : "  ") + m_models[index].name;
            if (selected) {
                UIText::renderTextWithColor(
                    itemText, x, y, 1.5f, 1.0f, 1.0f, 0.0f);
            } else {
                UIText::renderText(itemText, x, y, 1.5f);
            }
            y += lineHeight;
        }
        if (lastVisible < m_models.size()) {
            UIText::renderText("  ...", x, y, 1.5f);
            y += lineHeight;
        }
    }

    y += lineHeight;
    UIText::renderText("\nArrows/W/S: Navigate\nEnter: Load\nESC: Back\nF8: Close",
                       x, y, 1.5f);
}

void Menu::renderMovementRootMenu() {
    float x = m_renderX;
    float y = m_renderY;
    float lineHeight = 14.0f * 1.5f;
    
    std::string header = "Movement:\n\n";
    UIText::renderText(header, x, y, 1.5f);
    y += lineHeight * 2;
    
    for (size_t i = 0; i < MOVEMENT_ROOT_ITEMS.size(); i++) {
        std::string itemText;
        if (static_cast<int>(i) == m_selectedIndex) {
            itemText = "> " + MOVEMENT_ROOT_ITEMS[i];
            UIText::renderTextWithColor(itemText, x, y, 1.5f, 1.0f, 1.0f, 0.0f);
        } else {
            itemText = "  " + MOVEMENT_ROOT_ITEMS[i];
            UIText::renderText(itemText, x, y, 1.5f);
        }
        y += lineHeight;
    }
    
    y += lineHeight;
    std::string instructions = "\nArrows/W/S: Navigate\nEnter: Select\nESC: Back\nF8: Close";
    UIText::renderText(instructions, x, y, 1.5f);
}

void Menu::renderCubeMenu() {
    float x = m_renderX;
    float y = m_renderY;
    float lineHeight = 14.0f * 1.5f;
    
    std::string header = "Object Control:\n\n";
    UIText::renderText(header, x, y, 1.5f);
    y += lineHeight * 2;
    
    std::ostringstream cubeInfo;
    cubeInfo << "Selected Object: ";
    if (m_selectedCubeIndex >= 0) {
        cubeInfo << (m_selectedCubeIndex + 1);
    } else {
        cubeInfo << "none";
    }
    UIText::renderText(cubeInfo.str(), x, y, 1.5f);
    y += lineHeight * 2;
    
    for (size_t i = 0; i < CUBE_MENU_ITEMS.size(); i++) {
        std::string itemText;
        if (static_cast<int>(i) == m_selectedIndex) {
            itemText = "> " + CUBE_MENU_ITEMS[i];
            UIText::renderTextWithColor(itemText, x, y, 1.5f, 1.0f, 1.0f, 0.0f);
        } else {
            itemText = "  " + CUBE_MENU_ITEMS[i];
            UIText::renderText(itemText, x, y, 1.5f);
        }
        y += lineHeight;
    }
    
    y += lineHeight;
    std::ostringstream posStr;
    posStr << std::fixed << std::setprecision(1);
    posStr << "\nObject Pos: (" << m_cubePosX << ", " << m_cubePosY << ", "
           << m_cubePosZ << ")";
    UIText::renderText(posStr.str(), x, y, 1.5f);
    y += lineHeight * 2;
    
    std::string instructions = "\nArrows/W/S: Navigate\nEnter: Select\nESC: Back\nF8: Close";
    UIText::renderText(instructions, x, y, 1.5f);
}

void Menu::renderLightMenu() {
    float x = m_renderX;
    float y = m_renderY;
    float lineHeight = 14.0f * 1.5f;
    
    std::string header = "Light Control:\n\n";
    UIText::renderText(header, x, y, 1.5f);
    y += lineHeight * 2;
    
    for (size_t i = 0; i < LIGHT_MENU_ITEMS.size(); i++) {
        std::string itemText;
        if (static_cast<int>(i) == m_selectedIndex) {
            itemText = "> " + LIGHT_MENU_ITEMS[i];
            UIText::renderTextWithColor(itemText, x, y, 1.5f, 1.0f, 1.0f, 0.0f);
        } else {
            itemText = "  " + LIGHT_MENU_ITEMS[i];
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
    
    std::string instructions = "\nArrows/W/S: Navigate\nEnter: Select\nESC: Back\nF8: Close";
    UIText::renderText(instructions, x, y, 1.5f);
}

bool Menu::isOpen() {
    return m_isOpen;
}

void Menu::setRenderArea(float x, float y, float width, float height) {
    m_renderX = x;
    m_renderY = y;
    m_renderWidth = width;
    m_renderHeight = height;
}

bool Menu::processClick(float x, float y) {
    if (!m_isOpen || !std::isfinite(x) || !std::isfinite(y) ||
        x < m_renderX || y < m_renderY ||
        x >= m_renderX + m_renderWidth ||
        y >= m_renderY + m_renderHeight) {
        return false;
    }

    const float firstRowY = m_renderY + kDockHeaderHeight;
    const std::size_t capacity = dockVisibleRowCapacity(m_renderHeight);
    if (capacity == 0 || y < firstRowY ||
        y >= firstRowY + static_cast<float>(capacity) * kDockRowHeight) {
        return false;
    }

    const std::size_t visualRow = static_cast<std::size_t>(
        (y - firstRowY) / kDockRowHeight);

    int optionCount = 0;
    if (m_currentState == TEXTURES) {
        optionCount = static_cast<int>(m_textures.size());
    } else if (m_currentState == MODELS) {
        optionCount = static_cast<int>(m_models.size());
    } else {
        optionCount = itemCountForState(m_currentState, 0);
    }
    const std::size_t itemCount = static_cast<std::size_t>((std::max)(
        0, optionCount));
    const std::size_t firstVisible = dockFirstVisibleItem(
        itemCount, m_selectedIndex, capacity);
    const std::size_t itemIndex = firstVisible + visualRow;
    if (itemIndex >= itemCount) {
        return false;
    }

    m_selectedIndex = static_cast<int>(itemIndex);
    processKey(GLFW_KEY_ENTER);
    return true;
}

void Menu::toggle() {
    m_isOpen = !m_isOpen;
    if (m_isOpen) {
        m_currentState = MAIN_MENU;
        m_selectedIndex = 0;
        scanTextures();
        scanModels();
    }
}

void Menu::processKey(int key) {
    if (!m_isOpen) return;
    
    const int optionCount = m_currentState == MODELS
        ? static_cast<int>(m_models.size())
        : static_cast<int>(m_textures.size());
    const int maxItems = itemCountForState(m_currentState, optionCount);

    if (key == GLFW_KEY_UP && maxItems > 0) {
        m_selectedIndex--;
        if (m_selectedIndex < 0) {
            m_selectedIndex = maxItems - 1;
        }
    } else if (key == GLFW_KEY_DOWN && maxItems > 0) {
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
                m_currentState = MODELS;
                m_selectedIndex = 0;
            } else if (m_selectedIndex == 2) {
                m_currentState = MOVEMENT_ROOT;
                m_selectedIndex = 0;
            } else if (m_selectedIndex == 3) {
                m_currentState = LIGHTING;
                m_selectedIndex = 0;
            }
        } else if (m_currentState == TEXTURES) {
            if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_textures.size())) {
                m_lastSelectedPath = m_textures[m_selectedIndex].path;
                m_needsReload = true;
            }
        } else if (m_currentState == MODELS) {
            if (m_selectedIndex >= 0 &&
                m_selectedIndex < static_cast<int>(m_models.size())) {
                m_lastSelectedModelPath = m_models[m_selectedIndex].path;
                m_needsModelLoad = true;
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
        } else if (m_currentState == TEXTURES || m_currentState == MODELS ||
                   m_currentState == MOVEMENT_ROOT) {
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

std::string Menu::getSelectedModelPath() {
    return m_lastSelectedModelPath;
}

bool Menu::needsModelLoad() {
    return m_needsModelLoad;
}

void Menu::markModelLoaded() {
    m_needsModelLoad = false;
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
    float x = m_renderX;
    float y = m_renderY;
    float lineHeight = 14.0f * 1.5f;
    
    std::string header = "Lighting Control:\n\n";
    UIText::renderText(header, x, y, 1.5f);
    y += lineHeight * 2;
    
    for (size_t i = 0; i < LIGHTING_MENU_ITEMS.size(); i++) {
        std::string itemText;
        if (static_cast<int>(i) == m_selectedIndex) {
            itemText = "> " + LIGHTING_MENU_ITEMS[i];
            UIText::renderTextWithColor(itemText, x, y, 1.5f, 1.0f, 1.0f, 0.0f);
        } else {
            itemText = "  " + LIGHTING_MENU_ITEMS[i];
            UIText::renderText(itemText, x, y, 1.5f);
        }
        y += lineHeight;
    }
    
    y += lineHeight;
    std::string instructions = "\nArrows/W/S: Navigate\nEnter: Select\nESC: Back\nF8: Close";
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
