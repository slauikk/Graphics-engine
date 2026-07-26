#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <filesystem>
#include <string>
#include <unordered_map>

class Shader {
public:
    GLuint m_id;
    
    Shader(const std::filesystem::path& vertexPath,
           const std::filesystem::path& fragmentPath);
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

    bool loadFromSource(const char* vertexSource, const char* fragmentSource);
    bool reload();

private:
    std::filesystem::path m_vertexPath;
    std::filesystem::path m_fragmentPath;
    mutable std::unordered_map<std::string, GLint> m_uniformCache;
    mutable std::unordered_map<std::string, bool> m_uniformWarned;

    GLint getUniformLocation(const std::string& name) const;
    static std::string readTextFile(const std::filesystem::path& path);
    static GLuint compile(GLenum type, const char* source);
    static bool checkLink(GLuint program);
    static GLuint buildProgram(const char* vertexSource, const char* fragmentSource);
    bool replaceProgram(GLuint newProgram);
    bool loadFromFiles(const std::filesystem::path& vertexPath,
                       const std::filesystem::path& fragmentPath);
};

#endif // SHADER_H
