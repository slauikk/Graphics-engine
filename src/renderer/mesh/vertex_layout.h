#ifndef VERTEX_LAYOUT_H
#define VERTEX_LAYOUT_H

#include <glad/glad.h>
#include <cstddef>
#include <string>
#include <vector>
#include <cstdint>

enum class ShaderDataType {
    Float, Float2, Float3, Float4,
    Int, Int2, Int3, Int4
};

struct BufferElement {
    std::string name;
    ShaderDataType type;
    uint32_t size;
    size_t offset;
    bool normalized;

    BufferElement(ShaderDataType t, const std::string& n, bool norm = false)
        : name(n), type(t), normalized(norm), size(getSize(t)), offset(0) {}

    static uint32_t getSize(ShaderDataType type) {
        switch (type) {
            case ShaderDataType::Float:  return 4;
            case ShaderDataType::Float2: return 8;
            case ShaderDataType::Float3: return 12;
            case ShaderDataType::Float4: return 16;
            case ShaderDataType::Int:    return 4;
            case ShaderDataType::Int2:   return 8;
            case ShaderDataType::Int3:   return 12;
            case ShaderDataType::Int4:   return 16;
        }
        return 0;
    }

    static GLenum getGLType(ShaderDataType type) {
        switch (type) {
            case ShaderDataType::Float:
            case ShaderDataType::Float2:
            case ShaderDataType::Float3:
            case ShaderDataType::Float4:
                return GL_FLOAT;
            case ShaderDataType::Int:
            case ShaderDataType::Int2:
            case ShaderDataType::Int3:
            case ShaderDataType::Int4:
                return GL_INT;
        }
        return GL_FLOAT;
    }

    static bool isIntegerType(ShaderDataType type) {
        switch (type) {
            case ShaderDataType::Int:
            case ShaderDataType::Int2:
            case ShaderDataType::Int3:
            case ShaderDataType::Int4:
                return true;
            default:
                return false;
        }
    }

    static uint32_t getComponentCount(ShaderDataType type) {
        switch (type) {
            case ShaderDataType::Float:
            case ShaderDataType::Int:
                return 1;
            case ShaderDataType::Float2:
            case ShaderDataType::Int2:
                return 2;
            case ShaderDataType::Float3:
            case ShaderDataType::Int3:
                return 3;
            case ShaderDataType::Float4:
            case ShaderDataType::Int4:
                return 4;
        }
        return 0;
    }
};

class BufferLayout {
public:
    BufferLayout() = default;
    BufferLayout(std::initializer_list<BufferElement> elements)
        : m_elements(elements) {
        calculateOffsetsAndStride();
    }

    const std::vector<BufferElement>& getElements() const { return m_elements; }
    uint32_t getStride() const { return m_stride; }

private:
    void calculateOffsetsAndStride() {
        size_t offset = 0;
        m_stride = 0;
        for (auto& element : m_elements) {
            element.offset = offset;
            offset += element.size;
            m_stride += element.size;
        }
    }

    std::vector<BufferElement> m_elements;
    uint32_t m_stride = 0;
};

#endif // VERTEX_LAYOUT_H
