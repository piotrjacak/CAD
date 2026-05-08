#pragma once

#include <vector>
#include <cmath>
#include "core/pmath/pmath.h"

pmath::Mat4 createProjectionMatrix(float aspect, float fov, float n, float f);

pmath::Mat4 createOffsetProjectionMatrix(float aspect, float fov, float n, float f,
                                         float eyeWorldX, float convergence);
