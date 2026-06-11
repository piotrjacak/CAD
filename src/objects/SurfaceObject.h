#pragma once
#include "SceneObject.h"
#include "PointObject.h"
#include <memory>
#include <vector>

namespace objects {

class SurfaceObject : public SceneObject {
public:
    enum class Topology { Plane, Cylinder };
    enum class WrapAxis { U, V };

    Topology topology = Topology::Plane;
    WrapAxis wrapAxis = WrapAxis::U;
    int patchesU = 1;
    int patchesV = 1;
    int tessLevel = 4;
    bool showControlMesh = false;

    std::vector<std::weak_ptr<PointObject>> controlPoints;
    int gridU = 0;
    int gridV = 0;

    using SceneObject::SceneObject;

    pmath::Vec3 getCenter() const override;
    float getPickRadius() const override { return 1.0f; }
    std::vector<std::weak_ptr<PointObject>>* getControlPointsList() override { return &controlPoints; }

    // Control point at grid (u,v)
    std::shared_ptr<PointObject> controlPointAt(int u, int v) const;

protected:
    int gridIndex(int u, int v) const;
    std::vector<pmath::Vec3> resolveControlPoints() const;
};

} // namespace objects
