#ifndef TEXTURE_MENU_H
#define TEXTURE_MENU_H

#include <string>
#include <vector>

class TextureMenu {
public:
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
    
private:
    static bool m_isOpen;
    static int m_selectedIndex;
    static std::vector<TextureOption> m_textures;
    static bool m_needsReload;
    static std::string m_lastSelectedPath;
    
    static void scanTextures();
};

#endif // TEXTURE_MENU_H
