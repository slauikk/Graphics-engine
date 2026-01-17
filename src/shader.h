#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader {
public:
    GLuint m_id;
    
    Shader(const char* vertexSource, const char* fragmentSource);
    ~Shader();
    
    void use() const;
    void setMat4(const char* name, const glm::mat4& mat) const;
    
private:
    static GLuint compile(GLenum type, const char* source);
    static bool checkLink(GLuint program);
};

#endif // SHADER_H
