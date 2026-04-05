#include <string>
#include <vector>

namespace objects {
    enum class ObjectType { Torus, Point, BezierCurveC0, BezierCurveC2 };

    enum class CurveBasis { Bernstein, BSpline };

    struct VirtualPoint {
		pmath::Mat4 transform;
		pmath::Mat4 initialTransform;
		bool isSelected = false;
    };

    struct SceneObject {
		uint32_t id;
        std::string name;
        ObjectType type;
        bool isSelected = false;

		pmath::Mat4 initialTransform;
        pmath::Mat4 transform;

        unsigned int VAO = 0, VBO = 0, EBO = 0;
        int indexCount = 0;
        bool needsUpdate = true;

        // Torus parameters
        float R = 1.0f;
        float r = 0.3f;
        int meshAcc = 64;

		// Bezier curve parameters
		std::vector<uint32_t> controlPointIDs;
        bool showPolyline = true;
    };
}
