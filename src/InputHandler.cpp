#include "InputHandler.h"
#include "objects/CurveObject.h"
#include "objects/PointObject.h"
#include "objects/BezierCurveC2.h"
#include "imgui/imgui.h"
#include <set>

namespace {
// Flag every object (other than the one being moved) that references any of the given control points
void flagDependents(Scene& scene, const std::vector<std::weak_ptr<objects::PointObject>>& pts) {
    std::set<uint32_t> ids;
    for (auto& wp : pts)
        if (auto p = wp.lock()) ids.insert(p->id);
    for (auto& [id, obj] : scene.objects) {
        if (obj->needsUpdate) continue;
        if (auto* cps = obj->getControlPointsList())
            for (auto& wp : *cps)
                if (auto p = wp.lock(); p && ids.count(p->id)) { obj->needsUpdate = true; break; }
    }
}
} // namespace

void InputHandler::onScroll(double yoffset) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    SCREEN_SCALE += static_cast<float>(yoffset) * SCROLL_SPEED;
    if (SCREEN_SCALE > 5.0f)  SCREEN_SCALE = 5.0f;
    if (SCREEN_SCALE < 0.1f)  SCREEN_SCALE = 0.1f;
}

void InputHandler::onMouseButton(int button, int action, int mods,
                                  GLFWwindow* window, Scene& scene,
                                  const pmath::Vec3& cursorPos,
                                  const pmath::Vec3& medianPoint) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    bool alt_pressed   = (mods & GLFW_MOD_ALT);
    bool shift_pressed = (mods & GLFW_MOD_SHIFT);
    bool ctrl_pressed  = (mods & GLFW_MOD_CONTROL);
    bool s_pressed = (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);
    bool r_pressed = (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS);
    bool t_pressed = (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            if (alt_pressed) {
                CAM_ROTATE_ACTIVE = true;
                glfwGetCursorPos(window, &LAST_MOUSE_X, &LAST_MOUSE_Y);
            }
            else if (s_pressed || r_pressed || t_pressed) {
                if (s_pressed)      OBJ_SCALE_ACTIVE     = true;
                else if (r_pressed) OBJ_ROTATE_ACTIVE    = true;
                else if (t_pressed) OBJ_TRANSLATE_ACTIVE = true;
                glfwGetCursorPos(window, &START_MOUSE_X, &START_MOUSE_Y);

                for (auto& [id, obj] : scene.objects) {
                    if (obj->isSelected) {
                        obj->initialTransform = obj->transform;
                        if (auto* cps = obj->getControlPointsList())
                            for (auto& wp : *cps)
                                if (auto pt = wp.lock())
                                    pt->initialTransform = pt->transform;
                    }
                }

                if      (CURRENT_PIVOT == PivotType::Median) pivotPoint = medianPoint;
                else if (CURRENT_PIVOT == PivotType::Cursor) pivotPoint = cursorPos;
            }
            else {
                PROCESS_SELECTION = true;
                MULTI_SELECT = ctrl_pressed;
                glfwGetCursorPos(window, &SELECT_MOUSE_X, &SELECT_MOUSE_Y);
                LMB_PRESSED = true;
            }
        }
        else if (action == GLFW_RELEASE) {
            CAM_ROTATE_ACTIVE     = false;
            LMB_PRESSED           = false;
            OBJ_SCALE_ACTIVE      = false;
            OBJ_ROTATE_ACTIVE     = false;
            OBJ_TRANSLATE_ACTIVE  = false;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            if (alt_pressed) {
                CAM_PAN_ACTIVE = true;
                glfwGetCursorPos(window, &LAST_MOUSE_X, &LAST_MOUSE_Y);
            }
            else if (shift_pressed) {
                SET_CURSOR_FLAG = true;
                glfwGetCursorPos(window, &TARGET_CURSOR_X, &TARGET_CURSOR_Y);
            }
            else {
                RMB_PRESSED = true;
            }
        }
        else if (action == GLFW_RELEASE) {
            CAM_PAN_ACTIVE = false;
            RMB_PRESSED    = false;
        }
    }
}

