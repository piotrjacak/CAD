#include "matrices.h"

pmath::Mat4 createProjectionMatrix(float aspect, float fov, float n, float f)
{
	pmath::Mat4 projection(0.0f);

	projection.m[0][0] = 1.0f / (std::tan(fov / 2.0f) * aspect);
	projection.m[1][1] = 1.0f / std::tan(fov / 2.0f);
	projection.m[2][2] = (f + n) / (f - n);
	projection.m[2][3] = (-2.0f * f * n) / (f - n);
	projection.m[3][2] = 1.0f;

	return projection;
}