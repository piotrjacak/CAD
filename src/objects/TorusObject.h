#pragma once
#include "SceneObject.h"

namespace objects {

class TorusObject : public SceneObject {
public:
    float R = 1.0f;
    float r = 0.3f;
    int meshAcc = 64;

    TorusObject(uint32_t id, std::string name, float R, float r, int meshAcc);
    ~TorusObject() override = default;

    ObjectType getType() const override { return ObjectType::Torus; }
    void updateGPUBuffers() override;
    void render(const RenderContext& ctx) override;
    float getPickRadius() const override;
};

} // namespace objects
