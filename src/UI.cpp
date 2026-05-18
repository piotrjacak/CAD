#include "UI.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "objects/CurveObject.h"
#include "objects/PointObject.h"
#include "objects/TorusObject.h"
#include "objects/BezierCurveC0.h"
#include "objects/BezierCurveC2.h"
#include "objects/InterpolatingCurveC2.h"
#include "objects/BezierSurfaceC0.h"
#include <memory>
#include <string>

UIResult UI::render(Scene& scene,
                    float& cursorX, float& cursorY, float& cursorZ,
                    float& screenCursorX, float& screenCursorY,
                    PivotType& pivot, RotationAxis& rotAxis,
                    float default_R, float default_r, int default_meshAcc,
                    int displayW, int displayH,
                    StereoParams& stereo)
{
    UIResult result;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    float panel_width = 600.0f;
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panel_width, (float)displayH), ImGuiCond_Always);
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

    objects::TorusObject*             selectedTorus     = nullptr;
    objects::BezierCurveC0*           selectedBezierC0  = nullptr;
    objects::BezierCurveC2*           selectedBezierC2  = nullptr;
    objects::InterpolatingCurveC2*    selectedInterpC2  = nullptr;
    objects::BezierSurfaceC0*         selectedSurfaceC0 = nullptr;
    float currentR = default_R, current_r = default_r;
    int currentMeshAcc = default_meshAcc;

    for (auto& [id, obj] : scene.objects) {
        if (!obj->isSelected) continue;
        if (!selectedTorus)     selectedTorus     = dynamic_cast<objects::TorusObject*>(obj.get());
        if (!selectedBezierC0)  selectedBezierC0  = dynamic_cast<objects::BezierCurveC0*>(obj.get());
        if (!selectedBezierC2)  selectedBezierC2  = dynamic_cast<objects::BezierCurveC2*>(obj.get());
        if (!selectedInterpC2)  selectedInterpC2  = dynamic_cast<objects::InterpolatingCurveC2*>(obj.get());
        if (!selectedSurfaceC0) selectedSurfaceC0 = dynamic_cast<objects::BezierSurfaceC0*>(obj.get());
    }
    if (selectedTorus) {
        currentR = selectedTorus->R;
        current_r = selectedTorus->r;
        currentMeshAcc = selectedTorus->meshAcc;
    }

    ImGui::Begin("Options", nullptr, window_flags);

    // Torus parameters
    if (selectedTorus) {
        ImGui::Text("Selected Torus Parameters");
        bool paramsChanged = false;
        if (ImGui::SliderFloat("R", &currentR, 0.3f, 2.0f)) paramsChanged = true;
        if (ImGui::SliderFloat("r", &current_r, 0.1f, 1.0f)) paramsChanged = true;
        if (ImGui::SliderInt("m", &currentMeshAcc, 6, 128)) paramsChanged = true;
        if (paramsChanged) {
            selectedTorus->R = currentR;
            selectedTorus->r = current_r;
            selectedTorus->meshAcc = currentMeshAcc;
            selectedTorus->needsUpdate = true;
        }
    }
    else ImGui::TextDisabled("Select a Torus to edit parameters.");

    // Bezier C0
    ImGui::Separator();
    if (selectedBezierC0) {
        ImGui::Text("Selected Bezier Curve C0 Options");
        if (ImGui::Checkbox("Show polyline", &selectedBezierC0->showPolyline)) {}
        if (ImGui::Button("Add selected point to curve")) {
            for (auto& [id, obj] : scene.objects)
                if (obj->isSelected && obj->getType() == objects::ObjectType::Point)
                    if (auto pt = std::dynamic_pointer_cast<objects::PointObject>(obj))
                        selectedBezierC0->addControlPoint(pt);
        }
        ImGui::Text("Control Points:");
        ImGui::BeginChild("BezierControlPoints", ImVec2(0, 150), true);
        for (size_t i = 0; i < selectedBezierC0->controlPoints.size(); ++i) {
            auto pt = selectedBezierC0->controlPoints[i].lock();
            if (!pt) continue;
            ImGui::PushID(static_cast<int>(pt->id));
            ImGui::Text("%s (ID: %u)", pt->name.c_str(), pt->id);
            ImGui::SameLine(ImGui::GetWindowWidth() - 70);
            if (ImGui::Button("Remove")) {
                selectedBezierC0->removeControlPoint(pt->id);
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
    }
    else ImGui::TextDisabled("Select a Bezier Curve C0 to edit options.");

    // Bezier C2
    ImGui::Separator();
    if (selectedBezierC2) {
        ImGui::Text("Selected Bezier Curve C2 Options");
        if (ImGui::Checkbox("Show polyline", &selectedBezierC2->showPolyline)) {}
        int basis = selectedBezierC2->isBsplineBasis ? 0 : 1;
        ImGui::Text("Curve Basis:");
        if (ImGui::RadioButton("B-spline", &basis, 0))  { selectedBezierC2->isBsplineBasis = true;  selectedBezierC2->needsUpdate = true; }
        ImGui::SameLine();
        if (ImGui::RadioButton("Bernstein", &basis, 1)) { selectedBezierC2->isBsplineBasis = false; selectedBezierC2->needsUpdate = true; }
        if (ImGui::Button("Add selected point to curve##C2")) {
            for (auto& [id, obj] : scene.objects)
                if (obj->isSelected && obj->getType() == objects::ObjectType::Point)
                    if (auto pt = std::dynamic_pointer_cast<objects::PointObject>(obj))
                        selectedBezierC2->addControlPoint(pt);
        }
        ImGui::Text("Control Points:");
        ImGui::BeginChild("BezierC2ControlPoints", ImVec2(0, 150), true);
        for (size_t i = 0; i < selectedBezierC2->controlPoints.size(); ++i) {
            auto pt = selectedBezierC2->controlPoints[i].lock();
            if (!pt) continue;
            ImGui::PushID(static_cast<int>(pt->id));
            ImGui::Text("%s (ID: %u)", pt->name.c_str(), pt->id);
            ImGui::SameLine(ImGui::GetWindowWidth() - 70);
            if (ImGui::Button("Remove")) {
                selectedBezierC2->removeControlPoint(pt->id);
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
    }
    else ImGui::TextDisabled("Select a Bezier Curve C2 to edit options.");

    // Interpolating C2
    ImGui::Separator();
    if (selectedInterpC2) {
        ImGui::Text("Selected Interpolating Curve C2 Options");
        if (ImGui::Checkbox("Show polyline", &selectedInterpC2->showPolyline)) {}
        if (ImGui::Button("Add selected point to curve##IC2")) {
            for (auto& [id, obj] : scene.objects)
                if (obj->isSelected && obj->getType() == objects::ObjectType::Point)
                    if (auto pt = std::dynamic_pointer_cast<objects::PointObject>(obj))
                        selectedInterpC2->addControlPoint(pt);
        }
        ImGui::Text("Interpolation Points:");
        ImGui::BeginChild("InterpC2Points", ImVec2(0, 150), true);
        for (size_t i = 0; i < selectedInterpC2->controlPoints.size(); ++i) {
            auto pt = selectedInterpC2->controlPoints[i].lock();
            if (!pt) continue;
            ImGui::PushID(static_cast<int>(pt->id));
            ImGui::Text("%s (ID: %u)", pt->name.c_str(), pt->id);
            ImGui::SameLine(ImGui::GetWindowWidth() - 70);
            if (ImGui::Button("Remove")) {
                selectedInterpC2->removeControlPoint(pt->id);
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
    }
    else ImGui::TextDisabled("Select an Interpolating Curve C2 to edit options.");

    // Bezier Surface C0
    ImGui::Separator();
    if (selectedSurfaceC0) {
        ImGui::Text("Selected Bezier Surface C0 Options");
        const char* topoName =
            (selectedSurfaceC0->topology == objects::SurfaceObject::Topology::Plane)
                ? "Plane" : "Cylinder";
        ImGui::Text("Topology: %s",  topoName);
        ImGui::Text("Patches U: %d", selectedSurfaceC0->patchesU);
        ImGui::Text("Patches V: %d", selectedSurfaceC0->patchesV);
        ImGui::SliderInt("Tessellation",     &selectedSurfaceC0->tessLevel, 1, 64);
        ImGui::Checkbox ("Show control mesh", &selectedSurfaceC0->showControlMesh);
    }
    else ImGui::TextDisabled("Select a Bezier Surface C0 to edit options.");

    // Cursor position
    ImGui::Separator();
    ImGui::Text("Cursor position");
    ImGui::DragFloat("Scene X", &cursorX, 0.01f);
    ImGui::DragFloat("Scene Y", &cursorY, 0.01f);
    ImGui::DragFloat("Scene Z", &cursorZ, 0.01f);
    if (ImGui::DragFloat("Screen X", &screenCursorX, 1.0f)) result.screenCursorChanged = true;
    if (ImGui::DragFloat("Screen Y", &screenCursorY, 1.0f)) result.screenCursorChanged = true;

    // Add objects
    ImGui::Separator();
    ImGui::Text("Add objects");
    if (ImGui::Button("Add Torus")) {
        auto obj = std::make_shared<objects::TorusObject>(
            scene.nextId++,
            "Torus " + std::to_string(scene.torusCounter++),
            default_R, default_r, default_meshAcc);
        obj->transform.shift(cursorX, cursorY, cursorZ);
        scene.objects[obj->id] = obj;
    }
    ImGui::SameLine();
    if (ImGui::Button("Add point")) {
        auto obj = std::make_shared<objects::PointObject>(
            scene.nextId++,
            "Point " + std::to_string(scene.pointCounter++));
        obj->transform.shift(cursorX, cursorY, cursorZ);
        for (auto& [id, curve] : scene.objects)
            if (curve->isSelected)
                if (auto* c = dynamic_cast<objects::CurveObject*>(curve.get()))
                    c->addControlPoint(obj);
        scene.objects[obj->id] = obj;
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Bezier Curve C0")) {
        auto obj = std::make_shared<objects::BezierCurveC0>(
            scene.nextId++,
            "Bezier C0 " + std::to_string(scene.bezierC0Counter++));
        for (auto& [id, other] : scene.objects)
            if (other->isSelected && other->getType() == objects::ObjectType::Point)
                if (auto pt = std::dynamic_pointer_cast<objects::PointObject>(other))
                    obj->addControlPoint(pt);
        scene.objects[obj->id] = obj;
    }
    if (ImGui::Button("Add Bezier Curve C2")) {
        auto obj = std::make_shared<objects::BezierCurveC2>(
            scene.nextId++,
            "Bezier C2 " + std::to_string(scene.bezierC2Counter++));
        for (auto& [id, other] : scene.objects)
            if (other->isSelected && other->getType() == objects::ObjectType::Point)
                if (auto pt = std::dynamic_pointer_cast<objects::PointObject>(other))
                    obj->addControlPoint(pt);
        scene.objects[obj->id] = obj;
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Interpolating C2")) {
        auto obj = std::make_shared<objects::InterpolatingCurveC2>(
            scene.nextId++,
            "Interp C2 " + std::to_string(scene.interpC2Counter++));
        for (auto& [id, other] : scene.objects)
            if (other->isSelected && other->getType() == objects::ObjectType::Point)
                if (auto pt = std::dynamic_pointer_cast<objects::PointObject>(other))
                    obj->addControlPoint(pt);
        scene.objects[obj->id] = obj;
    }
    if (ImGui::Button("Add Bezier Surface C0")) {
        ImGui::OpenPopup("New Bezier Surface C0");
    }
    if (ImGui::BeginPopupModal("New Bezier Surface C0", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::RadioButton("Plane",    &newSurfaceC0_topology, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Cylinder", &newSurfaceC0_topology, 1);
        ImGui::Separator();

        ImGui::InputInt("Patches U", &newSurfaceC0_patchesU);
        if (newSurfaceC0_patchesU < 1) newSurfaceC0_patchesU = 1;
        ImGui::InputInt("Patches V", &newSurfaceC0_patchesV);
        if (newSurfaceC0_patchesV < 1) newSurfaceC0_patchesV = 1;

        if (newSurfaceC0_topology == 0) {
            ImGui::InputFloat("Width",  &newSurfaceC0_width);
            ImGui::InputFloat("Height", &newSurfaceC0_height);
        } else {
            ImGui::InputFloat("Radius", &newSurfaceC0_radius);
            ImGui::InputFloat("Height", &newSurfaceC0_cylHeight);
        }

        ImGui::Separator();
        if (ImGui::Button("Accept")) {
            if (newSurfaceC0_topology == 0) {
                objects::BezierSurfaceC0::createPlane(
                    scene,
                    newSurfaceC0_patchesU, newSurfaceC0_patchesV,
                    newSurfaceC0_width,    newSurfaceC0_height,
                    pmath::Vec3(cursorX, cursorY, cursorZ));
            } else {
                objects::BezierSurfaceC0::createCylinder(
                    scene,
                    newSurfaceC0_patchesU, newSurfaceC0_patchesV,
                    newSurfaceC0_radius,   newSurfaceC0_cylHeight,
                    pmath::Vec3(cursorX, cursorY, cursorZ));
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Object list
    ImGui::Separator();
    ImGui::Text("List of scene objects");
    if (ImGui::Button("Delete selected"))
        scene.deleteSelected();
    ImGui::BeginChild("ObjectList", ImVec2(0, 200), true);
    for (auto& [id, obj] : scene.objects) {
        ImGui::PushID(static_cast<int>(id));
        ImGui::Checkbox("##sel", &obj->isSelected);
        ImGui::SameLine();
        char nameBuf[64];
        strncpy_s(nameBuf, obj->name.c_str(), sizeof(nameBuf));
        nameBuf[sizeof(nameBuf) - 1] = '\0';
        ImGui::InputText("##Name", nameBuf, sizeof(nameBuf));
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            std::string newName(nameBuf);
            bool nameExists = false;
            for (auto& [otherId, otherObj] : scene.objects)
                if (otherId != id && otherObj->name == newName) { nameExists = true; break; }
            if (!nameExists && !newName.empty())
                obj->name = newName;
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    // Transform settings
    ImGui::Separator();
    ImGui::Text("Transform Pivot Point");
    int pivotInt = (int)pivot;
    ImGui::RadioButton("Local", &pivotInt, 0); ImGui::SameLine();
    ImGui::RadioButton("Median", &pivotInt, 1); ImGui::SameLine();
    ImGui::RadioButton("Cursor", &pivotInt, 2);
    pivot = (PivotType)pivotInt;

    ImGui::Text("Objects Rotation Axis");
    int rotAxisInt = (int)rotAxis;
    ImGui::RadioButton("Free (XY)", &rotAxisInt, 0); ImGui::SameLine();
    ImGui::RadioButton("X", &rotAxisInt, 1); ImGui::SameLine();
    ImGui::RadioButton("Y", &rotAxisInt, 2); ImGui::SameLine();
    ImGui::RadioButton("Z", &rotAxisInt, 3);
    rotAxis = (RotationAxis)rotAxisInt;

    // Stereo
    ImGui::Separator();
    ImGui::Text("Stereoscopic Rendering");
    ImGui::Checkbox("Enable Anaglyph", &stereo.enabled);
    if (stereo.enabled) {
        ImGui::SliderFloat("Eye Separation",      &stereo.eyeSep,      0.001f, 0.5f);
        ImGui::SliderFloat("Convergence Distance", &stereo.convergence, 0.5f,  20.0f);
    }

    ImGui::End();
    ImGui::Render();

    return result;
}
