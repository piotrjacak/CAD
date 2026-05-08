#include "SceneObject.h"
#include <glad/glad.h>

namespace objects {

SceneObject::SceneObject(uint32_t id, std::string name)
    : id(id), name(std::move(name)) {}

SceneObject::~SceneObject() {
    freeGPU();
}

pmath::Vec3 SceneObject::getCenter() const {
    return pmath::Vec3(transform.m[0][3], transform.m[1][3], transform.m[2][3]);
}

void SceneObject::freeGPU() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        if (EBO != 0) glDeleteBuffers(1, &EBO);
        VAO = 0;
        VBO = 0;
        EBO = 0;
    }
}

} // namespace objects
