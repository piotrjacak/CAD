#pragma once
#include "SurfaceObject.h"
#include <array>
#include <memory>
#include <string>
#include <vector>

class Scene;

namespace objects {

// Gregory patch
class GregoryFillSurface : public SurfaceObject {
public:
    struct HoleEdge {
        std::array<std::weak_ptr<PointObject>, 4> boundary;
        std::array<std::weak_ptr<PointObject>, 4> inner;
    };

    bool showContinuityVectors = false;

    GregoryFillSurface(uint32_t id, std::string name);
    ~GregoryFillSurface() override;

    ObjectType getType() const override { return ObjectType::GregoryFill; }
    void updateGPUBuffers() override;
    void render(const RenderContext& ctx) override;

    // Detect a closed 3-edge hole
    static std::shared_ptr<GregoryFillSurface> createFromSelection(Scene& scene);

private:
    std::vector<HoleEdge> holeEdges;                       // exactly 3
    std::vector<std::array<pmath::Vec3, 20>> patches;      // 3 Gregory patches
    int patchVertexCount = 0;                              // GL_PATCHES index count

    std::vector<pmath::Vec3> continuityPts;                // boundary continuity segments (pairs)
    unsigned int contVAO = 0, contVBO = 0;
    int contVertexCount = 0;

    void collectReferencedPoints();
    void computePatches();
};

} // namespace objects
