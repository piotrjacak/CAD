#include "TorusObject.h"
#include "Torus.h"
#include <glad/glad.h>
#include <cmath>
#include "../shader.h"

namespace objects {

TorusObject::TorusObject(uint32_t id, std::string name, float R, float r, int meshAcc)
    : SceneObject(id, std::move(name)), R(R), r(r), meshAcc(meshAcc) {}

void TorusObject::updateGPUBuffers() {
    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
    }
    Torus t(R, r, meshAcc, meshAcc);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, t.vertices.size() * sizeof(float), t.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, t.indices.size() * sizeof(unsigned int), t.indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    indexCount = (int)t.indices.size();
    needsUpdate = false;
}

void TorusObject::render(const RenderContext& ctx) {
    pmath::Mat4 objectModel = ctx.sceneModel * transform;
    ctx.shaderProgram->use();
    ctx.shaderProgram->setMat4("model", objectModel);
    ctx.shaderProgram->setMat4("view", ctx.view);
    ctx.shaderProgram->setMat4("projection", ctx.projection);
    if (isSelected) ctx.shaderProgram->setVec3("uColor", 0.7f, 0.8f, 1.0f);
    else            ctx.shaderProgram->setVec3("uColor", 1.0f, 1.0f, 1.0f);

    glBindVertexArray(VAO);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(0);
}

float TorusObject::getPickRadius() const {
    float scaleX = std::sqrt(
        transform.m[0][0] * transform.m[0][0] +
        transform.m[1][0] * transform.m[1][0] +
        transform.m[2][0] * transform.m[2][0]);
    return R * scaleX;
}

} // namespace objects
