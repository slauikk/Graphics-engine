#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

class Shader {
public:
    GLuint m_id;
    
    // Constructor for file paths (must be explicit to avoid ambiguity)
    explicit Shader(const std::string& vertexPath, const std::string& fragmentPath);
    // Constructor for inline shader source code
    Shader(const char* vertexSource, const char* fragmentSource);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;
    
    void use() const;
    void setMat4(const char* name, const glm::mat4& mat) const;
    void setInt(const char* name, int value) const;
    void setVec3(const char* name, float x, float y, float z) const;
    void setVec3(const char* name, const glm::vec3& vec) const;
    void setFloat(const char* name, float value) const;
    
    bool reload();
    
private:
    std::string m_vertexPath;
    std::string m_fragmentPath;
    mutable std::unordered_map<std::string, GLint> m_uniformCache;
    mutable std::unordered_map<std::string, bool> m_uniformWarned;
    
    GLint getUniformLocation(const std::string& name) const;
    static std::string readTextFile(const std::string& path);
    static GLuint compile(GLenum type, const char* source);
    static bool checkLink(GLuint program);
    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
};

#endif // SHADER_H
