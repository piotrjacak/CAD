#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>

#include "core/pmath/pmath.h"
#include "objects/SceneObject.h"

enum class PivotType { Local, Median, Cursor };
enum class RotationAxis { Free, X, Y, Z };

class Application {
public:
    Application();
    ~Application();

    int run();

private:
    GLFWwindow* window;

    // Speed variables
    float SCROLL_SPEED = 0.1f;
    float SCREEN_SCALE = 1.0f;
    float PAN_SPEED = 0.0025f;
    float ROTATE_SPEED = 0.005f;

    // Scene variables
    float ROT_X = 0.0f;
    float ROT_Y = 0.0f;
    float SHIFT_X = 0.0f;
    float SHIFT_Y = 0.0f;

    // Control variables
    bool CAM_ROTATE_ACTIVE = false;
    bool CAM_PAN_ACTIVE = false;
    bool LMB_PRESSED = false;
    bool RMB_PRESSED = false;
    bool SET_CURSOR_FLAG = false;
    bool PROCESS_SELECTION = false; 
    bool MULTI_SELECT = false;
    bool OBJ_SCALE_ACTIVE = false;
    bool OBJ_ROTATE_ACTIVE = false;
    bool OBJ_TRANSLATE_ACTIVE = false;

    // Position variables
    double TARGET_CURSOR_X = 0.0;
    double TARGET_CURSOR_Y = 0.0;
    double LAST_MOUSE_X = 0.0;
    double LAST_MOUSE_Y = 0.0;
    double START_MOUSE_X = 0.0;
    double START_MOUSE_Y = 0.0;
    double SELECT_MOUSE_X = 0.0;
    double SELECT_MOUSE_Y = 0.0;

    PivotType CURRENT_PIVOT = PivotType::Local;
    RotationAxis CURRENT_ROT_AXIS = RotationAxis::Free;

    float cursorX = 0.0f, cursorY = 0.0f, cursorZ = 0.0f;
    pmath::Vec3 medianPoint{0.0f, 0.0f, 0.0f};
    pmath::Vec3 pivotPoint{0.0f, 0.0f, 0.0f};

    std::vector<objects::SceneObject> sceneObjects;
    int torusCounter = 1;
    int pointCounter = 1;
    int bezierC0Counter = 1;
    int bezierC2Counter = 1;
    int interpC2Counter = 1;
    uint32_t globalObjectIdCounter = 1;

    std::vector<float> cursorVertices;
    std::vector<unsigned int> cursorIndices;

    float default_R = 1.0f;
    float default_r = 0.3f;
    int default_meshAcc = 64;

    void processInput(GLFWwindow* window);
    
    // Callbacks implementacje
    void scroll_callback_impl(double xoffset, double yoffset);
    void mouse_button_callback_impl(int button, int action, int mods);
    void cursor_position_callback_impl(double xpos, double ypos);
    void framebuffer_size_callback_impl(int width, int height);

    // Callbacks statyczne kierujące do instancji
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
};
