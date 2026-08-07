#include "selection_outline.h"

#include "mesh/mesh.h"
#include "../core/resource_manager.h"
#include "../shader.h"

namespace {

constexpr float kOutlineWidth = 0.035f;
const glm::vec3 kOutlineColor(1.0f, 0.58f, 0.08f);

} // namespace

bool SelectionOutline::init() {
    m_shader = ResourceManager::getShader(
        "shaders/selection_outline.vert", "shaders/selection_outline.frag");
    return isReady();
}

bool SelectionOutline::beginMask() {
    if (!isReady()) {
        return false;
    }
    if (m_passActive) {
        end();
    }

    m_stencilWasEnabled = glIsEnabled(GL_STENCIL_TEST);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &m_depthWriteMask);
    glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &m_stencilClearValue);
    m_frontState = captureFaceState(false);
    m_backState = captureFaceState(true);

    glEnable(GL_STENCIL_TEST);
    glClearStencil(0);
    glStencilMask(0xFFU);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_ALWAYS, 0, 0xFFU);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    m_passActive = true;
    m_outlineActive = false;
    return true;
}

void SelectionOutline::maskObject(bool selected) const {
    if (!m_passActive || m_outlineActive) {
        return;
    }
    glStencilMask(selected ? 0xFFU : 0x00U);
    glStencilFunc(GL_ALWAYS, selected ? 1 : 0, 0xFFU);
}

void SelectionOutline::beginOutline() {
    if (!m_passActive) {
        return;
    }
    glStencilFunc(GL_NOTEQUAL, 1, 0xFFU);
    glStencilMask(0x00U);
    glDepthMask(GL_FALSE);
    m_outlineActive = true;
}

void SelectionOutline::drawMesh(const Mesh& mesh, const glm::mat4& model,
                                const glm::mat4& view,
                                const glm::mat4& projection) const {
    if (!m_outlineActive || !isReady()) {
        return;
    }

    m_shader->use();
    m_shader->setMat4("u_Model", model);
    m_shader->setMat4("u_View", view);
    m_shader->setMat4("u_Projection", projection);
    m_shader->setVec3("u_OutlineColor", kOutlineColor);
    m_shader->setFloat("u_OutlineWidth", kOutlineWidth);
    mesh.draw();
}

void SelectionOutline::end() {
    if (!m_passActive) {
        return;
    }

    glDepthMask(m_depthWriteMask);
    restoreFaceState(GL_FRONT, m_frontState);
    restoreFaceState(GL_BACK, m_backState);
    glClearStencil(m_stencilClearValue);
    if (m_stencilWasEnabled == GL_FALSE) {
        glDisable(GL_STENCIL_TEST);
    }

    m_passActive = false;
    m_outlineActive = false;
}

bool SelectionOutline::isReady() const {
    return m_shader != nullptr && m_shader->m_id != 0;
}

SelectionOutline::StencilFaceState SelectionOutline::captureFaceState(bool backFace) {
    StencilFaceState state;
    glGetIntegerv(backFace ? GL_STENCIL_BACK_FUNC : GL_STENCIL_FUNC, &state.function);
    glGetIntegerv(backFace ? GL_STENCIL_BACK_REF : GL_STENCIL_REF, &state.reference);
    glGetIntegerv(backFace ? GL_STENCIL_BACK_VALUE_MASK : GL_STENCIL_VALUE_MASK,
                  &state.valueMask);
    glGetIntegerv(backFace ? GL_STENCIL_BACK_WRITEMASK : GL_STENCIL_WRITEMASK,
                  &state.writeMask);
    glGetIntegerv(backFace ? GL_STENCIL_BACK_FAIL : GL_STENCIL_FAIL,
                  &state.stencilFail);
    glGetIntegerv(backFace ? GL_STENCIL_BACK_PASS_DEPTH_FAIL
                           : GL_STENCIL_PASS_DEPTH_FAIL,
                  &state.depthFail);
    glGetIntegerv(backFace ? GL_STENCIL_BACK_PASS_DEPTH_PASS
                           : GL_STENCIL_PASS_DEPTH_PASS,
                  &state.depthPass);
    return state;
}

void SelectionOutline::restoreFaceState(GLenum face, const StencilFaceState& state) {
    glStencilFuncSeparate(face, static_cast<GLenum>(state.function), state.reference,
                          static_cast<GLuint>(state.valueMask));
    glStencilMaskSeparate(face, static_cast<GLuint>(state.writeMask));
    glStencilOpSeparate(face, static_cast<GLenum>(state.stencilFail),
                        static_cast<GLenum>(state.depthFail),
                        static_cast<GLenum>(state.depthPass));
}
