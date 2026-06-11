#include "GregoryFillSurface.h"
#include "PointObject.h"
#include "../Scene.h"
#include "../shader.h"
#include <glad/glad.h>
#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <string>

namespace objects {

namespace {

struct RawEdge {
    uint32_t surfaceId = 0;
    uint32_t cA = 0, cB = 0; // corner point ids
    std::array<std::shared_ptr<PointObject>, 4> boundary;
    std::array<std::shared_ptr<PointObject>, 4> inner;
};

pmath::Vec3 pointPos(const std::shared_ptr<PointObject>& p) {
    return pmath::Vec3(p->transform.m[0][3], p->transform.m[1][3], p->transform.m[2][3]);
}

// de Casteljau split of a cubic at t=0.5 into two cubics L (first half) and R (second half)
void splitCubic(const std::array<pmath::Vec3, 4>& b,
                std::array<pmath::Vec3, 4>& L,
                std::array<pmath::Vec3, 4>& R) {
    pmath::Vec3 q0 = (b[0] + b[1]) * 0.5f;
    pmath::Vec3 q1 = (b[1] + b[2]) * 0.5f;
    pmath::Vec3 q2 = (b[2] + b[3]) * 0.5f;
    pmath::Vec3 r0 = (q0 + q1) * 0.5f;
    pmath::Vec3 r1 = (q1 + q2) * 0.5f;
    pmath::Vec3 s  = (r0 + r1) * 0.5f;
    L = {b[0], q0, r0, s};
    R = {s, r1, q2, b[3]};
}

bool makeEdge(SurfaceObject* s,
              const int bu[4], const int bv[4],
              const int iu[4], const int iv[4],
              RawEdge& out) {
    out.surfaceId = s->id;
    for (int i = 0; i < 4; ++i) {
        auto pb = s->controlPointAt(bu[i], bv[i]);
        auto pin = s->controlPointAt(iu[i], iv[i]);
        if (!pb || !pin) return false;
        out.boundary[i] = pb;
        out.inner[i] = pin;
    }
    out.cA = out.boundary[0]->id;
    out.cB = out.boundary[3]->id;
    return true;
}

// Collect outer-rim patch edges of a Bezier C0 surface
void collectRimEdges(SurfaceObject* s, std::vector<RawEdge>& edges) {
    bool plane = (s->topology == SurfaceObject::Topology::Plane);
    for (int pj = 0; pj < s->patchesV; ++pj) {
        for (int pi = 0; pi < s->patchesU; ++pi) {
            int u0 = 3 * pi, v0 = 3 * pj;
            RawEdge e;
            // bottom (v = v0)
            if (pj == 0) {
                int bu[4] = {u0, u0 + 1, u0 + 2, u0 + 3};
                int bv[4] = {v0, v0, v0, v0};
                int iv[4] = {v0 + 1, v0 + 1, v0 + 1, v0 + 1};
                if (makeEdge(s, bu, bv, bu, iv, e)) edges.push_back(e);
            }
            // top (v = v0+3)
            if (pj == s->patchesV - 1) {
                int bu[4] = {u0, u0 + 1, u0 + 2, u0 + 3};
                int bv[4] = {v0 + 3, v0 + 3, v0 + 3, v0 + 3};
                int iv[4] = {v0 + 2, v0 + 2, v0 + 2, v0 + 2};
                if (makeEdge(s, bu, bv, bu, iv, e)) edges.push_back(e);
            }
            // left (u = u0)
            if (plane && pi == 0) {
                int bv[4] = {v0, v0 + 1, v0 + 2, v0 + 3};
                int bu[4] = {u0, u0, u0, u0};
                int iu[4] = {u0 + 1, u0 + 1, u0 + 1, u0 + 1};
                if (makeEdge(s, bu, bv, iu, bv, e)) edges.push_back(e);
            }
            // right (u = u0+3)
            if (plane && pi == s->patchesU - 1) {
                int bv[4] = {v0, v0 + 1, v0 + 2, v0 + 3};
                int bu[4] = {u0 + 3, u0 + 3, u0 + 3, u0 + 3};
                int iu[4] = {u0 + 2, u0 + 2, u0 + 2, u0 + 2};
                if (makeEdge(s, bu, bv, iu, bv, e)) edges.push_back(e);
            }
        }
    }
}

} // namespace

GregoryFillSurface::GregoryFillSurface(uint32_t id, std::string name)
    : SurfaceObject(id, std::move(name)) {}

GregoryFillSurface::~GregoryFillSurface() {
    if (contVBO != 0) glDeleteBuffers(1, &contVBO);
    if (contVAO != 0) glDeleteVertexArrays(1, &contVAO);
}

void GregoryFillSurface::collectReferencedPoints() {
    controlPoints.clear();
    std::set<uint32_t> seen;
    auto add = [&](const std::weak_ptr<PointObject>& wp) {
        if (auto p = wp.lock())
            if (seen.insert(p->id).second) controlPoints.push_back(wp);
    };
    for (auto& e : holeEdges) {
        for (auto& wp : e.boundary) add(wp);
        for (auto& wp : e.inner)    add(wp);
    }
}

std::shared_ptr<GregoryFillSurface> GregoryFillSurface::createFromSelection(Scene& scene) {
    // Take selected Bezier C0 surfaces.
    std::vector<SurfaceObject*> surfaces;
    for (auto& [id, obj] : scene.objects) {
        if (obj->isSelected && obj->getType() == ObjectType::BezierSurfaceC0)
            surfaces.push_back(static_cast<SurfaceObject*>(obj.get()));
    }
    if (surfaces.empty()) return nullptr;

    // Get outer rim edges
    std::vector<RawEdge> edges;
    for (auto* s : surfaces) collectRimEdges(s, edges);
    if (edges.size() < 3) return nullptr;

    // Find edges
    int ti = -1, tj = -1, tk = -1;
    int n = static_cast<int>(edges.size());
    for (int i = 0; i < n && ti < 0; ++i)
        for (int j = i + 1; j < n && ti < 0; ++j)
            for (int k = j + 1; k < n; ++k) {
                std::map<uint32_t, int> cnt;
                cnt[edges[i].cA]++; cnt[edges[i].cB]++;
                cnt[edges[j].cA]++; cnt[edges[j].cB]++;
                cnt[edges[k].cA]++; cnt[edges[k].cB]++;
                if (cnt.size() != 3) continue;
                bool good = true;
                for (auto& [c, c2] : cnt) if (c2 != 2) { good = false; break; }
                if (good) { ti = i; tj = j; tk = k; break; }
            }
    if (ti < 0) return nullptr;

    // Orient the edges
    RawEdge cyc[3] = {edges[ti], edges[tj], edges[tk]};
    std::vector<RawEdge> ordered;
    ordered.push_back(cyc[0]);
    bool used[3] = {true, false, false};
    uint32_t curEnd = cyc[0].cB;
    for (int step = 1; step < 3; ++step) {
        for (int m = 1; m < 3; ++m) {
            if (used[m]) continue;
            RawEdge e = cyc[m];
            if (e.cA == curEnd) {
                ordered.push_back(e); curEnd = e.cB; used[m] = true; break;
            }
            if (e.cB == curEnd) {
                std::reverse(e.boundary.begin(), e.boundary.end());
                std::reverse(e.inner.begin(), e.inner.end());
                std::swap(e.cA, e.cB);
                ordered.push_back(e); curEnd = e.cB; used[m] = true; break;
            }
        }
    }
    if (ordered.size() != 3) return nullptr;

    // Build object
    auto g = std::make_shared<GregoryFillSurface>(
        scene.nextId++, "Gregory Fill " + std::to_string(scene.gregoryCounter++));
    for (auto& re : ordered) {
        HoleEdge he;
        for (int i = 0; i < 4; ++i) { he.boundary[i] = re.boundary[i]; he.inner[i] = re.inner[i]; }
        g->holeEdges.push_back(he);
    }
    g->collectReferencedPoints();
    g->needsUpdate = true;
    scene.objects[g->id] = g;
    return g;
}

void GregoryFillSurface::computePatches() {
    patches.clear();
    continuityPts.clear();
    if (holeEdges.size() != 3) return;

    using pmath::Vec3;

    // Get boundary (B) and inner (C) rows of each hole edge.
    std::array<std::array<Vec3, 4>, 3> B, C;
    for (int k = 0; k < 3; ++k)
        for (int i = 0; i < 4; ++i) {
            auto pb  = holeEdges[k].boundary[i].lock();
            auto pin = holeEdges[k].inner[i].lock();
            if (!pb || !pin) return;
            B[k][i] = pointPos(pb);
            C[k][i] = pointPos(pin);
        }

    // Split each boundary/inner cubic at the midpoint
    std::array<std::array<Vec3, 4>, 3> Lb, Rb, Lc, Rc;
    for (int k = 0; k < 3; ++k) { splitCubic(B[k], Lb[k], Rb[k]); splitCubic(C[k], Lc[k], Rc[k]); }

    Vec3 mid[3];
    for (int k = 0; k < 3; ++k) mid[k] = Lb[k][3];
    Vec3 center = (mid[0] + mid[1] + mid[2]) * (1.0f / 3.0f);

    // Internal cubic from each midpoint to the centre
    std::array<std::array<Vec3, 4>, 3> IN;
    for (int k = 0; k < 3; ++k) {
        Vec3 d = center - mid[k];
        IN[k] = {mid[k], mid[k] + d * (1.0f / 3.0f), mid[k] + d * (2.0f / 3.0f), center};
    }

    // Build one Gregory patch per shared corner
    for (int k = 0; k < 3; ++k) {
        int kn = (k + 1) % 3;
        const auto& vb  = Rb[k];   // v=0 row: mid_k -> corner
        const auto& ub  = Lb[kn];  // u=3 col: corner -> mid_{k+1}
        const auto& in0 = IN[k];   // u=0 col: mid_k -> centre
        std::array<Vec3, 4> v3 = {IN[kn][3], IN[kn][2], IN[kn][1], IN[kn][0]}; // v=3 row: centre -> mid_{k+1}

        // Mirror inner rows across the boundary -> C1 with the Bezier patches.
        std::array<Vec3, 4> MRb, MLb;
        for (int i = 0; i < 4; ++i) {
            MRb[i] = Rb[k][i] * 2.0f - Rc[k][i];
            MLb[i] = Lb[kn][i] * 2.0f - Lc[kn][i];
        }

        std::array<Vec3, 20> P;

        P[0] = vb[0];  P[1] = vb[1];  P[2] = vb[2];  P[3] = vb[3];   // v=0 (outer)
        P[4] = in0[1]; P[7] = ub[1];                                 // v=1 ends
        P[8] = in0[2]; P[11] = ub[2];                                // v=2 ends
        P[12] = v3[0]; P[13] = v3[1]; P[14] = v3[2]; P[15] = v3[3];  // v=3 (internal)

        P[5]  = MRb[1];                          // (1,1) a
        P[16] = vb[1] + in0[1] - vb[0];          // (1,1) b
        P[6]  = MRb[2];                          // (2,1) a
        P[17] = MLb[1];                          // (2,1) b
        P[9]  = v3[1] + in0[2] - v3[0];          // (1,2) a
        P[18] = v3[1] + in0[2] - v3[0];          // (1,2) b
        P[10] = v3[2] + ub[2] - v3[3];           // (2,2) a
        P[19] = MLb[2];                          // (2,2) b

        patches.push_back(P);

        // Continuity segments: inner Bezier point -> boundary -> mirrored Gregory point. 
        for (int i = 0; i < 4; ++i) {
            continuityPts.push_back(Rc[k][i]);  continuityPts.push_back(MRb[i]);
            continuityPts.push_back(Lc[kn][i]); continuityPts.push_back(MLb[i]);
        }
    }
}

void GregoryFillSurface::updateGPUBuffers() {
    computePatches();
    if (patches.empty()) { patchVertexCount = 0; needsUpdate = false; return; }

    std::vector<float> vbo;
    std::vector<unsigned int> idx;       // GL_PATCHES (20 per patch)
    vbo.reserve(patches.size() * 20 * 3);
    idx.reserve(patches.size() * 20);
    for (const auto& P : patches) {
        unsigned int base = static_cast<unsigned int>(vbo.size() / 3);
        for (int i = 0; i < 20; ++i) {
            vbo.push_back(P[i].x); vbo.push_back(P[i].y); vbo.push_back(P[i].z);
            idx.push_back(base + i);
        }
    }

    if (VAO == 0) { glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO); glGenBuffers(1, &EBO); }
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vbo.size() * sizeof(float), vbo.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Continuity vectors
    if (contVAO == 0) { glGenVertexArrays(1, &contVAO); glGenBuffers(1, &contVBO); }
    glBindVertexArray(contVAO);
    glBindBuffer(GL_ARRAY_BUFFER, contVBO);
    glBufferData(GL_ARRAY_BUFFER, continuityPts.size() * 3 * sizeof(float), continuityPts.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    patchVertexCount = static_cast<int>(idx.size());
    contVertexCount  = static_cast<int>(continuityPts.size());
    needsUpdate = false;
}

void GregoryFillSurface::render(const RenderContext& ctx) {
    if (VAO == 0 || patchVertexCount == 0 || !ctx.gregoryShader) return;

    glBindVertexArray(VAO);

    // Surface: Gregory patches
    ctx.gregoryShader->use();
    ctx.gregoryShader->setMat4("model",      ctx.sceneModel);
    ctx.gregoryShader->setMat4("view",       ctx.view);
    ctx.gregoryShader->setMat4("projection", ctx.projection);
    ctx.gregoryShader->setVec3("screenSize",
        pmath::Vec3((float)ctx.displayW, (float)ctx.displayH, 0.0f));
    ctx.gregoryShader->setInt("tessLevel", tessLevel);
    if (isSelected) ctx.gregoryShader->setVec3("uColor", 1.0f, 0.6f, 0.2f);
    else            ctx.gregoryShader->setVec3("uColor", 0.95f, 0.75f, 0.3f);

    glPatchParameteri(GL_PATCH_VERTICES, 20);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    ctx.gregoryShader->setInt("orientation", 0);
    glDrawElements(GL_PATCHES, patchVertexCount, GL_UNSIGNED_INT, 0);
    ctx.gregoryShader->setInt("orientation", 1);
    glDrawElements(GL_PATCHES, patchVertexCount, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);

    // Continuity vectors
    if (showContinuityVectors && contVertexCount > 0 && ctx.shaderProgram) {
        ctx.shaderProgram->use();
        ctx.shaderProgram->setMat4("model",      ctx.sceneModel);
        ctx.shaderProgram->setMat4("view",       ctx.view);
        ctx.shaderProgram->setMat4("projection", ctx.projection);
        ctx.shaderProgram->setVec3("uColor", 0.2f, 1.0f, 0.4f);
        glBindVertexArray(contVAO);
        glDrawArrays(GL_LINES, 0, contVertexCount);
        glBindVertexArray(0);
    }
}

} // namespace objects
