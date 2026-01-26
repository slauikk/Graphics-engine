#ifndef MENU_H
#define MENU_H

#include <string>
#include <vector>

class Menu {
public:
    enum MenuState {
        MAIN_MENU,
        TEXTURES,
        MOVEMENT_ROOT,
        MOVEMENT_CUBE,
        MOVEMENT_LIGHT,
        LIGHTING
    };
    
    enum MovementState {
        MOVEMENT_STOPPED,
        MOVEMENT_SPINNING,
        MOVEMENT_RESET
    };
    
    enum LightControlAction {
        LIGHT_NONE,
        LIGHT_X_INC,
        LIGHT_X_DEC,
        LIGHT_Y_INC,
        LIGHT_Y_DEC,
        LIGHT_Z_INC,
        LIGHT_Z_DEC,
        LIGHT_RESET,
        LIGHT_SPIN,
        LIGHT_STOP,
        // Plane movements
        LIGHT_XY_UP,      // Increase Y in XY plane
        LIGHT_XY_DOWN,    // Decrease Y in XY plane
        LIGHT_XY_LEFT,    // Decrease X in XY plane
        LIGHT_XY_RIGHT,   // Increase X in XY plane
        LIGHT_XZ_FORWARD, // Increase Z in XZ plane
        LIGHT_XZ_BACK,    // Decrease Z in XZ plane
        LIGHT_XZ_LEFT,    // Decrease X in XZ plane
        LIGHT_XZ_RIGHT,   // Increase X in XZ plane
        LIGHT_YZ_UP,      // Increase Y in YZ plane
        LIGHT_YZ_DOWN,    // Decrease Y in YZ plane
        LIGHT_YZ_FORWARD, // Increase Z in YZ plane
        LIGHT_YZ_BACK     // Decrease Z in YZ plane
    };
    
    enum DirLightControlAction {
        DIRLIGHT_NONE,
        DIRLIGHT_TOGGLE,
        DIRLIGHT_ROTATE_LEFT,
        DIRLIGHT_ROTATE_RIGHT,
        DIRLIGHT_TILT_UP,
        DIRLIGHT_TILT_DOWN,
        POINTLIGHT_TOGGLE
    };

    enum CubeControlAction {
        CUBE_NONE,
        CUBE_RESET,
        CUBE_SPIN,
        CUBE_STOP,
        CUBE_X_INC,
        CUBE_X_DEC,
        CUBE_Y_INC,
        CUBE_Y_DEC,
        CUBE_Z_INC,
        CUBE_Z_DEC,
        CUBE_PREV,
        CUBE_NEXT
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

    static CubeControlAction getCubeControlAction();
    static bool needsCubeUpdate();
    static void markCubeUpdated();
    static void setCubePosition(float x, float y, float z);
    static void getCubePosition(float& x, float& y, float& z);
    static void setSelectedCubeIndex(int index);
    static int getSelectedCubeIndex();

    static LightControlAction getLightControlAction();
    static bool needsLightUpdate();
    static void markLightUpdated();
    static void setLightPosition(float x, float y, float z);
    static void getLightPosition(float& x, float& y, float& z);
    
    static DirLightControlAction getDirLightControlAction();
    static bool needsDirLightUpdate();
    static void markDirLightUpdated();
    
private:
    static bool m_isOpen;
    static MenuState m_currentState;
    static int m_selectedIndex;
    static std::vector<TextureOption> m_textures;
    static bool m_needsReload;
    static std::string m_lastSelectedPath;
    static MovementState m_movementState;
    static bool m_needsMovementUpdate;

    static CubeControlAction m_cubeControlAction;
    static bool m_needsCubeUpdate;
    static float m_cubePosX, m_cubePosY, m_cubePosZ;
    static int m_selectedCubeIndex;

    static LightControlAction m_lightControlAction;
    static bool m_needsLightUpdate;
    static float m_lightPosX, m_lightPosY, m_lightPosZ;
    
    static DirLightControlAction m_dirLightControlAction;
    static bool m_needsDirLightUpdate;
    
    static void scanTextures();
    static void renderMainMenu();
    static void renderTexturesMenu();
    static void renderMovementRootMenu();
    static void renderCubeMenu();
    static void renderLightMenu();
    static void renderLightingMenu();
};

#endif // MENU_H
