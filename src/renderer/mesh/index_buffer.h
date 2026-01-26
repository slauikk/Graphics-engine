#ifndef INDEX_BUFFER_H
#define INDEX_BUFFER_H

#include <glad/glad.h>
#include <cstdint>

class IndexBuffer {
public:
    IndexBuffer(const uint32_t* indices, uint32_t count);
    ~IndexBuffer();

    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;

    void bind() const;
    void unbind() const;

    uint32_t getCount() const { return m_count; }
    GLuint getId() const { return m_id; }

private:
    GLuint m_id = 0;
    uint32_t m_count = 0;
};

#endif // INDEX_BUFFER_H
