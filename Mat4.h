#pragma once
#include <cmath>
#include "Vec4.h"

namespace pmath {

	struct Mat4 {

		float m[4][4];

		// Constructors
		Mat4() {
			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					if (i == j)
						m[i][j] = 1.0f;
					else
						m[i][j] = 0.0f;
				}
			}
		}
		Mat4(float diagonal) {
			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					m[i][j] = (i == j) ? diagonal : 0.0f;
				}
			}
		}
		Mat4(float d0, float d1, float d2, float d3) {
			m[0][0] = d0; m[0][1] = 0.0f; m[0][2] = 0.0f; m[0][3] = 0.0f;
			m[1][0] = 0.0f; m[1][1] = d1; m[1][2] = 0.0f; m[1][3] = 0.0f;
			m[2][0] = 0.0f; m[2][1] = 0.0f; m[2][2] = d2; m[2][3] = 0.0f;
			m[3][0] = 0.0f; m[3][1] = 0.0f; m[3][2] = 0.0f; m[3][3] = d3;
		}

		// Vector multiplication
		Vec4 operator*(const Vec4& v) const {
			return Vec4(
				m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w,
				m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w,
				m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w,
				m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w
			);
		}

		// Matrix multiplication
		Mat4 operator*(const Mat4& other) const {
			Mat4 result;
			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					result.m[i][j] = m[i][0] * other.m[0][j] + m[i][1] * other.m[1][j] + m[i][2] * other.m[2][j] + m[i][3] * other.m[3][j];
				}
			}
			return result;
		}

		// Transpose
		Mat4 transpose() const {
			Mat4 result;
			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					result.m[i][j] = m[j][i];
				}
			}
			return result;
		}

		// Inverse
		Mat4 inverse() const {
			Mat4 inv;

			float A2323 = m[2][2] * m[3][3] - m[2][3] * m[3][2];
			float A1323 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
			float A1223 = m[2][1] * m[3][2] - m[2][2] * m[3][1];
			float A0323 = m[2][0] * m[3][3] - m[2][3] * m[3][0];
			float A0223 = m[2][0] * m[3][2] - m[2][2] * m[3][0];
			float A0123 = m[2][0] * m[3][1] - m[2][1] * m[3][0];
			float A2313 = m[1][2] * m[3][3] - m[1][3] * m[3][2];
			float A1313 = m[1][1] * m[3][3] - m[1][3] * m[3][1];
			float A1213 = m[1][1] * m[3][2] - m[1][2] * m[3][1];
			float A2312 = m[1][2] * m[2][3] - m[1][3] * m[2][2];
			float A1312 = m[1][1] * m[2][3] - m[1][3] * m[2][1];
			float A1212 = m[1][1] * m[2][2] - m[1][2] * m[2][1];
			float A0313 = m[1][0] * m[3][3] - m[1][3] * m[3][0];
			float A0213 = m[1][0] * m[3][2] - m[1][2] * m[3][0];
			float A0312 = m[1][0] * m[2][3] - m[1][3] * m[2][0];
			float A0212 = m[1][0] * m[2][2] - m[1][2] * m[2][0];
			float A0113 = m[1][0] * m[3][1] - m[1][1] * m[3][0];
			float A0112 = m[1][0] * m[2][1] - m[1][1] * m[2][0];

			float det = m[0][0] * (m[1][1] * A2323 - m[1][2] * A1323 + m[1][3] * A1223)
				- m[0][1] * (m[1][0] * A2323 - m[1][2] * A0323 + m[1][3] * A0223)
				+ m[0][2] * (m[1][0] * A1323 - m[1][1] * A0323 + m[1][3] * A0123)
				- m[0][3] * (m[1][0] * A1223 - m[1][1] * A0223 + m[1][2] * A0123);

			if (std::fabs(det) < 1e-8f) {
				return Mat4(0.0f);
			}

			float invDet = 1.0f / det;

			inv.m[0][0] = invDet * (m[1][1] * A2323 - m[1][2] * A1323 + m[1][3] * A1223);
			inv.m[0][1] = -invDet * (m[0][1] * A2323 - m[0][2] * A1323 + m[0][3] * A1223);
			inv.m[0][2] = invDet * (m[0][1] * A2313 - m[0][2] * A1313 + m[0][3] * A1213);
			inv.m[0][3] = -invDet * (m[0][1] * A2312 - m[0][2] * A1312 + m[0][3] * A1212);
			inv.m[1][0] = -invDet * (m[1][0] * A2323 - m[1][2] * A0323 + m[1][3] * A0223);
			inv.m[1][1] = invDet * (m[0][0] * A2323 - m[0][2] * A0323 + m[0][3] * A0223);
			inv.m[1][2] = -invDet * (m[0][0] * A2313 - m[0][2] * A0313 + m[0][3] * A0213);
			inv.m[1][3] = invDet * (m[0][0] * A2312 - m[0][2] * A0312 + m[0][3] * A0212);
			inv.m[2][0] = invDet * (m[1][0] * A1323 - m[1][1] * A0323 + m[1][3] * A0123);
			inv.m[2][1] = -invDet * (m[0][0] * A1323 - m[0][1] * A0323 + m[0][3] * A0123);
			inv.m[2][2] = invDet * (m[0][0] * A1313 - m[0][1] * A0313 + m[0][3] * A0113);
			inv.m[2][3] = -invDet * (m[0][0] * A1312 - m[0][1] * A0312 + m[0][3] * A0112);
			inv.m[3][0] = -invDet * (m[1][0] * A1223 - m[1][1] * A0223 + m[1][2] * A0123);
			inv.m[3][1] = invDet * (m[0][0] * A1223 - m[0][1] * A0223 + m[0][2] * A0123);
			inv.m[3][2] = -invDet * (m[0][0] * A1213 - m[0][1] * A0213 + m[0][2] * A0113);
			inv.m[3][3] = invDet * (m[0][0] * A1212 - m[0][1] * A0212 + m[0][2] * A0112);

			return inv;
		}


		// Shift 
		Mat4 shift(float x, float y, float z) const {
			Mat4 matrix;

			matrix.m[0][3] = x;
			matrix.m[1][3] = y;
			matrix.m[2][3] = z;
			return *this * matrix;
		}

		// Scale
		Mat4 scale(float x, float y, float z) const {
			Mat4 matrix;
			matrix.m[0][0] = x;
			matrix.m[1][1] = y;
			matrix.m[2][2] = z;
			return *this * matrix;
		}

		// Rotation (OX)
		Mat4 rotateX(float angle) const {
			Mat4 matrix;
			float c = std::cos(angle);
			float s = std::sin(angle);
			matrix.m[1][1] = c;
			matrix.m[1][2] = -s;
			matrix.m[2][1] = s;
			matrix.m[2][2] = c;
			return *this * matrix;
		}

		// Rotation (OY)
		Mat4 rotateY(float angle) const {
			Mat4 matrix;
			float c = std::cos(angle);
			float s = std::sin(angle);
			matrix.m[0][0] = c;
			matrix.m[0][2] = s;
			matrix.m[2][0] = -s;
			matrix.m[2][2] = c;
			return *this * matrix;
		}

		// Rotation (OZ)
		Mat4 rotateZ(float angle) const {
			Mat4 matrix;
			float c = std::cos(angle);
			float s = std::sin(angle);
			matrix.m[0][0] = c;
			matrix.m[0][1] = -s;
			matrix.m[1][0] = s;
			matrix.m[1][1] = c;
			return *this * matrix;
		}
	};
}