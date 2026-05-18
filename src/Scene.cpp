#include "Scene.h"
#include "objects/CurveObject.h"
#include "objects/SurfaceObject.h"
#include "objects/PointObject.h"
#include <set>

std::shared_ptr<objects::SceneObject> Scene::findById(uint32_t id) const {
    auto it = objects.find(id);
    return it != objects.end() ? it->second : nullptr;
}

void Scene::deleteSelected() {
    // 1. Kandydaci do usuniecia: wszystkie zaznaczone obiekty + punkty kontrolne
    //    zaznaczonych powierzchni.
    std::set<uint32_t> candidates;
    for (auto& [id, obj] : objects)
        if (obj->isSelected) candidates.insert(id);

    for (uint32_t id : std::set<uint32_t>(candidates)) {
        auto it = objects.find(id);
        if (it == objects.end()) continue;
        if (auto* surf = dynamic_cast<objects::SurfaceObject*>(it->second.get()))
            for (auto& wp : surf->controlPoints)
                if (auto pt = wp.lock())
                    candidates.insert(pt->id);
    }

    // 2. Filtrujemy: punkt z ownerSurfaceId != 0 mozemy usunac TYLKO wtedy,
    //    gdy jego owner tez jest na liscie do usuniecia.
    std::set<uint32_t> toRemove;
    for (uint32_t id : candidates) {
        auto it = objects.find(id);
        if (it == objects.end()) continue;
        if (auto pt = std::dynamic_pointer_cast<objects::PointObject>(it->second)) {
            if (pt->ownerSurfaceId != 0 && candidates.count(pt->ownerSurfaceId) == 0)
                continue;
        }
        toRemove.insert(id);
    }

    // 3. Czyscimy referencje weak_ptr w krzywych, a potem usuwamy obiekty.
    for (uint32_t removedId : toRemove) {
        for (auto& [id, obj] : objects)
            if (auto* c = dynamic_cast<objects::CurveObject*>(obj.get()))
                c->removeControlPoint(removedId);
        objects.erase(removedId);
    }
}

std::vector<std::shared_ptr<objects::SceneObject>> Scene::getSelected() const {
    std::vector<std::shared_ptr<objects::SceneObject>> result;
    for (auto& [id, obj] : objects)
        if (obj->isSelected) result.push_back(obj);
    return result;
}
