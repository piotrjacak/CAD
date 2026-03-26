#include <string>
#include <vector>

namespace objects {
    enum class ObjectType { Torus, Point };

    struct SceneObject {
        std::string name;
        ObjectType type;
        float posX, posY, posZ;
        float scale = 1.0f;
        float rotX = 0.0f;
        float rotY = 0.0f;
        float rotZ = 0.0f;
        bool isSelected = false;
    };
}
