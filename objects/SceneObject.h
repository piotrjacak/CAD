#include <string>
#include <vector>

namespace objects {
    enum class ObjectType { Torus, Point };

    struct SceneObject {
        std::string name;
        ObjectType type;
        bool isSelected = false;

		pmath::Mat4 initialTransform;
        pmath::Mat4 transform;
        
        // Torus parameters
        float R = 1.0f;
        float r = 0.3f;
        int meshAcc = 64;

        unsigned int VAO = 0, VBO = 0, EBO = 0;
        int indexCount = 0;
        bool needsUpdate = true;
    };
}
