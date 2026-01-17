#include "gl_debug.h"
#include <glad/glad.h>
#include <iostream>

static void GLAPIENTRY debugCallback(GLenum source, GLenum type, GLuint id,
                                     GLenum severity, GLsizei length,
                                     const GLchar* message, const void* userParam) {
    (void)source;
    (void)type;
    (void)id;
    (void)length;
    (void)userParam;
    
    std::cerr << "[OpenGL] " << message << "\n";
}

void GL_EnableDebugOutput() {
#ifdef GLAD_GL_KHR_debug
    if (GLAD_GL_KHR_debug) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(debugCallback, nullptr);
        
        // Вимкнути нотифікації
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, 
                             GL_DEBUG_SEVERITY_NOTIFICATION, 
                             0, nullptr, GL_FALSE);
    }
#endif
}
