#pragma once

#include <GLFW/glfw3.h>
#include "core/pmath/pmath.h"
#include "Scene.h"

enum class PivotType  { Local, Median, Cursor };
enum class RotationAxis { Free, X, Y, Z };

class InputHandler {
public:
    float SCREEN_SCALE = 1.0f;
    float ROT_X = 0.0f, ROT_Y = 0.0f;
    float SHIFT_X = 0.0f, SHIFT_Y = 0.0f;
    PivotType    CURRENT_PIVOT    = PivotType::Local;
    RotationAxis CURRENT_ROT_AXIS = RotationAxis::Free;
    bool   SET_CURSOR_FLAG = false;
    double TARGET_CURSOR_X = 0.0, TARGET_CURSOR_Y = 0.0;
    bool   PROCESS_SELECTION = false;
    double SELECT_MOUSE_X = 0.0, SELECT_MOUSE_Y = 0.0;
    pmath::Vec3 pivotPoint{0.0f, 0.0f, 0.0f};

    void onScroll(double yoffset);
    void onMouseButton(int button, int action, int mods,
                       GLFWwindow* window, Scene& scene,
                       const pmath::Vec3& cursorPos,
                       const pmath::Vec3& medianPoint);
    void onCursorPos(double xpos, double ypos, Scene& scene);
    void processSelection(Scene& scene,
                          const pmath::Mat4& projection,
                          const pmath::Mat4& view,
                          const pmath::Mat4& sceneModel,
                          int displayW, int displayH);

private:
    static constexpr float SCROLL_SPEED  = 0.1f;
    static constexpr float PAN_SPEED     = 0.0025f;
    static constexpr float ROTATE_SPEED  = 0.005f;

    bool CAM_ROTATE_ACTIVE    = false;
    bool CAM_PAN_ACTIVE       = false;
    bool OBJ_SCALE_ACTIVE     = false;
    bool OBJ_ROTATE_ACTIVE    = false;
    bool OBJ_TRANSLATE_ACTIVE = false;
    bool LMB_PRESSED  = false;
    bool RMB_PRESSED  = false;
    bool MULTI_SELECT = false;
    double LAST_MOUSE_X  = 0.0, LAST_MOUSE_Y  = 0.0;
    double START_MOUSE_X = 0.0, START_MOUSE_Y = 0.0;
};
