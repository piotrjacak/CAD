#include "SceneIO.h"
#include "Scene.h"
#include "nlohmann/json.hpp"
#include "objects/PointObject.h"
#include "objects/TorusObject.h"
#include "objects/CurveObject.h"
#include "objects/SurfaceObject.h"
#include "objects/BezierCurveC0.h"
#include "objects/BezierCurveC2.h"
#include "objects/InterpolatingCurveC2.h"
#include "objects/BezierSurfaceC0.h"
#include "objects/BezierSurfaceC2.h"
#include <fstream>
#include <cmath>

using nlohmann::json;
using namespace objects;

namespace {

json vec3ToJson(float x, float y, float z) {
    return json{{"x", x}, {"y", y}, {"z", z}};
}

json positionOf(const SceneObject& o) {
    return vec3ToJson(o.transform.m[0][3], o.transform.m[1][3], o.transform.m[2][3]);
}

// Decompose an affine transform (TRS) into scale + quaternion.
void decompose(const pmath::Mat4& m, json& scale, json& rotation) {
    float sx = std::sqrt(m.m[0][0]*m.m[0][0] + m.m[1][0]*m.m[1][0] + m.m[2][0]*m.m[2][0]);
    float sy = std::sqrt(m.m[0][1]*m.m[0][1] + m.m[1][1]*m.m[1][1] + m.m[2][1]*m.m[2][1]);
    float sz = std::sqrt(m.m[0][2]*m.m[0][2] + m.m[1][2]*m.m[1][2] + m.m[2][2]*m.m[2][2]);
    scale = vec3ToJson(sx, sy, sz);

    float ix = sx > 1e-8f ? 1.0f/sx : 0.0f;
    float iy = sy > 1e-8f ? 1.0f/sy : 0.0f;
    float iz = sz > 1e-8f ? 1.0f/sz : 0.0f;
    float r[3][3];
    for (int i = 0; i < 3; ++i) {
        r[i][0] = m.m[i][0] * ix;
        r[i][1] = m.m[i][1] * iy;
        r[i][2] = m.m[i][2] * iz;
    }

    // Rotation matrix -> quaternion
    float qw, qx, qy, qz;
    float tr = r[0][0] + r[1][1] + r[2][2];
    if (tr > 0.0f) {
        float s = std::sqrt(tr + 1.0f) * 2.0f;
        qw = 0.25f * s;
        qx = (r[2][1] - r[1][2]) / s;
        qy = (r[0][2] - r[2][0]) / s;
        qz = (r[1][0] - r[0][1]) / s;
    } else if (r[0][0] > r[1][1] && r[0][0] > r[2][2]) {
        float s = std::sqrt(1.0f + r[0][0] - r[1][1] - r[2][2]) * 2.0f;
        qw = (r[2][1] - r[1][2]) / s;
        qx = 0.25f * s;
        qy = (r[0][1] + r[1][0]) / s;
        qz = (r[0][2] + r[2][0]) / s;
    } else if (r[1][1] > r[2][2]) {
        float s = std::sqrt(1.0f + r[1][1] - r[0][0] - r[2][2]) * 2.0f;
        qw = (r[0][2] - r[2][0]) / s;
        qx = (r[0][1] + r[1][0]) / s;
        qy = 0.25f * s;
        qz = (r[1][2] + r[2][1]) / s;
    } else {
        float s = std::sqrt(1.0f + r[2][2] - r[0][0] - r[1][1]) * 2.0f;
        qw = (r[1][0] - r[0][1]) / s;
        qx = (r[0][2] + r[2][0]) / s;
        qy = (r[1][2] + r[2][1]) / s;
        qz = 0.25f * s;
    }
    rotation = json{{"x", qx}, {"y", qy}, {"z", qz}, {"w", qw}};
}

json controlPointRefs(std::vector<std::weak_ptr<PointObject>>* cps) {
    json arr = json::array();
    if (cps)
        for (auto& wp : *cps)
            if (auto p = wp.lock())
                arr.push_back(json{{"id", p->id}});
    return arr;
}

// Control points row-major (v rows of u)
json surfaceControlPoints(const SurfaceObject& s, int extU, int extV, bool wrapU, bool wrapV) {
    json arr = json::array();
    for (int v = 0; v < extV; ++v) {
        for (int u = 0; u < extU; ++u) {
            int uu = wrapU ? (u % s.gridU) : u;
            int vv = wrapV ? (v % s.gridV) : v;
            int idx = vv * s.gridU + uu;
            uint32_t pid = 0;
            if (idx >= 0 && idx < static_cast<int>(s.controlPoints.size()))
                if (auto p = s.controlPoints[idx].lock()) pid = p->id;
            arr.push_back(json{{"id", pid}});
        }
    }
    return arr;
}

// Build a T*R*S transform from position/quaternion/scale json
pmath::Mat4 composeTransform(const json& pos, const json& rot, const json& scl) {
    float px = pos.at("x").get<float>(), py = pos.at("y").get<float>(), pz = pos.at("z").get<float>();
    float qx = rot.at("x").get<float>(), qy = rot.at("y").get<float>();
    float qz = rot.at("z").get<float>(), qw = rot.at("w").get<float>();
    float sx = scl.at("x").get<float>(), sy = scl.at("y").get<float>(), sz = scl.at("z").get<float>();

    float n = std::sqrt(qx*qx + qy*qy + qz*qz + qw*qw);
    if (n > 1e-8f) { qx/=n; qy/=n; qz/=n; qw/=n; }

    float R[3][3] = {
        {1-2*(qy*qy+qz*qz), 2*(qx*qy-qw*qz),   2*(qx*qz+qw*qy)},
        {2*(qx*qy+qw*qz),   1-2*(qx*qx+qz*qz), 2*(qy*qz-qw*qx)},
        {2*(qx*qz-qw*qy),   2*(qy*qz+qw*qx),   1-2*(qx*qx+qy*qy)}
    };

    pmath::Mat4 m;
    float s[3] = {sx, sy, sz};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            m.m[i][j] = R[i][j] * s[j];
    m.m[0][3] = px; m.m[1][3] = py; m.m[2][3] = pz;
    return m;
}

} // namespace

