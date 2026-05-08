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

pmath::Mat4 createOffsetProjectionMatrix(float aspect, float fov, float n, float f,
                                         float eyeWorldX, float convergence)
{
	float half_h = n * std::tan(fov / 2.0f);
	float half_w = half_h * aspect;

	float delta = eyeWorldX * n / convergence;

	float l = -half_w + delta;
	float r =  half_w + delta;
	float t =  half_h;
	float b = -half_h;

	pmath::Mat4 proj(0.0f);
	proj.m[0][0] = (2.0f * n) / (r - l);
	proj.m[0][2] = (r + l)    / (r - l);
	proj.m[1][1] = (2.0f * n) / (t - b);
	proj.m[1][2] = (t + b)    / (t - b);
	proj.m[2][2] = (f + n)    / (f - n);
	proj.m[2][3] = (-2.0f * f * n) / (f - n);
	proj.m[3][2] = 1.0f;

	return proj;
}