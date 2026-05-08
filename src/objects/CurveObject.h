#pragma once
#include "SceneObject.h"
#include "PointObject.h"
#include <memory>

namespace objects {

class CurveObject : public SceneObject {
public:
    std::vector<std::weak_ptr<PointObject>> controlPoints;
    bool showPolyline = true;

    using SceneObject::SceneObject;

    void addControlPoint(std::shared_ptr<PointObject> pt);
    void removeControlPoint(uint32_t ptId);
    pmath::Vec3 getCenter() const override;

protected:
    std::vector<pmath::Vec3> resolveControlPoints() const;
};

} // namespace objects