namespace sceneio {

bool save(const Scene& scene, const std::string& path) {
    json root;
    root["points"] = json::array();
    root["geometry"] = json::array();

    for (auto& [id, obj] : scene.objects) {
        switch (obj->getType()) {
        case ObjectType::Point: {
            root["points"].push_back(json{
                {"id", obj->id},
                {"name", obj->name},
                {"position", positionOf(*obj)}});
            break;
        }
        case ObjectType::Torus: {
            auto* t = static_cast<TorusObject*>(obj.get());
            json scale, rotation;
            decompose(t->transform, scale, rotation);
            root["geometry"].push_back(json{
                {"objectType", "torus"},
                {"id", t->id},
                {"name", t->name},
                {"position", positionOf(*t)},
                {"rotation", rotation},
                {"scale", scale},
                {"samples", json{{"u", t->meshAcc}, {"v", t->meshAcc}}},
                {"smallRadius", t->r},
                {"largeRadius", t->R}});
            break;
        }
        case ObjectType::BezierCurveC0:
        case ObjectType::BezierCurveC2:
        case ObjectType::InterpolatingCurveC2: {
            const char* ot = obj->getType() == ObjectType::BezierCurveC0 ? "bezierC0"
                           : obj->getType() == ObjectType::BezierCurveC2 ? "bezierC2"
                           : "interpolatedC2";
            root["geometry"].push_back(json{
                {"objectType", ot},
                {"id", obj->id},
                {"name", obj->name},
                {"controlPoints", controlPointRefs(obj->getControlPointsList())}});
            break;
        }
        case ObjectType::BezierSurfaceC0:
        case ObjectType::BezierSurfaceC2: {
            auto* s = static_cast<SurfaceObject*>(obj.get());
            bool isC0 = obj->getType() == ObjectType::BezierSurfaceC0;
            int overlap = isC0 ? 1 : 3; // duplicated rows/cols encoding the wrap
            bool wrapU = s->topology == SurfaceObject::Topology::Cylinder
                      && s->wrapAxis == SurfaceObject::WrapAxis::U;
            bool wrapV = s->topology == SurfaceObject::Topology::Cylinder
                      && s->wrapAxis == SurfaceObject::WrapAxis::V;
            int extU = s->gridU + (wrapU ? overlap : 0);
            int extV = s->gridV + (wrapV ? overlap : 0);
            root["geometry"].push_back(json{
                {"objectType", isC0 ? "bezierSurfaceC0" : "bezierSurfaceC2"},
                {"id", s->id},
                {"name", s->name},
                {"controlPoints", surfaceControlPoints(*s, extU, extV, wrapU, wrapV)},
                {"size", json{{"u", extU}, {"v", extV}}},
                {"samples", json{{"u", s->tessLevel}, {"v", s->tessLevel}}}});
            break;
        }
        case ObjectType::GregoryFill:
            break; 
        }
    }

    std::ofstream out(path);
    if (!out) return false;
    out << root.dump(4);
    return out.good();
}

bool load(Scene& scene, const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;

    json root;
    try {
        in >> root;
    } catch (const std::exception&) {
        return false;
    }

    scene.clear();

    try {
        // Pass 1: all points
        if (root.contains("points")) {
            for (const auto& jp : root["points"]) {
                uint32_t id = jp.at("id").get<uint32_t>();
                std::string name = jp.value("name", std::string());
                const auto& pos = jp.at("position");
                auto pt = std::make_shared<PointObject>(id, name);
                pt->transform.shift(pos.at("x").get<float>(),
                                    pos.at("y").get<float>(),
                                    pos.at("z").get<float>());
                scene.objects[id] = pt;
            }
        }

        // Pass 2: geometry
        if (root.contains("geometry")) {
            for (const auto& jg : root["geometry"]) {
                std::string type = jg.at("objectType").get<std::string>();
                uint32_t id = jg.at("id").get<uint32_t>();
                std::string name = jg.value("name", std::string());

                if (type == "torus") {
                    int meshAcc = jg.at("samples").at("u").get<int>();
                    auto t = std::make_shared<TorusObject>(
                        id, name,
                        jg.at("largeRadius").get<float>(),
                        jg.at("smallRadius").get<float>(),
                        meshAcc);
                    t->transform = composeTransform(jg.at("position"), jg.at("rotation"), jg.at("scale"));
                    scene.objects[id] = t;
                } else if (type == "bezierC0" || type == "bezierC2" || type == "interpolatedC2") {
                    std::shared_ptr<CurveObject> curve;
                    if (type == "bezierC0")      curve = std::make_shared<BezierCurveC0>(id, name);
                    else if (type == "bezierC2") curve = std::make_shared<BezierCurveC2>(id, name);
                    else                         curve = std::make_shared<InterpolatingCurveC2>(id, name);
                    for (const auto& cp : jg.at("controlPoints")) {
                        uint32_t pid = cp.at("id").get<uint32_t>();
                        if (auto p = std::dynamic_pointer_cast<PointObject>(scene.findById(pid)))
                            curve->addControlPoint(p);
                    }
                    scene.objects[id] = curve;
                } else if (type == "bezierSurfaceC0" || type == "bezierSurfaceC2") {
                    bool isC0 = (type == "bezierSurfaceC0");
                    int overlap = isC0 ? 1 : 3;
                    int sizeU = jg.at("size").at("u").get<int>();
                    int sizeV = jg.at("size").at("v").get<int>();
                    const auto& jcp = jg.at("controlPoints");
                    auto idAt = [&](int u, int v) {
                        return jcp.at(static_cast<size_t>(v) * sizeU + u).at("id").get<uint32_t>();
                    };

                    // Wrap
                    bool wrapV = sizeV > overlap;
                    for (int k = 0; k < overlap && wrapV; ++k)
                        for (int u = 0; u < sizeU; ++u)
                            if (idAt(u, sizeV - overlap + k) != idAt(u, k)) { wrapV = false; break; }
                    bool wrapU = (!wrapV) && sizeU > overlap;
                    for (int k = 0; k < overlap && wrapU; ++k)
                        for (int v = 0; v < sizeV; ++v)
                            if (idAt(sizeU - overlap + k, v) != idAt(k, v)) { wrapU = false; break; }

                    int gridU = wrapU ? sizeU - overlap : sizeU;
                    int gridV = wrapV ? sizeV - overlap : sizeV;
                    auto patchesFor = [&](int grid, bool closed) {
                        if (isC0) return closed ? grid / 3 : (grid - 1) / 3;
                        return closed ? grid : grid - 3;
                    };

                    std::shared_ptr<SurfaceObject> surf;
                    if (isC0) surf = std::make_shared<BezierSurfaceC0>(id, name);
                    else      surf = std::make_shared<BezierSurfaceC2>(id, name);
                    surf->topology = (wrapU || wrapV) ? SurfaceObject::Topology::Cylinder
                                                      : SurfaceObject::Topology::Plane;
                    surf->wrapAxis = wrapV ? SurfaceObject::WrapAxis::V : SurfaceObject::WrapAxis::U;
                    surf->gridU = gridU;
                    surf->gridV = gridV;
                    surf->patchesU = patchesFor(gridU, wrapU);
                    surf->patchesV = patchesFor(gridV, wrapV);
                    surf->tessLevel = jg.at("samples").at("u").get<int>();
                    surf->controlPoints.reserve(static_cast<size_t>(gridU) * gridV);

                    // Store the unique leading block row-major
                    for (int v = 0; v < gridV; ++v) {
                        for (int u = 0; u < gridU; ++u) {
                            auto p = std::dynamic_pointer_cast<PointObject>(scene.findById(idAt(u, v)));
                            surf->controlPoints.push_back(p);
                            if (p) p->ownerSurfaceId = surf->id;
                        }
                    }
                    surf->needsUpdate = true;
                    scene.objects[id] = surf;
                }
            }
        }
    } catch (const std::exception&) {
        return false;
    }

    scene.normalizeIdsAfterLoad();
    return true;
}

} // namespace sceneio
