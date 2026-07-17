#include "shader.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cctype>
#include <cstring>
#include <utility>
#include <glm/gtc/type_ptr.hpp>

GLuint Shader::compile(GLenum type, const char* source) {
    if (source == nullptr) {
        std::cerr << "[Shader] compile() called with null source\n";
        return 0;
    }
    
    // Check if source looks like a file path (contains '/' or '\') but only if it's short
    // Long shader code might contain '/' in comments, so check length first
    size_t len = strlen(source);
    if (len < 50 && (strchr(source, '/') != nullptr || strchr(source, '\\') != nullptr)) {
        // Check if it looks like a path (no newlines, starts with letter or dot)
        bool looksLikePath = true;
        for (size_t i = 0; i < len; i++) {
            if (source[i] == '\n' || source[i] == '\r') {
                looksLikePath = false;
                break;
            }
        }
        if (looksLikePath && (std::isalpha(static_cast<unsigned char>(source[0])) || source[0] == '.' || source[0] == '/')) {
            std::cerr << "[Shader] ERROR: compile() received what looks like a file path instead of shader source!\n";
            std::cerr << "[Shader] Received: " << source << "\n";
            std::cerr << "[Shader] This means readTextFile() failed to read the file!\n";
            return 0;
        }
    }
    
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
        
        // Debug: print first 100 chars of source
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

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) 
    : m_id(0), m_vertexPath(vertexPath), m_fragmentPath(fragmentPath) {
    loadFromFiles(vertexPath, fragmentPath);
}

Shader::Shader(const char* vertexSource, const char* fragmentSource) : m_id(0) {
    // Check if these look like file paths (not shader code)
    if (vertexSource != nullptr && fragmentSource != nullptr) {
        size_t vlen = strlen(vertexSource);
        size_t flen = strlen(fragmentSource);
        
        // If both are short and contain path separators, they're probably file paths
        if ((vlen < 50 && (strchr(vertexSource, '/') != nullptr || strchr(vertexSource, '\\') != nullptr)) &&
            (flen < 50 && (strchr(fragmentSource, '/') != nullptr || strchr(fragmentSource, '\\') != nullptr))) {
            // Check if they don't contain shader keywords
            if (strstr(vertexSource, "#version") == nullptr && strstr(fragmentSource, "#version") == nullptr) {
                // These look like file paths, not shader code - convert to file-based loading
                m_vertexPath = vertexSource;
                m_fragmentPath = fragmentSource;
                loadFromFiles(m_vertexPath, m_fragmentPath);
                return;
            }
        }
    }
    
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

std::string Shader::readTextFile(const std::string& path) {
    if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        std::cerr << "[Shader] Failed to read file: " << path << "\n";
        return "";
    }
    
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[Shader] Failed to open file: " << path << "\n";
        return "";
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size == 0) {
        return "";
    }
    std::string content(size, '\0');
    file.read(&content[0], size);
    file.close();

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

bool Shader::loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexSource = readTextFile(vertexPath);
    std::string fragmentSource = readTextFile(fragmentPath);
    
    if (vertexSource.empty() || fragmentSource.empty()) {
        std::cerr << "[Shader] Failed to load shader files. Vertex empty: " << vertexSource.empty() 
                  << ", Fragment empty: " << fragmentSource.empty() << "\n";
        std::cerr << "[Shader] Vertex path: " << vertexPath << "\n";
        std::cerr << "[Shader] Fragment path: " << fragmentPath << "\n";
        return false;
    }
    
    // Debug: verify we got actual shader code, not paths
    if (vertexSource.find("#version") == std::string::npos) {
        std::cerr << "[Shader] ERROR: Vertex source doesn't contain '#version'! First 100 chars:\n";
        std::cerr << vertexSource.substr(0, 100) << "\n";
        return false;
    }
    if (fragmentSource.find("#version") == std::string::npos) {
        std::cerr << "[Shader] ERROR: Fragment source doesn't contain '#version'! First 100 chars:\n";
        std::cerr << fragmentSource.substr(0, 100) << "\n";
        return false;
    }
    
    // Debug: check if source is valid
    if (vertexSource.size() < 10) {
        std::cerr << "[Shader] Vertex source too short: " << vertexSource.size() << " bytes\n";
        return false;
    }
    if (fragmentSource.size() < 10) {
        std::cerr << "[Shader] Fragment source too short: " << fragmentSource.size() << " bytes\n";
        return false;
    }
    
    // Don't add null terminator - c_str() already provides it
    // But ensure the string doesn't have embedded nulls that would break c_str()
    
    GLuint vertexShader = compile(GL_VERTEX_SHADER, vertexSource.c_str());
    if (vertexShader == 0) {
        std::cerr << "[Shader] Vertex shader compilation failed for: " << vertexPath << "\n";
        return false;
    }
    
    GLuint fragmentShader = compile(GL_FRAGMENT_SHADER, fragmentSource.c_str());
    if (fragmentShader == 0) {
        std::cerr << "[Shader] Fragment shader compilation failed for: " << fragmentPath << "\n";
        glDeleteShader(vertexShader);
        return false;
    }
    
    GLuint newProgram = glCreateProgram();
    glAttachShader(newProgram, vertexShader);
    glAttachShader(newProgram, fragmentShader);
    glLinkProgram(newProgram);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    if (!checkLink(newProgram)) {
        std::cerr << "[Shader] Program linking failed\n";
        glDeleteProgram(newProgram);
        return false;
    }
    
    if (m_id != 0) {
        glDeleteProgram(m_id);
    }
    
    m_id = newProgram;
    m_uniformCache.clear();
    m_uniformWarned.clear();
    
    return true;
}

bool Shader::reload() {
    if (m_vertexPath.empty() || m_fragmentPath.empty()) {
        return false;
    }
    return loadFromFiles(m_vertexPath, m_fragmentPath);
}
