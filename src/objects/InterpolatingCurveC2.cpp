#include "InterpolatingCurveC2.h"
#include <glad/glad.h>
#include <cmath>
#include "../shader.h"

namespace objects {

InterpolatingCurveC2::InterpolatingCurveC2(uint32_t id, std::string name)
    : CurveObject(id, std::move(name)) {}

std::vector<pmath::Vec3> InterpolatingCurveC2::solveCubicSpline(const std::vector<pmath::Vec3>& pts) {
    int n = (int)pts.size();
    if (n < 2) return {};

    // Chord distances
    std::vector<float> d(n - 1);
    for (int i = 0; i < n - 1; ++i) {
        d[i] = (pts[i + 1] - pts[i]).length();
        if (d[i] < 0.0001f) d[i] = 0.0001f;
    }

    // Tridiagonal system
    std::vector<float> a(n, 0.0f), b(n, 0.0f), c(n, 0.0f);
    std::vector<pmath::Vec3> r(n, pmath::Vec3(0.0f, 0.0f, 0.0f));

    b[0] = 2.0f; c[0] = 1.0f;
    r[0] = (pts[1] - pts[0]) * (3.0f / d[0]);

    for (int i = 1; i < n - 1; ++i) {
        a[i] = d[i];
        b[i] = 2.0f * (d[i-1] + d[i]);
        c[i] = d[i-1];
        pmath::Vec3 t1 = (pts[i]   - pts[i-1]) * (d[i]   / d[i-1]);
        pmath::Vec3 t2 = (pts[i+1] - pts[i])   * (d[i-1] / d[i]);
        r[i] = (t1 + t2) * 3.0f;
    }

    a[n-1] = 1.0f; b[n-1] = 2.0f;
    r[n-1] = (pts[n-1] - pts[n-2]) * (3.0f / d[n-2]);

    // Thomas algorithm
    std::vector<float> cp(n, 0.0f);
    std::vector<pmath::Vec3> rp(n, pmath::Vec3(0.0f, 0.0f, 0.0f));
    std::vector<pmath::Vec3> T(n, pmath::Vec3(0.0f, 0.0f, 0.0f));

    cp[0] = c[0] / b[0];
    rp[0] = r[0] * (1.0f / b[0]);
    for (int i = 1; i < n; ++i) {
        float m = b[i] - a[i] * cp[i-1];
        if (i < n-1) cp[i] = c[i] / m;
        rp[i] = (r[i] - rp[i-1] * a[i]) * (1.0f / m);
    }
    T[n-1] = rp[n-1];
    for (int i = n-2; i >= 0; --i)
        T[i] = rp[i] - T[i+1] * cp[i];

    // Convert tangents to Bezier control points
    std::vector<pmath::Vec3> result;
    for (int i = 0; i < n - 1; ++i) {
        result.push_back(pts[i]);
        result.push_back(pts[i]   + T[i]   * (d[i] / 3.0f));
        result.push_back(pts[i+1] - T[i+1] * (d[i] / 3.0f));
        result.push_back(pts[i+1]);
    }
    return result;
}

void InterpolatingCurveC2::updateGPUBuffers() {
    std::vector<pmath::Vec3> pts = resolveControlPoints();
    interpolatedBezierPoints = solveCubicSpline(pts);

    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
    }

    std::vector<float> vboData;
    std::vector<unsigned int> patchIdx;
    for (size_t i = 0; i < interpolatedBezierPoints.size(); ++i) {
        vboData.push_back(interpolatedBezierPoints[i].x);
        vboData.push_back(interpolatedBezierPoints[i].y);
        vboData.push_back(interpolatedBezierPoints[i].z);
        patchIdx.push_back((unsigned int)i);
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vboData.size() * sizeof(float), vboData.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, patchIdx.size() * sizeof(unsigned int), patchIdx.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    curveIndexCount = (int)patchIdx.size();
    needsUpdate = false;
}

void InterpolatingCurveC2::render(const RenderContext& ctx) {
    if (VAO == 0 || curveIndexCount == 0) return;

    pmath::Mat4 curveModel = ctx.sceneModel;

    if (showPolyline) {
        ctx.shaderProgram->use();
        ctx.shaderProgram->setMat4("model", curveModel);
        ctx.shaderProgram->setMat4("view", ctx.view);
        ctx.shaderProgram->setMat4("projection", ctx.projection);
        ctx.shaderProgram->setVec3("uColor", 0.9f, 0.6f, 0.2f);

        glBindVertexArray(VAO);
        glDrawElements(GL_LINE_STRIP, curveIndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    ctx.bezierShader->use();
    ctx.bezierShader->setMat4("model", curveModel);
    ctx.bezierShader->setMat4("view", ctx.view);
    ctx.bezierShader->setMat4("projection", ctx.projection);
    ctx.bezierShader->setVec3("screenSize", pmath::Vec3((float)ctx.displayW, (float)ctx.displayH, 0.0f));
    if (isSelected) ctx.bezierShader->setVec3("uColor", 0.4f, 1.0f, 0.4f);
    else            ctx.bezierShader->setVec3("uColor", 0.2f, 0.8f, 0.2f);

    glBindVertexArray(VAO);
    glPatchParameteri(GL_PATCH_VERTICES, 4);
    glDrawElements(GL_PATCHES, curveIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

} // namespace objects
