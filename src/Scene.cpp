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

    // Remove Gregory fills that reference any removed control point (avoid dangling).
    for (auto& [id, obj] : objects) {
        if (obj->getType() != objects::ObjectType::GregoryFill || toRemove.count(id)) continue;
        if (auto* cps = obj->getControlPointsList())
            for (auto& wp : *cps)
                if (auto p = wp.lock(); p && toRemove.count(p->id)) { toRemove.insert(id); break; }
    }

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

bool Scene::collapsePoints(uint32_t idA, uint32_t idB) {
    if (idA == idB) return false;

    auto itA = objects.find(idA);
    auto itB = objects.find(idB);
    if (itA == objects.end() || itB == objects.end()) return false;

    auto ptA = std::dynamic_pointer_cast<objects::PointObject>(itA->second);
    auto ptB = std::dynamic_pointer_cast<objects::PointObject>(itB->second);
    if (!ptA || !ptB) return false;

    // Median position
    float ax = ptA->transform.m[0][3], ay = ptA->transform.m[1][3], az = ptA->transform.m[2][3];
    float bx = ptB->transform.m[0][3], by = ptB->transform.m[1][3], bz = ptB->transform.m[2][3];
    ptA->transform.m[0][3] = 0.5f * (ax + bx);
    ptA->transform.m[1][3] = 0.5f * (ay + by);
    ptA->transform.m[2][3] = 0.5f * (az + bz);
    ptA->needsUpdate = true;

    // Repoint references to idB onto ptA
    for (auto& [id, obj] : objects) {
        auto* cps = obj->getControlPointsList();
        if (!cps) continue;
        bool affected = false;
        for (auto& wp : *cps) {
            auto p = wp.lock();
            if (!p) continue;
            if (p->id == idB)      { wp = ptA; affected = true; }
            else if (p->id == idA) { affected = true; }
        }
        if (affected) obj->needsUpdate = true;
    }

    // Delete the second point
    objects.erase(idB);
    return true;
}
