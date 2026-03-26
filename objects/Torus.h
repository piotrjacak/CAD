#pragma once

#include <cmath>
#include <vector>
#include "../pmath/Utils.h"

namespace objects {
	struct Torus
	{
		std::vector<float> vertices;
		std::vector<unsigned int> indices;

		Torus(float R, float r, int alphaAcc, int betaAcc)
		{
			for (int i = 0; i < alphaAcc; i++)
			{
				float alpha = i * (2.0f * pmath::PI / alphaAcc);

				for (int j = 0; j < betaAcc; j++)
				{
					float beta = j * (2.0f * pmath::PI / betaAcc);

					// Vertices
					float x = (R + r * std::cos(alpha)) * std::cos(beta);
					float y = (R + r * std::cos(alpha)) * std::sin(beta);
					float z = r * std::sin(alpha);

					vertices.push_back(x);
					vertices.push_back(y);
					vertices.push_back(z);

					// Indices
					int current = i * betaAcc + j;
					int nextAlpha = ((i + 1) % alphaAcc) * betaAcc + j;
					int nextBeta = i * betaAcc + ((j + 1) % betaAcc);
					int nextAlphaBeta = ((i + 1) % alphaAcc) * betaAcc + ((j + 1) % betaAcc);

					indices.push_back(current);
					indices.push_back(nextAlpha);
					indices.push_back(nextBeta);

					indices.push_back(nextAlpha);
					indices.push_back(nextAlphaBeta);
					indices.push_back(nextBeta);
				}
			}
		}
	};
}