void InputHandler::onCursorPos(double xpos, double ypos, Scene& scene) {
    double objectDeltaX = xpos - START_MOUSE_X;
    double objectDeltaY = ypos - START_MOUSE_Y;
    double cameraDeltaX = xpos - LAST_MOUSE_X;
    double cameraDeltaY = ypos - LAST_MOUSE_Y;

    if (CAM_ROTATE_ACTIVE) {
        ROT_Y += static_cast<float>(-cameraDeltaX) * ROTATE_SPEED;
        ROT_X += static_cast<float>(-cameraDeltaY) * ROTATE_SPEED;
        LAST_MOUSE_X = xpos;
        LAST_MOUSE_Y = ypos;
    }
    else if (CAM_PAN_ACTIVE) {
        SHIFT_X += static_cast<float>(cameraDeltaX) * PAN_SPEED;
        SHIFT_Y -= static_cast<float>(cameraDeltaY) * PAN_SPEED;
        LAST_MOUSE_X = xpos;
        LAST_MOUSE_Y = ypos;
    }
    else if (OBJ_SCALE_ACTIVE) {
        float scaleDelta = static_cast<float>(objectDeltaX - objectDeltaY) * 0.005f;
        float factor = 1.0f + scaleDelta;
        if (factor <= 0.01f) factor = 0.01f;

        for (auto& [id, obj] : scene.objects) {
            if (!obj->isSelected) continue;
            if (obj->getType() == objects::ObjectType::GregoryFill) continue; // not transformable
            pmath::Vec3 pivot;
            if (CURRENT_PIVOT == PivotType::Local)
                pivot = pmath::Vec3(obj->initialTransform.m[0][3], obj->initialTransform.m[1][3], obj->initialTransform.m[2][3]);
            else pivot = pivotPoint;

            pmath::Mat4 delta;
            delta.shift(-pivot.x, -pivot.y, -pivot.z);
            delta.scale(factor, factor, factor);
            delta.shift(pivot.x, pivot.y, pivot.z);

            if (auto* cps = obj->getControlPointsList()) {
                for (auto& wp : *cps)
                    if (auto pt = wp.lock())
                        pt->transform = delta * pt->initialTransform;
                obj->needsUpdate = true;
                flagDependents(scene, *cps);
            }
            else if (obj->getType() == objects::ObjectType::Point) {
                for (auto& [cid, c] : scene.objects)
                    if (auto* otherCps = c->getControlPointsList())
                        for (auto& wp : *otherCps)
                            if (auto pt = wp.lock(); pt && pt->id == obj->id)
                                { c->needsUpdate = true; break; }
                obj->transform = delta * obj->initialTransform;
            }
            else {
                obj->transform = delta * obj->initialTransform;
            }
        }
    }
    else if (OBJ_ROTATE_ACTIVE) {
        float rotAngleY    = static_cast<float>(-objectDeltaX) * ROTATE_SPEED;
        float rotAngleX    = static_cast<float>(-objectDeltaY) * ROTATE_SPEED;
        float combinedAngle = static_cast<float>(objectDeltaY - objectDeltaX) * ROTATE_SPEED;

        for (auto& [id, obj] : scene.objects) {
            if (!obj->isSelected) continue;
            if (obj->getType() == objects::ObjectType::GregoryFill) continue; // not transformable
            pmath::Vec3 pivot;
            if (CURRENT_PIVOT == PivotType::Local)
                pivot = pmath::Vec3(obj->initialTransform.m[0][3], obj->initialTransform.m[1][3], obj->initialTransform.m[2][3]);
            else pivot = pivotPoint;

            pmath::Mat4 delta;
            delta.shift(-pivot.x, -pivot.y, -pivot.z);
            if      (CURRENT_ROT_AXIS == RotationAxis::Free) { delta.rotateX(rotAngleX); delta.rotateY(rotAngleY); }
            else if (CURRENT_ROT_AXIS == RotationAxis::X)    delta.rotateX(combinedAngle);
            else if (CURRENT_ROT_AXIS == RotationAxis::Y)    delta.rotateY(combinedAngle);
            else if (CURRENT_ROT_AXIS == RotationAxis::Z)    delta.rotateZ(combinedAngle);
            delta.shift(pivot.x, pivot.y, pivot.z);

            if (auto* cps = obj->getControlPointsList()) {
                for (auto& wp : *cps)
                    if (auto pt = wp.lock())
                        pt->transform = delta * pt->initialTransform;
                obj->needsUpdate = true;
                flagDependents(scene, *cps);
            }
            else if (obj->getType() == objects::ObjectType::Point) {
                for (auto& [cid, c] : scene.objects)
                    if (auto* otherCps = c->getControlPointsList())
                        for (auto& wp : *otherCps)
                            if (auto pt = wp.lock(); pt && pt->id == obj->id)
                                { c->needsUpdate = true; break; }
                obj->transform = delta * obj->initialTransform;
            }
            else {
                obj->transform = delta * obj->initialTransform;
            }
        }
    }
    else if (OBJ_TRANSLATE_ACTIVE) {
        float moveX = static_cast<float>(objectDeltaX) * PAN_SPEED;
        float moveY = static_cast<float>(-objectDeltaY) * PAN_SPEED;

        pmath::Mat4 invSceneModel;
        invSceneModel.scale(SCREEN_SCALE, SCREEN_SCALE, SCREEN_SCALE);
        invSceneModel.rotateX(ROT_X);
        invSceneModel.rotateY(ROT_Y);
        invSceneModel.inverse();

        pmath::Vec4 worldMoveDir = invSceneModel * pmath::Vec4(moveX, moveY, 0.0f, 0.0f);

        for (auto& [id, obj] : scene.objects) {
            if (!obj->isSelected) continue;
            if (obj->getType() == objects::ObjectType::GregoryFill) continue; // not transformable

            auto* c2 = dynamic_cast<objects::BezierCurveC2*>(obj.get());
            if (c2 && !c2->isBsplineBasis && c2->selectedVirtualPointIndex != -1) {
                int j   = c2->selectedVirtualPointIndex;
                int seg = j / 4;
                int p   = j % 4;
                int dIndex = (p == 0 || p == 1) ? seg + 1 : seg + 2;

                if (dIndex >= 0 && dIndex < (int)c2->controlPoints.size()) {
                    if (auto pt = c2->controlPoints[dIndex].lock()) {
                        pmath::Mat4 delta;
                        delta.shift(worldMoveDir.x * 1.5f, worldMoveDir.y * 1.5f, worldMoveDir.z * 1.5f);
                        pt->transform = delta * pt->initialTransform;
                    }
                }
                obj->needsUpdate = true;
                continue;
            }

            pmath::Mat4 delta;
            delta.shift(worldMoveDir.x, worldMoveDir.y, worldMoveDir.z);

            if (auto* cps = obj->getControlPointsList()) {
                for (auto& wp : *cps)
                    if (auto pt = wp.lock(); pt && !pt->isSelected)
                        pt->transform = delta * pt->initialTransform;
                obj->needsUpdate = true;
                flagDependents(scene, *cps);
            }
            else if (obj->getType() == objects::ObjectType::Point) {
                for (auto& [cid, c] : scene.objects)
                    if (auto* otherCps = c->getControlPointsList())
                        for (auto& wp : *otherCps)
                            if (auto pt = wp.lock(); pt && pt->id == obj->id)
                                { c->needsUpdate = true; break; }
                obj->transform = delta * obj->initialTransform;
            }
            else {
                obj->transform = delta * obj->initialTransform;
            }
        }
    }
}

