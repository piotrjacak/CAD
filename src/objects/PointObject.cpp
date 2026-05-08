#include "PointObject.h"
#include <glad/glad.h>
#include <cmath>
#include "../shader.h"

namespace objects {

PointObject::PointObject(uint32_t id, std::string name)
    : SceneObject(id, std::move(name)) {}

void PointObject::updateGPUBuffers() {
    needsUpdate = false;
}

void PointObject::render(const RenderContext& ctx) {
    pmath::Mat4 objectModel = ctx.sceneModel * transform;
    ctx.shaderProgram->use();
    ctx.shaderProgram->setMat4("model", objectModel);
    ctx.shaderProgram->setMat4("view", ctx.view);
    ctx.shaderProgram->setMat4("projection", ctx.projection);
    if (isSelected) ctx.shaderProgram->setVec3("uColor", 0.7f, 0.8f, 1.0f);
    else            ctx.shaderProgram->setVec3("uColor", 1.0f, 1.0f, 1.0f);

    glBindVertexArray(ctx.sharedPointVAO);
    glPointSize(10.0f);
    glDrawArrays(GL_POINTS, 0, 1);
    glBindVertexArray(0);
}

float PointObject::getPickRadius() const {
    float scaleX = std::sqrt(
        transform.m[0][0] * transform.m[0][0] +
        transform.m[1][0] * transform.m[1][0] +
        transform.m[2][0] * transform.m[2][0]);
    return 0.3f * scaleX;
}

} // namespace objects
