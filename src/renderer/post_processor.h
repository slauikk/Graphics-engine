#ifndef POST_PROCESSOR_H
#define POST_PROCESSOR_H

#include <glad/glad.h>
#include <memory>

class Shader;

enum class PostProcessEffect : int {
    Normal = 0,
    Grayscale,
    Invert,
    Edges,
    Vignette,
    Crt
};

class PostProcessor {
public:
    PostProcessor() = default;
    ~PostProcessor();

    PostProcessor(const PostProcessor&) = delete;
    PostProcessor& operator=(const PostProcessor&) = delete;

    bool init();
    void render(GLuint sceneTexture, PostProcessEffect effect, float elapsedSeconds) const;
    bool isReady() const;

    static const char* effectName(PostProcessEffect effect);

private:
    GLuint m_vertexArray = 0;
    std::shared_ptr<Shader> m_shader;
};

#endif // POST_PROCESSOR_H
