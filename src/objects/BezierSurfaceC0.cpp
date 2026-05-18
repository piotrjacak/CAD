#include "BezierSurfaceC0.h"
#include "PointObject.h"
#include "../Scene.h"
#include "../shader.h"
#include <glad/glad.h>
#include <cmath>

namespace objects {

BezierSurfaceC0::BezierSurfaceC0(uint32_t id, std::string name)
    : SurfaceObject(id, std::move(name)) {}

void BezierSurfaceC0::updateGPUBuffers() {
    std::vector<pmath::Vec3> pts = resolveControlPoints();
    if (pts.empty() || gridU == 0 || gridV == 0) {
        needsUpdate = false;
        return;
    }

    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
    }

    std::vector<float> vboData;
    vboData.reserve(pts.size() * 3);
    for (const auto& p : pts) {
        vboData.push_back(p.x);
        vboData.push_back(p.y);
        vboData.push_back(p.z);
    }

    // Indeksy linii siatki kontrolnej (GL_LINES)
    std::vector<unsigned int> meshLineIdx;
    const int uMax = (topology == Topology::Cylinder) ? gridU : gridU - 1;
    for (int v = 0; v < gridV; ++v) {
        for (int u = 0; u < uMax; ++u) {
            meshLineIdx.push_back(static_cast<unsigned int>(gridIndex(u,     v)));
            meshLineIdx.push_back(static_cast<unsigned int>(gridIndex(u + 1, v)));
        }
    }
    for (int v = 0; v < gridV - 1; ++v) {
        for (int u = 0; u < gridU; ++u) {
            meshLineIdx.push_back(static_cast<unsigned int>(gridIndex(u, v)));
            meshLineIdx.push_back(static_cast<unsigned int>(gridIndex(u, v + 1)));
        }
    }

    // Indeksy patchy (GL_PATCHES, 16 per patch)
    std::vector<unsigned int> patchIdx;
    patchIdx.reserve(static_cast<size_t>(patchesU) * patchesV * 16);
    for (int pj = 0; pj < patchesV; ++pj) {
        for (int pi = 0; pi < patchesU; ++pi) {
            for (int b = 0; b < 4; ++b) {
                for (int a = 0; a < 4; ++a) {
                    int u = 3 * pi + a;
                    int v = 3 * pj + b;
                    patchIdx.push_back(static_cast<unsigned int>(gridIndex(u, v)));
                }
            }
        }
    }

    std::vector<unsigned int> allIdx;
    allIdx.reserve(meshLineIdx.size() + patchIdx.size());
    allIdx.insert(allIdx.end(), meshLineIdx.begin(), meshLineIdx.end());
    allIdx.insert(allIdx.end(), patchIdx.begin(),    patchIdx.end());

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vboData.size() * sizeof(float), vboData.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, allIdx.size() * sizeof(unsigned int), allIdx.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    meshLineIndexCount = static_cast<int>(meshLineIdx.size());
    patchIndexCount    = static_cast<int>(patchIdx.size());
    needsUpdate = false;
}

void BezierSurfaceC0::render(const RenderContext& ctx) {
    if (VAO == 0 || patchIndexCount == 0) return;

    pmath::Mat4 model = ctx.sceneModel;

    glBindVertexArray(VAO);

    // Siatka kontrolna (Beziera) - kolor jak BezierCurveC0 polyline
    if (showControlMesh && meshLineIndexCount > 0 && ctx.shaderProgram) {
        ctx.shaderProgram->use();
        ctx.shaderProgram->setMat4("model",      model);
        ctx.shaderProgram->setMat4("view",       ctx.view);
        ctx.shaderProgram->setMat4("projection", ctx.projection);
        ctx.shaderProgram->setVec3("uColor", 0.8f, 0.8f, 0.2f);
        glDrawElements(GL_LINES, meshLineIndexCount, GL_UNSIGNED_INT, (void*)0);
    }

    // Powierzchnia (patche)
    if (ctx.bezierSurfaceC0Shader) {
        ctx.bezierSurfaceC0Shader->use();
        ctx.bezierSurfaceC0Shader->setMat4("model",      model);
        ctx.bezierSurfaceC0Shader->setMat4("view",       ctx.view);
        ctx.bezierSurfaceC0Shader->setMat4("projection", ctx.projection);
        ctx.bezierSurfaceC0Shader->setVec3("screenSize",
            pmath::Vec3((float)ctx.displayW, (float)ctx.displayH, 0.0f));
        ctx.bezierSurfaceC0Shader->setInt("tessLevel", tessLevel);
        if (isSelected) ctx.bezierSurfaceC0Shader->setVec3("uColor", 0.7f, 0.8f, 1.0f);
        else            ctx.bezierSurfaceC0Shader->setVec3("uColor", 1.0f, 1.0f, 1.0f);

        glPatchParameteri(GL_PATCH_VERTICES, 16);
        void* patchOffset = (void*)(static_cast<size_t>(meshLineIndexCount) * sizeof(unsigned int));

        ctx.bezierSurfaceC0Shader->setInt("orientation", 0);
        glDrawElements(GL_PATCHES, patchIndexCount, GL_UNSIGNED_INT, patchOffset);

        ctx.bezierSurfaceC0Shader->setInt("orientation", 1);
        glDrawElements(GL_PATCHES, patchIndexCount, GL_UNSIGNED_INT, patchOffset);
    }

    glBindVertexArray(0);
}

