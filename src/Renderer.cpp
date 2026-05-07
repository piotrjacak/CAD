#include "Renderer.h"
#include "objects/SceneObject.h"

void Renderer::init() {
    shaderProgram = std::make_unique<Shader>("shader.vs", "shader.fs");
    cursorShader  = std::make_unique<Shader>("cursorShader.vs", "cursorShader.fs");
    bezierShader  = std::make_unique<Shader>("bezierShader.vs", "bezierShader.fs",
                                              "bezierShader.tcs", "bezierShader.tes");

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

    glEnable(GL_PROGRAM_POINT_SIZE);
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
    glClearColor(0.18f, 0.18f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (showMedian)
    {
        pmath::Mat4 medianLocal;
        medianLocal.shift(medianPoint.x, medianPoint.y, medianPoint.z);
        pmath::Mat4 medianModel = sceneModel * medianLocal;

        shaderProgram->use();
        shaderProgram->setMat4("model", medianModel);
        shaderProgram->setMat4("view", view);
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
    cursorShader->setMat4("model", cursorModel);
    cursorShader->setMat4("view", view);
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
    ctx.shaderProgram  = shaderProgram.get();
    ctx.bezierShader   = bezierShader.get();
    ctx.sharedPointVAO = pointVAO;

    for (auto& [id, obj] : scene.objects)
    {
        if (obj->needsUpdate)
            obj->updateGPUBuffers();
        obj->render(ctx);
    }
}
