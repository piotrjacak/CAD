#pragma once
#include "SurfaceObject.h"
#include <memory>

class Scene;

namespace objects {

class BezierSurfaceC2 : public SurfaceObject {
public:
    BezierSurfaceC2(uint32_t id, std::string name);
    ~BezierSurfaceC2() override = default;

    ObjectType getType() const override { return ObjectType::BezierSurfaceC2; }
    void updateGPUBuffers() override;
    void render(const RenderContext& ctx) override;

    static std::shared_ptr<BezierSurfaceC2> createPlane(
        Scene& scene,
        int patchesU, int patchesV,
        float width, float height,
        const pmath::Vec3& origin);

    static std::shared_ptr<BezierSurfaceC2> createCylinder(
        Scene& scene,
        int patchesU, int patchesV,
        float radius, float height,
        const pmath::Vec3& origin);

private:
    int meshLineIndexCount = 0;
    int patchIndexCount    = 0;
};

} // namespace objects
