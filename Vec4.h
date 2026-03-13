#pragma once
#include <cmath>

namespace pmath {

	struct Vec4 {
		union {
			struct { float x, y, z, w; };
			float data[4];
		};

		// Constructors
		Vec4() : x(0), y(0), z(0), w(0) {}
		Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

		// Operators
		Vec4 operator+(const Vec4& other) const {
			return Vec4(x + other.x, y + other.y, z + other.z, w + other.w);
		}
		Vec4 operator-(const Vec4& other) const {
			return Vec4(x - other.x, y - other.y, z - other.z, w - other.w);
		}
		Vec4 operator*(float scalar) const {
			return Vec4(x * scalar, y * scalar, z * scalar, w * scalar);
		}

		// Dot product
		float dot(const Vec4& other) const {
			return x * other.x + y * other.y + z * other.z + w * other.w;
		}

		// Length of the vector (magnitude)
		float length() const {
			return std::sqrt(x * x + y * y + z * z + w * w);
		}

		// Normalization
		Vec4 normalize() const {
			float len = length();
			if (len > 0.0f) {
				return *this * (1.0f / len);
			}
			return *this;
		}
	};


}
