#pragma once
#include "SurfaceObject.h"
#include <memory>

class Scene;

namespace objects {

class BezierSurfaceC0 : public SurfaceObject {
public:
    BezierSurfaceC0(uint32_t id, std::string name);
    ~BezierSurfaceC0() override = default;

    ObjectType getType() const override { return ObjectType::BezierSurfaceC0; }
    void updateGPUBuffers() override;
    void render(const RenderContext& ctx) override;

    static std::shared_ptr<BezierSurfaceC0> createPlane(
        Scene& scene,
        int patchesU, int patchesV,
        float width, float height,
        const pmath::Vec3& origin);

    static std::shared_ptr<BezierSurfaceC0> createCylinder(
        Scene& scene,
        int patchesU, int patchesV,
        float radius, float height,
        const pmath::Vec3& origin);

private:
    int meshLineIndexCount = 0;
    int patchIndexCount    = 0;
};

} // namespace objects
