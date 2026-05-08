#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "core/pmath/pmath.h"
#include "Scene.h"
#include "Renderer.h"
#include "InputHandler.h"
#include "UI.h"

class Application {
public:
    Application();
    ~Application();

    int run();

private:
    GLFWwindow* window;

    float cursorX = 0.0f, cursorY = 0.0f, cursorZ = 0.0f;
    float screenCursorX = 0.0f, screenCursorY = 0.0f;
    pmath::Vec3 medianPoint{0.0f, 0.0f, 0.0f};

    Scene        scene;
    Renderer     renderer;
    InputHandler inputHandler;
    UI           ui;
    StereoParams stereoParams;

    float default_R = 1.0f;
    float default_r = 0.3f;
    int   default_meshAcc = 64;

    void processInput(GLFWwindow* window);

    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
};
