#pragma once

#include <map>
#include <memory>
#include <vector>
#include "objects/SceneObject.h"

class Scene {
public:
    std::map<uint32_t, std::shared_ptr<objects::SceneObject>> objects;
    uint32_t nextId = 1;
    int torusCounter = 1, pointCounter = 1;
    int bezierC0Counter = 1, bezierC2Counter = 1, interpC2Counter = 1;

    std::shared_ptr<objects::SceneObject> findById(uint32_t id) const;
    void deleteSelected();
    std::vector<std::shared_ptr<objects::SceneObject>> getSelected() const;
};
