#include "CurveObject.h"
#include "PointObject.h"
#include <algorithm>

namespace objects {

void CurveObject::addControlPoint(std::shared_ptr<PointObject> pt) {
    if (!pt) return;
    for (auto& wp : controlPoints) {
        if (auto existing = wp.lock())
            if (existing->id == pt->id) return;
    }
    controlPoints.push_back(pt);
    needsUpdate = true;
}

void CurveObject::removeControlPoint(uint32_t ptId) {
    size_t before = controlPoints.size();
    auto it = std::remove_if(controlPoints.begin(), controlPoints.end(),
        [ptId](const std::weak_ptr<PointObject>& wp) {
            auto pt = wp.lock();
            return !pt || pt->id == ptId;
        });
    controlPoints.erase(it, controlPoints.end());
    if (controlPoints.size() != before)
        needsUpdate = true;
}

std::vector<pmath::Vec3> CurveObject::resolveControlPoints() const {
    std::vector<pmath::Vec3> pts;
    for (const auto& wp : controlPoints) {
        if (auto pt = wp.lock())
            pts.push_back(pmath::Vec3(
                pt->transform.m[0][3],
                pt->transform.m[1][3],
                pt->transform.m[2][3]));
    }
    return pts;
}

pmath::Vec3 CurveObject::getCenter() const {
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
