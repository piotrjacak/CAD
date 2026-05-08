#include "Application.h"
#include <iostream>

#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui.h"

#include "matrices.h"

const unsigned int SCR_WIDTH = 1800;
const unsigned int SCR_HEIGHT = 1200;

Application::Application() {
    window = nullptr;
}

Application::~Application() {
    if (window) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}

int Application::run() {

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "CAD", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glEnable(GL_DEPTH_TEST);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
    // High res Imgui scaling
    float scale_factor = 1.5f;
    ImGui::GetStyle().ScaleAllSizes(scale_factor);
    io.FontGlobalScale = scale_factor;

    renderer.init();

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        UIResult uiResult = ui.render(
            scene,
            cursorX, cursorY, cursorZ,
            screenCursorX, screenCursorY,
            inputHandler.CURRENT_PIVOT, inputHandler.CURRENT_ROT_AXIS,
            default_R, default_r, default_meshAcc,
            display_w, display_h,
            stereoParams);

        // Scene matrices
        pmath::Mat4 sceneModel;
        sceneModel.scale(inputHandler.SCREEN_SCALE, inputHandler.SCREEN_SCALE, inputHandler.SCREEN_SCALE);
        sceneModel.rotateX(inputHandler.ROT_X);
        sceneModel.rotateY(inputHandler.ROT_Y);
        sceneModel.shift(inputHandler.SHIFT_X, inputHandler.SHIFT_Y, 0.0f);

        pmath::Mat4 view;
        view.shift(0.0f, 0.0f, 5.0f);

        float aspect = (float)display_w / (float)display_h;
        pmath::Mat4 projection = createProjectionMatrix(aspect, pmath::degToRad(45.0f), 0.1f, 100.0f);

        // Cursor position sync
        pmath::Vec4 cursorWorldPos(cursorX, cursorY, cursorZ, 1.0f);
        pmath::Vec4 clipSpace = projection * view * sceneModel * cursorWorldPos;
        float ndcX = clipSpace.x / clipSpace.w;
        float ndcY = clipSpace.y / clipSpace.w;
        float ndcZ = clipSpace.z / clipSpace.w;

        if (uiResult.screenCursorChanged)
        {
            float newNdcX = (screenCursorX / display_w) * 2.0f - 1.0f;
            float newNdcY = 1.0f - (screenCursorY / display_h) * 2.0f;

            pmath::Mat4 invVP = projection * view * sceneModel;
            invVP.inverse();

            pmath::Vec4 newWorld = invVP * pmath::Vec4(newNdcX, newNdcY, ndcZ, 1.0f);
            cursorX = newWorld.x / newWorld.w;
            cursorY = newWorld.y / newWorld.w;
            cursorZ = newWorld.z / newWorld.w;
        }
        else if (inputHandler.SET_CURSOR_FLAG)
        {
            float newNdcX = (float)(inputHandler.TARGET_CURSOR_X / display_w) * 2.0f - 1.0f;
            float newNdcY = 1.0f - (float)(inputHandler.TARGET_CURSOR_Y / display_h) * 2.0f;

            pmath::Mat4 invVP = projection * view * sceneModel;
            invVP.inverse();

            pmath::Vec4 newWorld = invVP * pmath::Vec4(newNdcX, newNdcY, ndcZ, 1.0f);
            cursorX = newWorld.x / newWorld.w;
            cursorY = newWorld.y / newWorld.w;
            cursorZ = newWorld.z / newWorld.w;

            inputHandler.SET_CURSOR_FLAG = false;
        }
        else
        {
            screenCursorX = (ndcX + 1.0f) * 0.5f * display_w;
            screenCursorY = (1.0f - ndcY) * 0.5f * display_h;
        }

        // Selection
        if (inputHandler.PROCESS_SELECTION)
            inputHandler.processSelection(scene, projection, view, sceneModel, display_w, display_h);

        // Median point
        int selectedCount = 0;
        medianPoint = pmath::Vec3(0.0f, 0.0f, 0.0f);
        for (const auto& [id, obj] : scene.objects) {
            if (obj->isSelected) {
                medianPoint = medianPoint + obj->getCenter();
                selectedCount++;
            }
        }
        if (selectedCount > 0)
            medianPoint = medianPoint * (1.0f / static_cast<float>(selectedCount));

        if (stereoParams.enabled) {
            renderer.renderFrameStereo(scene, sceneModel, view,
                display_w, display_h,
                pmath::Vec3(cursorX, cursorY, cursorZ),
                medianPoint, selectedCount > 0,
                stereoParams,
                aspect, pmath::degToRad(45.0f), 0.1f, 100.0f);
        } else {
            renderer.renderFrame(scene, sceneModel, view, projection,
                display_w, display_h,
                pmath::Vec3(cursorX, cursorY, cursorZ),
                medianPoint, selectedCount > 0);
        }

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}

void Application::processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void Application::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->inputHandler.onScroll(yoffset);
}
void Application::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->inputHandler.onMouseButton(button, action, mods, window, app->scene,
        pmath::Vec3(app->cursorX, app->cursorY, app->cursorZ), app->medianPoint);
}
void Application::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->inputHandler.onCursorPos(xpos, ypos, app->scene);
}
void Application::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) glViewport(0, 0, width, height);
}
