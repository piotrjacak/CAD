#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../core/pmath/pmath.h"

class Shader;

namespace objects {

class PointObject;

enum class ObjectType { Torus, Point, BezierCurveC0, BezierCurveC2, InterpolatingCurveC2, BezierSurfaceC0, BezierSurfaceC2 };

struct RenderContext {
    pmath::Mat4 sceneModel;
    pmath::Mat4 view;
    pmath::Mat4 projection;
    int displayW = 0;
    int displayH = 0;
    Shader* shaderProgram = nullptr;
    Shader* bezierShader = nullptr;
    Shader* bezierSurfaceC0Shader = nullptr;
    Shader* bezierSurfaceC2Shader = nullptr;
    unsigned int sharedPointVAO = 0;
};

class SceneObject {
public:
    uint32_t id = 0;
    std::string name;
    bool isSelected = false;
    pmath::Mat4 transform;
    pmath::Mat4 initialTransform;
    bool needsUpdate = true;

    SceneObject(uint32_t id, std::string name);
    virtual ~SceneObject();

    virtual ObjectType getType() const = 0;
    virtual void updateGPUBuffers() = 0;
    virtual void render(const RenderContext& ctx) = 0;
    virtual pmath::Vec3 getCenter() const;
    virtual float getPickRadius() const = 0;
    virtual std::vector<std::weak_ptr<PointObject>>* getControlPointsList() { return nullptr; }

protected:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    int indexCount = 0;
    void freeGPU();
};

} // namespace objects
