#include "SurfaceObject.h"
#include "PointObject.h"

namespace objects {

int SurfaceObject::gridIndex(int u, int v) const {
    if (topology == Topology::Cylinder) {
        if (wrapAxis == WrapAxis::U && gridU > 0) u = ((u % gridU) + gridU) % gridU;
        if (wrapAxis == WrapAxis::V && gridV > 0) v = ((v % gridV) + gridV) % gridV;
    }
    return v * gridU + u;
}

std::shared_ptr<PointObject> SurfaceObject::controlPointAt(int u, int v) const {
    int idx = gridIndex(u, v);
    if (idx < 0 || idx >= static_cast<int>(controlPoints.size())) return nullptr;
    return controlPoints[idx].lock();
}

std::vector<pmath::Vec3> SurfaceObject::resolveControlPoints() const {
    std::vector<pmath::Vec3> pts;
    pts.reserve(controlPoints.size());
    for (const auto& wp : controlPoints) {
        if (auto pt = wp.lock()) {
            pts.push_back(pmath::Vec3(
                pt->transform.m[0][3],
                pt->transform.m[1][3],
                pt->transform.m[2][3]));
        } else {
            pts.push_back(pmath::Vec3(0.0f, 0.0f, 0.0f));
        }
    }
    return pts;
}

pmath::Vec3 SurfaceObject::getCenter() const {
    pmath::Vec3 center(0.0f, 0.0f, 0.0f);
    int count = 0;
    for (const auto& wp : controlPoints) {
        if (auto pt = wp.lock()) {
            center = center + pmath::Vec3(
                pt->transform.m[0][3],
                pt->transform.m[1][3],
                pt->transform.m[2][3]);
            ++count;
        }
    }
    if (count > 0) return center * (1.0f / static_cast<float>(count));
    return SceneObject::getCenter();
}

} // namespace objects
