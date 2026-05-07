#include "BezierCurveC2.h"
#include <glad/glad.h>
#include "../shader.h"

namespace objects {

BezierCurveC2::BezierCurveC2(uint32_t id, std::string name)
    : CurveObject(id, std::move(name)) {}

void BezierCurveC2::updateGPUBuffers() {
    std::vector<pmath::Vec3> pts = resolveControlPoints();
    int N = (int)pts.size();
    if (N == 0) { needsUpdate = false; return; }

    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
    }

    // Convert B-spline de Boor points to Bezier control points
    std::vector<pmath::Vec3> bezierPts;
    if (N >= 4) {
        for (int i = 0; i <= N - 4; ++i) {
            pmath::Vec3 d0 = pts[i], d1 = pts[i+1], d2 = pts[i+2], d3 = pts[i+3];
            bezierPts.push_back((d0 + d1 * 4.0f + d2) * (1.0f / 6.0f));
            bezierPts.push_back((d1 * 2.0f + d2) * (1.0f / 3.0f));
            bezierPts.push_back((d1 + d2 * 2.0f) * (1.0f / 3.0f));
            bezierPts.push_back((d1 + d2 * 4.0f + d3) * (1.0f / 6.0f));
        }
    }
    virtualPoints = bezierPts;

    std::vector<float> vboData;
    std::vector<unsigned int> polylineIdx, patchIdx;

    for (int i = 0; i < N; ++i) {
        vboData.push_back(pts[i].x);
        vboData.push_back(pts[i].y);
        vboData.push_back(pts[i].z);
        polylineIdx.push_back(i);
    }
    int bezierStart = N;
    for (int i = 0; i < (int)bezierPts.size(); ++i) {
        vboData.push_back(bezierPts[i].x);
        vboData.push_back(bezierPts[i].y);
        vboData.push_back(bezierPts[i].z);
        patchIdx.push_back(bezierStart + i);
    }

    std::vector<unsigned int> allIdx = polylineIdx;
    allIdx.insert(allIdx.end(), patchIdx.begin(), patchIdx.end());

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vboData.size() * sizeof(float), vboData.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, allIdx.size() * sizeof(unsigned int), allIdx.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    polylineIndexCount = (int)polylineIdx.size();
    curveIndexCount = (int)patchIdx.size();
    needsUpdate = false;
}

void BezierCurveC2::render(const RenderContext& ctx) {
    if (VAO == 0) return;

    pmath::Mat4 curveModel = ctx.sceneModel;

    // B-spline basis: draw polyline over de Boor control points
    if (showPolyline && isBsplineBasis) {
        ctx.shaderProgram->use();
        ctx.shaderProgram->setMat4("model", curveModel);
        ctx.shaderProgram->setMat4("view", ctx.view);
        ctx.shaderProgram->setMat4("projection", ctx.projection);
        ctx.shaderProgram->setVec3("uColor", 0.2f, 0.6f, 0.8f);

        glBindVertexArray(VAO);
        glDrawElements(GL_LINE_STRIP, polylineIndexCount, GL_UNSIGNED_INT, (void*)0);
        glBindVertexArray(0);
    }

    // Bernstein basis: draw virtual (Bezier) control points and optional polyline
    if (!isBsplineBasis) {
        ctx.shaderProgram->use();
        ctx.shaderProgram->setMat4("model", curveModel);
        ctx.shaderProgram->setMat4("view", ctx.view);
        ctx.shaderProgram->setMat4("projection", ctx.projection);
        ctx.shaderProgram->setVec3("uColor", 0.9f, 0.3f, 0.8f);

        glBindVertexArray(VAO);
        glPointSize(10.0f);
        void* offset = (void*)(polylineIndexCount * sizeof(unsigned int));
        glDrawElements(GL_POINTS, curveIndexCount, GL_UNSIGNED_INT, offset);

        if (showPolyline) {
            ctx.shaderProgram->setVec3("uColor", 0.8f, 0.4f, 0.6f);
            glDrawElements(GL_LINE_STRIP, curveIndexCount, GL_UNSIGNED_INT, offset);
        }

        if (selectedVirtualPointIndex != -1) {
            glPointSize(15.0f);
            void* singleOffset = (void*)((polylineIndexCount + selectedVirtualPointIndex) * sizeof(unsigned int));
            glDrawElements(GL_POINTS, 1, GL_UNSIGNED_INT, singleOffset);
        }
        glBindVertexArray(0);
    }

    // Tessellation curve
    if (curveIndexCount > 0) {
        ctx.bezierShader->use();
        ctx.bezierShader->setMat4("model", curveModel);
        ctx.bezierShader->setMat4("view", ctx.view);
        ctx.bezierShader->setMat4("projection", ctx.projection);
        ctx.bezierShader->setVec3("screenSize", pmath::Vec3((float)ctx.displayW, (float)ctx.displayH, 0.0f));
        if (isSelected) ctx.bezierShader->setVec3("uColor", 0.7f, 0.8f, 1.0f);
        else            ctx.bezierShader->setVec3("uColor", 1.0f, 1.0f, 1.0f);

        glBindVertexArray(VAO);
        glPatchParameteri(GL_PATCH_VERTICES, 4);
        void* offset = (void*)(polylineIndexCount * sizeof(unsigned int));
        glDrawElements(GL_PATCHES, curveIndexCount, GL_UNSIGNED_INT, offset);
        glBindVertexArray(0);
    }
}

} // namespace objects
