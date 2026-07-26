#include "shader.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <utility>
#include <glm/gtc/type_ptr.hpp>

namespace {

std::string shaderInfoLog(GLuint shader) {
    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength <= 0) {
        return {};
    }

    std::vector<GLchar> log(static_cast<std::size_t>(logLength), '\0');
    GLsizei written = 0;
    glGetShaderInfoLog(shader, logLength, &written, log.data());
    return std::string(log.data(), static_cast<std::size_t>(written > 0 ? written : 0));
}

std::string programInfoLog(GLuint program) {
    GLint logLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength <= 0) {
        return {};
    }

    std::vector<GLchar> log(static_cast<std::size_t>(logLength), '\0');
    GLsizei written = 0;
    glGetProgramInfoLog(program, logLength, &written, log.data());
    return std::string(log.data(), static_cast<std::size_t>(written > 0 ? written : 0));
}

} // namespace

GLuint Shader::compile(GLenum type, const char* source) {
    if (source == nullptr) {
        std::cerr << "[Shader] compile() called with null source\n";
        return 0;
    }

    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        std::cerr << "[Shader] Failed to allocate shader object\n";
        return 0;
    }

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        const std::string log = shaderInfoLog(shader);
        std::cerr << "Shader compile error: "
                  << (log.empty() ? "<no driver log>" : log) << "\n";

        std::cerr << "[Shader] Source preview (first 100 chars):\n";
        for (int i = 0; i < 100 && source[i] != '\0'; i++) {
            char c = source[i];
            if (c >= 32 && c < 127) {
                std::cerr << c;
            } else if (c == '\n') {
                std::cerr << "\\n\n";
            } else if (c == '\r') {
                std::cerr << "\\r";
            } else {
                std::cerr << "[" << static_cast<int>(static_cast<unsigned char>(c)) << "]";
            }
        }
        std::cerr << "\n";

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

bool Shader::checkLink(GLuint program) {
    if (program == 0) {
        return false;
    }

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        const std::string log = programInfoLog(program);
        std::cerr << "Link error: " << (log.empty() ? "<no driver log>" : log) << "\n";
        return false;
    }
    return true;
}

Shader::Shader(const std::filesystem::path& vertexPath,
               const std::filesystem::path& fragmentPath)
    : m_id(0), m_vertexPath(vertexPath), m_fragmentPath(fragmentPath) {
    loadFromFiles(vertexPath, fragmentPath);
}

Shader::Shader(const char* vertexSource, const char* fragmentSource) : m_id(0) {
    loadFromSource(vertexSource, fragmentSource);
}

Shader::~Shader() {
    if (m_id != 0) {
        glDeleteProgram(m_id);
    }
}

Shader::Shader(Shader&& other) noexcept
    : m_id(other.m_id),
      m_vertexPath(std::move(other.m_vertexPath)),
      m_fragmentPath(std::move(other.m_fragmentPath)),
      m_uniformCache(std::move(other.m_uniformCache)),
      m_uniformWarned(std::move(other.m_uniformWarned)) {
    other.m_id = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_id != 0) {
            GLint currentProgram = 0;
            glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
            if (static_cast<GLuint>(currentProgram) == m_id) {
                glUseProgram(other.m_id);
            }
            glDeleteProgram(m_id);
        }

        m_id = other.m_id;
        m_vertexPath = std::move(other.m_vertexPath);
        m_fragmentPath = std::move(other.m_fragmentPath);
        m_uniformCache = std::move(other.m_uniformCache);
        m_uniformWarned = std::move(other.m_uniformWarned);
        other.m_id = 0;
    }
    return *this;
}

void Shader::use() const {
    glUseProgram(m_id);
}

GLint Shader::getUniformLocation(const std::string& name) const {
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end()) {
        return it->second;
    }
    
    GLint location = glGetUniformLocation(m_id, name.c_str());
    m_uniformCache[name] = location;
    
    if (location == -1 && m_uniformWarned.find(name) == m_uniformWarned.end()) {
        std::cerr << "[Shader] Uniform '" << name << "' not found (warning logged once)\n";
        m_uniformWarned[name] = true;
    }
    
    return location;
}

