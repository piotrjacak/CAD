#pragma once

#include <memory>
#include <glad/glad.h>
#include "Scene.h"
#include "core/pmath/pmath.h"
#include "shader.h"

class Renderer {
public:
    void init();
    void renderFrame(
        Scene& scene,
        const pmath::Mat4& sceneModel,
        const pmath::Mat4& view,
        const pmath::Mat4& projection,
        int displayW, int displayH,
        const pmath::Vec3& cursorPos,
        const pmath::Vec3& medianPoint,
        bool showMedian);

private:
    std::unique_ptr<Shader> shaderProgram;
    std::unique_ptr<Shader> cursorShader;
    std::unique_ptr<Shader> bezierShader;
    unsigned int cursorVAO = 0, cursorVBO = 0, cursorEBO = 0;
    unsigned int pointVAO  = 0, pointVBO  = 0;
    int cursorIndexCount   = 0;
};
