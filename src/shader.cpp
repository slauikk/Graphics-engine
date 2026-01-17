#include "shader.h"
#include <iostream>
#include <vector>
#include <glm/gtc/type_ptr.hpp>

GLuint Shader::compile(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength);
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        std::cerr << "Shader Compile error: " << log.data() << "\n";
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

bool Shader::checkLink(GLuint program) {
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength);
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        std::cerr << "Link error: " << log.data() << "\n";
        return false;
    }
    return true;
}

Shader::Shader(const char* vertexSource, const char* fragmentSource) : m_id(0) {
    GLuint vertexShader = compile(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0) {
        return;
    }
    
    GLuint fragmentShader = compile(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return;
    }
    
    m_id = glCreateProgram();
    glAttachShader(m_id, vertexShader);
    glAttachShader(m_id, fragmentShader);
    glLinkProgram(m_id);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    if (!checkLink(m_id)) {
        glDeleteProgram(m_id);
        m_id = 0;
    }
}

Shader::~Shader() {
    if (m_id != 0) {
        glDeleteProgram(m_id);
    }
}

void Shader::use() const {
    glUseProgram(m_id);
}

void Shader::setMat4(const char* name, const glm::mat4& mat) const {
    GLint location = glGetUniformLocation(m_id, name);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat));
    }
}

void Shader::setInt(const char* name, int value) const {
    GLint location = glGetUniformLocation(m_id, name);
    if (location != -1) {
        glUniform1i(location, value);
    }
}
