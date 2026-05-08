#pragma once
#include "CurveObject.h"

namespace objects {

class InterpolatingCurveC2 : public CurveObject {
public:
    InterpolatingCurveC2(uint32_t id, std::string name);
    ~InterpolatingCurveC2() override = default;

    ObjectType getType() const override { return ObjectType::InterpolatingCurveC2; }
    void updateGPUBuffers() override;
    void render(const RenderContext& ctx) override;
    float getPickRadius() const override { return 0.5f; }

private:
    std::vector<pmath::Vec3> interpolatedBezierPoints;
    int curveIndexCount = 0;

    static std::vector<pmath::Vec3> solveCubicSpline(const std::vector<pmath::Vec3>& pts);
};

} // namespace objects
