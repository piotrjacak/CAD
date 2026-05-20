#include "BezierSurfaceC2.h"
#include "PointObject.h"
#include "../Scene.h"
#include "../shader.h"
#include <glad/glad.h>
#include <cmath>

namespace objects {

BezierSurfaceC2::BezierSurfaceC2(uint32_t id, std::string name)
    : SurfaceObject(id, std::move(name)) {}

void BezierSurfaceC2::updateGPUBuffers() {
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

    // Meshline indices (GL_LINES)
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

    // Patch indices (GL_PATCHES, 16 per patch)
    std::vector<unsigned int> patchIdx;
    patchIdx.reserve(static_cast<size_t>(patchesU) * patchesV * 16);
    for (int pj = 0; pj < patchesV; ++pj) {
        for (int pi = 0; pi < patchesU; ++pi) {
            for (int b = 0; b < 4; ++b) {
                for (int a = 0; a < 4; ++a) {
                    int u = pi + a;
                    int v = pj + b;
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

void BezierSurfaceC2::render(const RenderContext& ctx) {
    if (VAO == 0 || patchIndexCount == 0) return;

    pmath::Mat4 model = ctx.sceneModel;

    glBindVertexArray(VAO);

    // Meshline (Bezier)
    if (showControlMesh && meshLineIndexCount > 0 && ctx.shaderProgram) {
        ctx.shaderProgram->use();
        ctx.shaderProgram->setMat4("model",      model);
        ctx.shaderProgram->setMat4("view",       ctx.view);
        ctx.shaderProgram->setMat4("projection", ctx.projection);
        ctx.shaderProgram->setVec3("uColor", 0.2f, 0.6f, 0.8f);
        glDrawElements(GL_LINES, meshLineIndexCount, GL_UNSIGNED_INT, (void*)0);
    }

    // Surface (patches)
    if (ctx.bezierSurfaceC2Shader) {
        ctx.bezierSurfaceC2Shader->use();
        ctx.bezierSurfaceC2Shader->setMat4("model",      model);
        ctx.bezierSurfaceC2Shader->setMat4("view",       ctx.view);
        ctx.bezierSurfaceC2Shader->setMat4("projection", ctx.projection);
        ctx.bezierSurfaceC2Shader->setVec3("screenSize",
            pmath::Vec3((float)ctx.displayW, (float)ctx.displayH, 0.0f));
        ctx.bezierSurfaceC2Shader->setInt("tessLevel", tessLevel);
        if (isSelected) ctx.bezierSurfaceC2Shader->setVec3("uColor", 0.7f, 0.8f, 1.0f);
        else            ctx.bezierSurfaceC2Shader->setVec3("uColor", 1.0f, 1.0f, 1.0f);

        glPatchParameteri(GL_PATCH_VERTICES, 16);
        void* patchOffset = (void*)(static_cast<size_t>(meshLineIndexCount) * sizeof(unsigned int));

        ctx.bezierSurfaceC2Shader->setInt("orientation", 0);
        glDrawElements(GL_PATCHES, patchIndexCount, GL_UNSIGNED_INT, patchOffset);

        ctx.bezierSurfaceC2Shader->setInt("orientation", 1);
        glDrawElements(GL_PATCHES, patchIndexCount, GL_UNSIGNED_INT, patchOffset);
    }

    glBindVertexArray(0);
}

std::shared_ptr<BezierSurfaceC2> BezierSurfaceC2::createPlane(
    Scene& scene,
    int patchesU, int patchesV,
    float width, float height,
    const pmath::Vec3& origin)
{
    if (patchesU < 1) patchesU = 1;
    if (patchesV < 1) patchesV = 1;

    auto surface = std::make_shared<BezierSurfaceC2>(
        scene.nextId++,
        "Bezier Surface C2 " + std::to_string(scene.bezierSurfaceC2Counter++));

    surface->topology = Topology::Plane;
    surface->patchesU = patchesU;
    surface->patchesV = patchesV;
    surface->gridU = patchesU + 3;
    surface->gridV = patchesV + 3;
    surface->controlPoints.reserve(static_cast<size_t>(surface->gridU) * surface->gridV);

    const float xStep = width  / static_cast<float>(patchesU);
    const float zStep = height / static_cast<float>(patchesV);
    const float halfW = width  * 0.5f;
    const float halfH = height * 0.5f;

    for (int v = 0; v < surface->gridV; ++v) {
        for (int u = 0; u < surface->gridU; ++u) {
            float x = origin.x - halfW + (u - 1) * xStep;
            float y = origin.y;
            float z = origin.z - halfH + (v - 1) * zStep;

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

std::shared_ptr<BezierSurfaceC2> BezierSurfaceC2::createCylinder(
    Scene& scene,
    int patchesU, int patchesV,
    float radius, float height,
    const pmath::Vec3& origin)
{
    if (patchesU < 1) patchesU = 1;
    if (patchesV < 1) patchesV = 1;

    auto surface = std::make_shared<BezierSurfaceC2>(
        scene.nextId++,
        "Bezier Surface C2 " + std::to_string(scene.bezierSurfaceC2Counter++));

    surface->topology = Topology::Cylinder;
    surface->patchesU = patchesU;
    surface->patchesV = patchesV;
    surface->gridU = patchesU;            // cyclic, no margin in U for cylinder
    surface->gridV = patchesV + 3;
    surface->controlPoints.reserve(static_cast<size_t>(surface->gridU) * surface->gridV);

    const float halfH = height * 0.5f;
    const float twoPi = 6.28318530717958647692f;
    const float zStep = height / static_cast<float>(patchesV);

    for (int v = 0; v < surface->gridV; ++v) {
        for (int u = 0; u < surface->gridU; ++u) {
            float theta = twoPi * static_cast<float>(u) / static_cast<float>(surface->gridU);
            float x = origin.x + radius * std::cos(theta);
            float y = origin.y + radius * std::sin(theta);
            float z = origin.z - halfH + (v - 1) * zStep;

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
