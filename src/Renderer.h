#pragma once

#include <memory>
#include <glad/glad.h>
#include "Scene.h"
#include "core/pmath/pmath.h"
#include "shader.h"
#include "StereoParams.h"

class Renderer {
public:
    void init();
    void cleanup();

    void renderFrame(
        Scene& scene,
        const pmath::Mat4& sceneModel,
        const pmath::Mat4& view,
        const pmath::Mat4& projection,
        int displayW, int displayH,
        const pmath::Vec3& cursorPos,
        const pmath::Vec3& medianPoint,
        bool showMedian);

    void renderFrameStereo(
        Scene& scene,
        const pmath::Mat4& sceneModel,
        const pmath::Mat4& baseView,
        int displayW, int displayH,
        const pmath::Vec3& cursorPos,
        const pmath::Vec3& medianPoint,
        bool showMedian,
        const StereoParams& stereo,
        float aspect, float fov, float nearP, float farP);

private:
    void renderSceneGeometry(
        Scene& scene,
        const pmath::Mat4& sceneModel,
        const pmath::Mat4& view,
        const pmath::Mat4& projection,
        int displayW, int displayH,
        const pmath::Vec3& cursorPos,
        const pmath::Vec3& medianPoint,
        bool showMedian);

    void initStereoFBOs(int w, int h);
    void resizeStereoFBOs(int w, int h);

    std::unique_ptr<Shader> shaderProgram;
    std::unique_ptr<Shader> cursorShader;
    std::unique_ptr<Shader> bezierShader;
    std::unique_ptr<Shader> bezierSurfaceC0Shader;
    std::unique_ptr<Shader> compositeShader;

    unsigned int cursorVAO = 0, cursorVBO = 0, cursorEBO = 0;
    unsigned int pointVAO  = 0, pointVBO  = 0;
    int cursorIndexCount   = 0;

    // FBO per oko
    unsigned int stereoFBO[2]      = {0, 0};
    unsigned int stereoColorTex[2] = {0, 0};
    unsigned int stereoDepthRBO[2] = {0, 0};
    int stereoFboW = 0, stereoFboH = 0;

    unsigned int quadVAO = 0, quadVBO = 0;
};
