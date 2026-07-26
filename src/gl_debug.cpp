#include "gl_debug.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace {

constexpr GLenum kDebugOutput = 0x92E0;
constexpr GLenum kDebugSeverityHigh = 0x9146;
constexpr GLenum kDebugSeverityMedium = 0x9147;
constexpr GLenum kDebugSeverityLow = 0x9148;
constexpr GLenum kDebugSeverityNotification = 0x826B;

using DebugCallback = void (GLAPIENTRY *)(GLenum source, GLenum type, GLuint id,
                                          GLenum severity, GLsizei length,
                                          const GLchar* message, const void* userParam);
using DebugMessageCallbackFunction = void (GLAPIENTRY *)(DebugCallback callback,
                                                         const void* userParam);
using DebugMessageControlFunction = void (GLAPIENTRY *)(GLenum source, GLenum type,
                                                        GLenum severity, GLsizei count,
                                                        const GLuint* ids, GLboolean enabled);

const char* severityName(GLenum severity) {
    switch (severity) {
        case kDebugSeverityHigh: return "high";
        case kDebugSeverityMedium: return "medium";
        case kDebugSeverityLow: return "low";
        case kDebugSeverityNotification: return "notification";
        default: return "unknown";
    }
}

template <typename Function>
Function resolveGlFunction(const char* name) {
    return reinterpret_cast<Function>(glfwGetProcAddress(name));
}

void GLAPIENTRY debugCallback(GLenum source, GLenum type, GLuint id,
                              GLenum severity, GLsizei length,
                              const GLchar* message, const void* userParam) {
    (void)source;
    (void)type;
    (void)length;
    (void)userParam;

    if (severity == kDebugSeverityNotification) {
        return;
    }

    std::cerr << "[OpenGL][" << severityName(severity) << "][id=" << id << "] "
              << (message != nullptr ? message : "No debug message") << "\n";
}

} // namespace

void GL_EnableDebugOutput() {
    DebugMessageCallbackFunction setCallback =
        resolveGlFunction<DebugMessageCallbackFunction>("glDebugMessageCallback");
    DebugMessageControlFunction setMessageControl =
        resolveGlFunction<DebugMessageControlFunction>("glDebugMessageControl");
    bool supportsNotificationSeverity = setCallback != nullptr;

    if (setCallback == nullptr) {
        setCallback =
            resolveGlFunction<DebugMessageCallbackFunction>("glDebugMessageCallbackARB");
        setMessageControl =
            resolveGlFunction<DebugMessageControlFunction>("glDebugMessageControlARB");
        supportsNotificationSeverity = false;
    }

    if (setCallback == nullptr) {
        std::cerr << "[OpenGL] Debug output unavailable: callback entry point not found\n";
        return;
    }

    if (supportsNotificationSeverity) {
        glEnable(kDebugOutput);
    }
    setCallback(debugCallback, nullptr);

    if (supportsNotificationSeverity && setMessageControl != nullptr) {
        setMessageControl(GL_DONT_CARE, GL_DONT_CARE, kDebugSeverityNotification,
                          0, nullptr, GL_FALSE);
    }
}
