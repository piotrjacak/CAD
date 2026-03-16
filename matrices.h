#pragma once

#include <vector>
#include <cmath>
#include "pmath/pmath.h"

pmath::Mat4 createProjectionMatrix(float aspect, float fov, float n, float f);
