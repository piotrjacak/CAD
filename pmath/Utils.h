#pragma once

#include <vector>
#include <cmath>

namespace pmath {

	const float PI = 2.0f * std::acos(0.0f);

	inline float degToRad(float degrees) {
		return degrees * (PI / 180.0f);
	}

	inline float radToDeg(float radians) {
		return radians * (180.0f / PI);
	}
}