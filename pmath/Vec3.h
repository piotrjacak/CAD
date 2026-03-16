#pragma once
#include <cmath>

namespace pmath {

	struct Vec3 {
		union {
			struct { float x, y, z; };
			float data[3];
		};
		// Constructors
		Vec3() : x(0), y(0), z(0) {}
		Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

		// Operators
		Vec3 operator+(const Vec3& other) const {
			return Vec3(x + other.x, y + other.y, z + other.z);
		}
		Vec3 operator-(const Vec3& other) const {
			return Vec3(x - other.x, y - other.y, z - other.z);
		}
		Vec3 operator*(float scalar) const {
			return Vec3(x * scalar, y * scalar, z * scalar);
		}

		// Dot product
		float dot(const Vec3& other) const {
			return x * other.x + y * other.y + z * other.z;
		}

		// Length of the vector (magnitude)
		float length() const {
			return std::sqrt(x * x + y * y + z * z);
		}

		// Normalization
		Vec3 normalize() const {
			float len = length();
			if (len > 0.0f) {
				return *this * (1.0f / len);
			}
			return *this;
		}
	};
}