#ifndef MENU_H
#define MENU_H

#include <string>
#include <vector>

class Menu {
public:
    enum MenuState {
        MAIN_MENU,
        TEXTURES,
        MOVEMENT
    };
    
    enum MovementState {
        MOVEMENT_STOPPED,
        MOVEMENT_SPINNING,
        MOVEMENT_RESET
    };
    
    struct TextureOption {
        std::string name;
        std::string path;
        bool isGenerated;
    };
    
    static void init();
    static void update();
    static void render();
    static bool isOpen();
    static void toggle();
    static void processKey(int key);
    static std::string getSelectedTexturePath();
    static bool needsReload();
    static void markReloaded();
    static MovementState getMovementState();
    static bool needsMovementUpdate();
    static void markMovementUpdated();
    
private:
    static bool m_isOpen;
    static MenuState m_currentState;
    static int m_selectedIndex;
    static std::vector<TextureOption> m_textures;
    static bool m_needsReload;
    static std::string m_lastSelectedPath;
    static MovementState m_movementState;
    static bool m_needsMovementUpdate;
    
    static void scanTextures();
    static void renderMainMenu();
    static void renderTexturesMenu();
    static void renderMovementMenu();
};

#endif // MENU_H
