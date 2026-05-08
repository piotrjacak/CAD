#include "BezierCurveC0.h"
#include <glad/glad.h>
#include <algorithm>
#include "../shader.h"

namespace objects {

BezierCurveC0::BezierCurveC0(uint32_t id, std::string name)
    : CurveObject(id, std::move(name)) {}

void BezierCurveC0::updateGPUBuffers() {
    std::vector<pmath::Vec3> pts = resolveControlPoints();
    int N = (int)pts.size();
    if (N == 0) { needsUpdate = false; return; }

    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
    }

    std::vector<float> vboData;
    for (const auto& pt : pts) {
        vboData.push_back(pt.x);
        vboData.push_back(pt.y);
        vboData.push_back(pt.z);
    }

    std::vector<unsigned int> patchIndices;
    if      (N == 1) patchIndices = { 0, 0, 0, 0 };
    else if (N == 2) patchIndices = { 0, 0, 1, 1 };
    else if (N == 3) patchIndices = { 0, 1, 1, 2 };
    else {
        for (int i = 0; i < N - 1; i += 3) {
            patchIndices.push_back(i);
            patchIndices.push_back(std::min(i + 1, N - 1));
            patchIndices.push_back(std::min(i + 2, N - 1));
            patchIndices.push_back(std::min(i + 3, N - 1));
        }
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vboData.size() * sizeof(float), vboData.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, patchIndices.size() * sizeof(unsigned int), patchIndices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    indexCount = (int)patchIndices.size();
    needsUpdate = false;
}

void BezierCurveC0::render(const RenderContext& ctx) {
    if (VAO == 0 || indexCount == 0) return;

    pmath::Mat4 curveModel = ctx.sceneModel;

    if (showPolyline) {
        ctx.shaderProgram->use();
        ctx.shaderProgram->setMat4("model", curveModel);
        ctx.shaderProgram->setMat4("view", ctx.view);
        ctx.shaderProgram->setMat4("projection", ctx.projection);
        ctx.shaderProgram->setVec3("uColor", 0.8f, 0.8f, 0.2f);

        glBindVertexArray(VAO);
        glDrawElements(GL_LINE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    ctx.bezierShader->use();
    ctx.bezierShader->setMat4("model", curveModel);
    ctx.bezierShader->setMat4("view", ctx.view);
    ctx.bezierShader->setMat4("projection", ctx.projection);
    ctx.bezierShader->setVec3("screenSize", pmath::Vec3((float)ctx.displayW, (float)ctx.displayH, 0.0f));
    if (isSelected) ctx.bezierShader->setVec3("uColor", 0.7f, 0.8f, 1.0f);
    else            ctx.bezierShader->setVec3("uColor", 1.0f, 1.0f, 1.0f);

    glBindVertexArray(VAO);
    glPatchParameteri(GL_PATCH_VERTICES, 4);
    glDrawElements(GL_PATCHES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

} // namespace objects