std::shared_ptr<BezierSurfaceC0> BezierSurfaceC0::createPlane(
    Scene& scene,
    int patchesU, int patchesV,
    float width, float height,
    const pmath::Vec3& origin)
{
    if (patchesU < 1) patchesU = 1;
    if (patchesV < 1) patchesV = 1;

    auto surface = std::make_shared<BezierSurfaceC0>(
        scene.nextId++,
        "Bezier Surface C0 " + std::to_string(scene.bezierSurfaceC0Counter++));

    surface->topology = Topology::Plane;
    surface->patchesU = patchesU;
    surface->patchesV = patchesV;
    surface->gridU = 3 * patchesU + 1;
    surface->gridV = 3 * patchesV + 1;
    surface->controlPoints.reserve(static_cast<size_t>(surface->gridU) * surface->gridV);

    const float halfW = width * 0.5f;
    const float halfH = height * 0.5f;

    for (int v = 0; v < surface->gridV; ++v) {
        for (int u = 0; u < surface->gridU; ++u) {
            float fx = static_cast<float>(u) / static_cast<float>(surface->gridU - 1);
            float fz = static_cast<float>(v) / static_cast<float>(surface->gridV - 1);
            float x = origin.x - halfW + fx * width;
            float y = origin.y;
            float z = origin.z - halfH + fz * height;

            auto pt = std::make_shared<PointObject>(
                scene.nextId++,
                "Point " + std::to_string(scene.pointCounter++));
            pt->transform.shift(x, y, z);
            pt->ownerSurfaceId = surface->id;
            scene.objects[pt->id] = pt;
            surface->controlPoints.push_back(pt);
        }
    }

    surface->needsUpdate = true;
    scene.objects[surface->id] = surface;
    return surface;
}

std::shared_ptr<BezierSurfaceC0> BezierSurfaceC0::createCylinder(
    Scene& scene,
    int patchesU, int patchesV,
    float radius, float height,
    const pmath::Vec3& origin)
{
    if (patchesU < 1) patchesU = 1;
    if (patchesV < 1) patchesV = 1;

    auto surface = std::make_shared<BezierSurfaceC0>(
        scene.nextId++,
        "Bezier Surface C0 " + std::to_string(scene.bezierSurfaceC0Counter++));

    surface->topology = Topology::Cylinder;
    surface->patchesU = patchesU;
    surface->patchesV = patchesV;
    surface->gridU = 3 * patchesU;           // cyclic U
    surface->gridV = 3 * patchesV + 1;
    surface->controlPoints.reserve(static_cast<size_t>(surface->gridU) * surface->gridV);

    const float halfH = height * 0.5f;
    const float twoPi = 6.28318530717958647692f;

    for (int v = 0; v < surface->gridV; ++v) {
        for (int u = 0; u < surface->gridU; ++u) {
            float theta = twoPi * static_cast<float>(u) / static_cast<float>(surface->gridU);
            float fz = static_cast<float>(v) / static_cast<float>(surface->gridV - 1);
            float x = origin.x + radius * std::cos(theta);
            float y = origin.y + radius * std::sin(theta);
            float z = origin.z - halfH + fz * height;

            auto pt = std::make_shared<PointObject>(
                scene.nextId++,
                "Point " + std::to_string(scene.pointCounter++));
            pt->transform.shift(x, y, z);
            pt->ownerSurfaceId = surface->id;
            scene.objects[pt->id] = pt;
            surface->controlPoints.push_back(pt);
        }
    }

    surface->needsUpdate = true;
    scene.objects[surface->id] = surface;
    return surface;
}

} // namespace objects
