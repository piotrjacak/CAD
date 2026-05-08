#pragma once
#include "CurveObject.h"

namespace objects {

class BezierCurveC2 : public CurveObject {
public:
    bool isBsplineBasis = true;
    int selectedVirtualPointIndex = -1;
    std::vector<pmath::Vec3> virtualPoints;

    BezierCurveC2(uint32_t id, std::string name);
    ~BezierCurveC2() override = default;

    ObjectType getType() const override { return ObjectType::BezierCurveC2; }
    void updateGPUBuffers() override;
    void render(const RenderContext& ctx) override;
    float getPickRadius() const override { return 0.5f; }

private:
    int polylineIndexCount = 0;
    int curveIndexCount = 0;
};

} // namespace objects