void InputHandler::processSelection(Scene& scene,
                                     const pmath::Mat4& projection,
                                     const pmath::Mat4& view,
                                     const pmath::Mat4& sceneModel,
                                     int displayW, int displayH) {
    PROCESS_SELECTION = false;

    float selNdcX = (float)(SELECT_MOUSE_X / displayW) * 2.0f - 1.0f;
    float selNdcY = 1.0f - (float)(SELECT_MOUSE_Y / displayH) * 2.0f;

    pmath::Mat4 invVP = projection * view * sceneModel;
    invVP.inverse();

    pmath::Vec4 nearPoint = invVP * pmath::Vec4(selNdcX, selNdcY, -1.0f, 1.0f);
    pmath::Vec4 farPoint  = invVP * pmath::Vec4(selNdcX, selNdcY,  1.0f, 1.0f);

    pmath::Vec3 rayOrigin(nearPoint.x / nearPoint.w, nearPoint.y / nearPoint.w, nearPoint.z / nearPoint.w);
    pmath::Vec3 rayEnd(farPoint.x / farPoint.w, farPoint.y / farPoint.w, farPoint.z / farPoint.w);
    pmath::Vec3 rayDir = (rayEnd - rayOrigin).normalize();

    float minDistanceToCamera = 999999.0f;

    bool hitVirtualPoint = false;
    for (auto& [id, obj] : scene.objects) {
        auto* c2 = dynamic_cast<objects::BezierCurveC2*>(obj.get());
        if (c2) c2->selectedVirtualPointIndex = -1;

        if (obj->isSelected && c2 && !c2->isBsplineBasis) {
            float minHitDistanceToRay = 999999.0f;
            for (int j = 0; j < (int)c2->virtualPoints.size(); ++j) {
                pmath::Vec3 worldPt = c2->virtualPoints[j];
                pmath::Vec3 v = worldPt - rayOrigin;
                float t = v.dot(rayDir);
                if (t > 0.0f) {
                    pmath::Vec3 closest = rayOrigin + (rayDir * t);
                    float dist = (worldPt - closest).length();
                    if (dist < 0.15f && dist < minHitDistanceToRay) {
                        minHitDistanceToRay   = dist;
                        minDistanceToCamera   = t;
                        c2->selectedVirtualPointIndex = j;
                        hitVirtualPoint = true;
                    }
                }
            }
        }
    }

    if (!hitVirtualPoint) {
        std::shared_ptr<objects::SceneObject> hitObject;
        for (auto& [id, obj] : scene.objects) {
            pmath::Vec3 objPos = obj->getCenter();
            pmath::Vec3 v = objPos - rayOrigin;
            float t = v.dot(rayDir);
            if (t > 0.0f) {
                pmath::Vec3 closest = rayOrigin + (rayDir * t);
                float dist = (objPos - closest).length();
                if (dist < obj->getPickRadius() && t < minDistanceToCamera) {
                    minDistanceToCamera = t;
                    hitObject = obj;
                }
            }
        }
        if (hitObject) {
            if (!MULTI_SELECT) {
                for (auto& [id, obj] : scene.objects) obj->isSelected = false;
                hitObject->isSelected = true;
            } else {
                hitObject->isSelected = !hitObject->isSelected;
            }
        } else {
            if (!MULTI_SELECT)
                for (auto& [id, obj] : scene.objects) obj->isSelected = false;
        }
    }
}
