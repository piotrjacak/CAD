#pragma once

#include "Scene.h"
#include "InputHandler.h"

struct UIResult {
    bool screenCursorChanged = false;
};

class UI {
public:
    UIResult render(Scene& scene,
                    float& cursorX, float& cursorY, float& cursorZ,
                    float& screenCursorX, float& screenCursorY,
                    PivotType& pivot, RotationAxis& rotAxis,
                    float default_R, float default_r, int default_meshAcc,
                    int displayW, int displayH);
};
