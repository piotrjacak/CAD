#pragma once

#include "Scene.h"
#include "InputHandler.h"
#include "StereoParams.h"

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
                    int displayW, int displayH,
                    StereoParams& stereo);

private:
    int   newSurfaceC0_patchesU  = 1;
    int   newSurfaceC0_patchesV  = 1;
    int   newSurfaceC0_topology  = 0;   // 0 = Plane, 1 = Cylinder
    float newSurfaceC0_width     = 5.0f;
    float newSurfaceC0_height    = 5.0f;
    float newSurfaceC0_radius    = 2.0f;
    float newSurfaceC0_cylHeight = 5.0f;
};
