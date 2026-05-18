#include "Renderer.h"
#include "objects/SceneObject.h"
#include "matrices.h"
#include "core/pmath/pmath.h"

void Renderer::init() {
    shaderProgram   = std::make_unique<Shader>("shader.vs", "shader.fs");
    cursorShader    = std::make_unique<Shader>("cursorShader.vs", "cursorShader.fs");
    bezierShader    = std::make_unique<Shader>("bezierShader.vs", "bezierShader.fs",
                                               "bezierShader.tcs", "bezierShader.tes");
    bezierSurfaceC0Shader = std::make_unique<Shader>("bezierSurface.vs", "bezierSurface.fs",
                                                     "bezierSurfaceC0.tcs", "bezierSurface.tes");
    compositeShader = std::make_unique<Shader>("anaglyphComposite.vs", "anaglyphComposite.fs");

    compositeShader->use();
    compositeShader->setInt("leftEyeTex",  0);
    compositeShader->setInt("rightEyeTex", 1);

    static const float cursorVerts[] = {
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.9f, 0.1f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.9f,-0.1f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
       -0.1f, 0.9f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.1f, 0.9f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
       -0.1f, 0.0f, 0.9f, 0.0f, 0.0f, 1.0f,
        0.1f, 0.0f, 0.9f, 0.0f, 0.0f, 1.0f
    };
    static const unsigned int cursorIdx[] = {
        0, 1, 1, 2, 1, 3, 4, 5, 5, 6, 5, 7, 8, 9, 9, 10, 9, 11
    };
    cursorIndexCount = (int)(sizeof(cursorIdx) / sizeof(unsigned int));

    glGenVertexArrays(1, &cursorVAO);
    glGenBuffers(1, &cursorVBO);
    glGenBuffers(1, &cursorEBO);
    glBindVertexArray(cursorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cursorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cursorVerts), cursorVerts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cursorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cursorIdx), cursorIdx, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    static const float pointData[] = { 0.0f, 0.0f, 0.0f };
    glGenVertexArrays(1, &pointVAO);
    glGenBuffers(1, &pointVBO);
    glBindVertexArray(pointVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pointData), pointData, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Pełnoekranowy quad w NDC: dwa trójkąty, format [x, y, u, v]
    static const float quadVerts[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
    };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    glEnable(GL_PROGRAM_POINT_SIZE);
}

void Renderer::cleanup() {
    for (int i = 0; i < 2; i++) {
        if (stereoFBO[i])      { glDeleteFramebuffers(1,  &stereoFBO[i]);      stereoFBO[i]      = 0; }
        if (stereoColorTex[i]) { glDeleteTextures(1,      &stereoColorTex[i]); stereoColorTex[i] = 0; }
        if (stereoDepthRBO[i]) { glDeleteRenderbuffers(1, &stereoDepthRBO[i]); stereoDepthRBO[i] = 0; }
    }
    if (quadVAO) { glDeleteVertexArrays(1, &quadVAO); quadVAO = 0; }
    if (quadVBO) { glDeleteBuffers(1,      &quadVBO); quadVBO = 0; }
}

