#pragma once
#include "SceneObject.h"

namespace objects {

class PointObject : public SceneObject {
public:
    PointObject(uint32_t id, std::string name);
    ~PointObject() override = default;

    ObjectType getType() const override { return ObjectType::Point; }
    void updateGPUBuffers() override;
    void render(const RenderContext& ctx) override;
    float getPickRadius() const override;
};

} // namespace objects
