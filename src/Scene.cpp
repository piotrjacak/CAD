#include "Scene.h"
#include "objects/CurveObject.h"

std::shared_ptr<objects::SceneObject> Scene::findById(uint32_t id) const {
    auto it = objects.find(id);
    return it != objects.end() ? it->second : nullptr;
}

void Scene::deleteSelected() {
    std::vector<uint32_t> toRemove;
    for (auto& [id, obj] : objects)
        if (obj->isSelected) toRemove.push_back(id);

    for (uint32_t removedId : toRemove) {
        for (auto& [id, obj] : objects) {
            if (auto* c = dynamic_cast<objects::CurveObject*>(obj.get()))
                c->removeControlPoint(removedId);
        }
        objects.erase(removedId);
    }
}

std::vector<std::shared_ptr<objects::SceneObject>> Scene::getSelected() const {
    std::vector<std::shared_ptr<objects::SceneObject>> result;
    for (auto& [id, obj] : objects)
        if (obj->isSelected) result.push_back(obj);
    return result;
}