void Renderer::initStereoFBOs(int w, int h) {
    for (int i = 0; i < 2; i++) {
        glGenFramebuffers(1, &stereoFBO[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, stereoFBO[i]);

        glGenTextures(1, &stereoColorTex[i]);
        glBindTexture(GL_TEXTURE_2D, stereoColorTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, stereoColorTex[i], 0);

        glGenRenderbuffers(1, &stereoDepthRBO[i]);
        glBindRenderbuffer(GL_RENDERBUFFER, stereoDepthRBO[i]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, stereoDepthRBO[i]);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    stereoFboW = w;
    stereoFboH = h;
}

void Renderer::resizeStereoFBOs(int w, int h) {
    if (w == stereoFboW && h == stereoFboH) return;
    for (int i = 0; i < 2; i++) {
        if (stereoFBO[i])      { glDeleteFramebuffers(1,  &stereoFBO[i]);      stereoFBO[i]      = 0; }
        if (stereoColorTex[i]) { glDeleteTextures(1,      &stereoColorTex[i]); stereoColorTex[i] = 0; }
        if (stereoDepthRBO[i]) { glDeleteRenderbuffers(1, &stereoDepthRBO[i]); stereoDepthRBO[i] = 0; }
    }
    initStereoFBOs(w, h);
}

void Renderer::renderSceneGeometry(
    Scene& scene,
    const pmath::Mat4& sceneModel,
    const pmath::Mat4& view,
    const pmath::Mat4& projection,
    int displayW, int displayH,
    const pmath::Vec3& cursorPos,
    const pmath::Vec3& medianPoint,
    bool showMedian)
{
    glClearColor(0.18f, 0.18f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (showMedian)
    {
        pmath::Mat4 medianLocal;
        medianLocal.shift(medianPoint.x, medianPoint.y, medianPoint.z);
        pmath::Mat4 medianModel = sceneModel * medianLocal;

        shaderProgram->use();
        shaderProgram->setMat4("model",      medianModel);
        shaderProgram->setMat4("view",       view);
        shaderProgram->setMat4("projection", projection);
        shaderProgram->setVec3("uColor", 1.0f, 0.5f, 0.0f);

        glBindVertexArray(pointVAO);
        glPointSize(15.0f);
        glDrawArrays(GL_POINTS, 0, 1);
        glBindVertexArray(0);
    }

    pmath::Mat4 cursorLocal;
    cursorLocal.shift(cursorPos.x, cursorPos.y, cursorPos.z);
    pmath::Mat4 cursorModel = sceneModel * cursorLocal;

    cursorShader->use();
    cursorShader->setMat4("model",      cursorModel);
    cursorShader->setMat4("view",       view);
    cursorShader->setMat4("projection", projection);
    glLineWidth(3.0f);
    glBindVertexArray(cursorVAO);
    glDrawElements(GL_LINES, cursorIndexCount, GL_UNSIGNED_INT, 0);
    glLineWidth(1.0f);
    glBindVertexArray(0);

    objects::RenderContext ctx;
    ctx.sceneModel     = sceneModel;
    ctx.view           = view;
    ctx.projection     = projection;
    ctx.displayW       = displayW;
    ctx.displayH       = displayH;
    ctx.shaderProgram         = shaderProgram.get();
    ctx.bezierShader          = bezierShader.get();
    ctx.bezierSurfaceC0Shader = bezierSurfaceC0Shader.get();
    ctx.sharedPointVAO        = pointVAO;

    for (auto& [id, obj] : scene.objects)
    {
        if (obj->needsUpdate)
            obj->updateGPUBuffers();
        obj->render(ctx);
    }
}

void Renderer::renderFrame(
    Scene& scene,
    const pmath::Mat4& sceneModel,
    const pmath::Mat4& view,
    const pmath::Mat4& projection,
    int displayW, int displayH,
    const pmath::Vec3& cursorPos,
    const pmath::Vec3& medianPoint,
    bool showMedian)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, displayW, displayH);
    renderSceneGeometry(scene, sceneModel, view, projection,
        displayW, displayH, cursorPos, medianPoint, showMedian);
}

void Renderer::renderFrameStereo(
    Scene& scene,
    const pmath::Mat4& sceneModel,
    const pmath::Mat4& baseView,
    int displayW, int displayH,
    const pmath::Vec3& cursorPos,
    const pmath::Vec3& medianPoint,
    bool showMedian,
    const StereoParams& stereo,
    float aspect, float fov, float nearP, float farP)
{
    resizeStereoFBOs(displayW, displayH);

    float halfEye = stereo.eyeSep / 2.0f;

    // Lewe oko
    pmath::Mat4 viewLeft;
    viewLeft.shift(+halfEye, 0.0f, 5.0f);
    pmath::Mat4 projLeft = createOffsetProjectionMatrix(
        aspect, fov, nearP, farP, -halfEye, stereo.convergence);

    // Prawe oko
    pmath::Mat4 viewRight;
    viewRight.shift(-halfEye, 0.0f, 5.0f);
    pmath::Mat4 projRight = createOffsetProjectionMatrix(
        aspect, fov, nearP, farP, +halfEye, stereo.convergence);

    // lewe oko - FBO[0]
    glBindFramebuffer(GL_FRAMEBUFFER, stereoFBO[0]);
    glViewport(0, 0, displayW, displayH);
    renderSceneGeometry(scene, sceneModel, viewLeft, projLeft,
        displayW, displayH, cursorPos, medianPoint, showMedian);

    // prawe oko - FBO[1]
    glBindFramebuffer(GL_FRAMEBUFFER, stereoFBO[1]);
    glViewport(0, 0, displayW, displayH);
    renderSceneGeometry(scene, sceneModel, viewRight, projRight,
        displayW, displayH, cursorPos, medianPoint, showMedian);

    // framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, displayW, displayH);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    compositeShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, stereoColorTex[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, stereoColorTex[1]);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}
