#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <memory>
#include <string>

class Texture2D;
class Shader;

namespace ResourceManager {

    std::shared_ptr<Texture2D> getTexture(const std::string& relativePath);
    std::shared_ptr<Shader> getShader(const std::string& vertRel, const std::string& fragRel);
    bool reloadAllShaders();
    void clear();

} // namespace ResourceManager

#endif // RESOURCE_MANAGER_H
