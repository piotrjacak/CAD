#pragma once
#include "CurveObject.h"

namespace objects {

class BezierCurveC0 : public CurveObject {
public:
    BezierCurveC0(uint32_t id, std::string name);
    ~BezierCurveC0() override = default;

    ObjectType getType() const override { return ObjectType::BezierCurveC0; }
    void updateGPUBuffers() override;
    void render(const RenderContext& ctx) override;
    float getPickRadius() const override { return 0.5f; }
};

} // namespace objects