void Shader::setMat4(const char* name, const glm::mat4& mat) const {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat));
    }
}

void Shader::setInt(const char* name, int value) const {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform1i(location, value);
    }
}

void Shader::setVec3(const char* name, float x, float y, float z) const {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform3f(location, x, y, z);
    }
}

void Shader::setVec3(const char* name, const glm::vec3& vec) const {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform3f(location, vec.x, vec.y, vec.z);
    }
}

void Shader::setFloat(const char* name, float value) const {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform1f(location, value);
    }
}

std::string Shader::readTextFile(const std::filesystem::path& path) {
    std::error_code filesystemError;
    const bool isRegularFile = std::filesystem::is_regular_file(path, filesystemError);
    if (filesystemError || !isRegularFile) {
        std::cerr << "[Shader] Failed to read file: " << path << "\n";
        return "";
    }

    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[Shader] Failed to open file: " << path << "\n";
        return "";
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    if (file.bad()) {
        std::cerr << "[Shader] Failed while reading file: " << path << "\n";
        return "";
    }

    std::string content = buffer.str();

    // Remove BOM if present (UTF-8 BOM: EF BB BF)
    if (content.size() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        content = content.substr(3);
    }

    // Remove null terminators that might be at the end
    while (!content.empty() && content.back() == '\0') {
        content.pop_back();
    }

    if (content.empty()) {
        std::cerr << "[Shader] File is empty: " << path << "\n";
    }

    return content;
}

GLuint Shader::buildProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = compile(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0) {
        return 0;
    }

    GLuint fragmentShader = compile(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program == 0) {
        std::cerr << "[Shader] Failed to allocate program object\n";
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (!checkLink(program)) {
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

bool Shader::replaceProgram(GLuint newProgram) {
    if (newProgram == 0) {
        return false;
    }

    const GLuint oldProgram = m_id;
    if (oldProgram == newProgram) {
        return true;
    }

    if (oldProgram != 0) {
        GLint currentProgram = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
        if (static_cast<GLuint>(currentProgram) == oldProgram) {
            glUseProgram(newProgram);
        }
    }

    m_id = newProgram;
    if (oldProgram != 0) {
        glDeleteProgram(oldProgram);
    }

    m_uniformCache.clear();
    m_uniformWarned.clear();
    return true;
}

bool Shader::loadFromSource(const char* vertexSource, const char* fragmentSource) {
    return replaceProgram(buildProgram(vertexSource, fragmentSource));
}

bool Shader::loadFromFiles(const std::filesystem::path& vertexPath,
                           const std::filesystem::path& fragmentPath) {
    std::string vertexSource = readTextFile(vertexPath);
    std::string fragmentSource = readTextFile(fragmentPath);

    if (vertexSource.empty() || fragmentSource.empty()) {
        std::cerr << "[Shader] Failed to load shader files. Vertex empty: " << vertexSource.empty()
                  << ", Fragment empty: " << fragmentSource.empty() << "\n";
        std::cerr << "[Shader] Vertex path: " << vertexPath << "\n";
        std::cerr << "[Shader] Fragment path: " << fragmentPath << "\n";
        return false;
    }

    if (vertexSource.find('\0') != std::string::npos ||
        fragmentSource.find('\0') != std::string::npos) {
        std::cerr << "[Shader] Shader file contains an embedded null byte\n";
        return false;
    }

    if (!loadFromSource(vertexSource.c_str(), fragmentSource.c_str())) {
        std::cerr << "[Shader] Failed to build program from " << vertexPath
                  << " and " << fragmentPath << "\n";
        return false;
    }

    return true;
}

bool Shader::reload() {
    if (m_vertexPath.empty() || m_fragmentPath.empty()) {
        return false;
    }
    return loadFromFiles(m_vertexPath, m_fragmentPath);
}
