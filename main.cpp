#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui.h"

#include "pmath/pmath.h"
#include "matrices.h"
#include "shader.h"
#include "objects/Torus.h"
#include "objects/SceneObject.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

enum class PivotType { Local, Median, Cursor };
enum class RotationAxis { Free, X, Y, Z };

// GLOBAL VARIABLES
const unsigned int SCR_WIDTH = 1800;
const unsigned int SCR_HEIGHT = 1200;
//const unsigned int SCR_WIDTH = 1600;
//const unsigned int SCR_HEIGHT = 1000;

// Speed variables
float SCROLL_SPEED = 0.1f;
float SCREEN_SCALE = 1.0f;
float PAN_SPEED = 0.0025f;
float ROTATE_SPEED = 0.005f;

// Scene variables
float ROT_X = 0.0f;
float ROT_Y = 0.0f;
float SHIFT_X = 0.0f;
float SHIFT_Y = 0.0f;

// Control variables
bool CAM_ROTATE_ACTIVE = false;     // Alt + LMB
bool CAM_PAN_ACTIVE = false;        // Alt + RMB
bool LMB_PRESSED = false;           // LMB
bool RMB_PRESSED = false;           // RMB
bool SET_CURSOR_FLAG = false;       // Shift + RMB
bool PROCESS_SELECTION = false; 
bool MULTI_SELECT = false;          // CTRL
bool OBJ_SCALE_ACTIVE = false;          // S + LPM
bool OBJ_ROTATE_ACTIVE = false;     // R + LPM
bool OBJ_TRANSLATE_ACTIVE = false;  // T + LPM

// Position variables
double TARGET_CURSOR_X = 0.0;
double TARGET_CURSOR_Y = 0.0;
double LAST_MOUSE_X = 0.0;
double LAST_MOUSE_Y = 0.0;
double START_MOUSE_X = 0.0;
double START_MOUSE_Y = 0.0;
double SELECT_MOUSE_X = 0.0;
double SELECT_MOUSE_Y = 0.0;

PivotType CURRENT_PIVOT = PivotType::Local;
RotationAxis CURRENT_ROT_AXIS = RotationAxis::Free;


float cursorX = 0.0f, cursorY = 0.0f, cursorZ = 0.0f;
pmath::Vec3 medianPoint(0.0f, 0.0f, 0.0f);
pmath::Vec3 pivotPoint(0.0f, 0.0f, 0.0f);

std::vector<objects::SceneObject> sceneObjects;
int torusCounter = 1;
int pointCounter = 1;

std::vector<float> cursorVertices = {
    0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.9f, 0.1f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.9f, -0.1f, 0.0f, 1.0f, 0.0f, 0.0f,

    0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    -0.1f, 0.9f, 0.0f, 0.0f, 1.0f, 0.0f,
     0.1f, 0.9f, 0.0f, 0.0f, 1.0f, 0.0f,

    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    -0.1f, 0.0f, 0.9f, 0.0f, 0.0f, 1.0f,
    0.1f, 0.0f, 0.9f, 0.0f, 0.0f, 1.0f
};
std::vector<unsigned int> cursorIndices = {
    0, 1, 1, 2, 1, 3, 4, 5, 5, 6, 5, 7, 8, 9, 9, 10, 9, 11
};

float default_R = 1.0f;
float default_r = 0.3f;
int default_meshAcc = 64;


int main()
{
	// -- BOILERPLATE CODE --
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "CAD", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glEnable(GL_DEPTH_TEST);
    // IMGUI SETUP
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
    // High resolution screen setting
    float scale_factor = 1.5f;
    ImGui::GetStyle().ScaleAllSizes(scale_factor);
    io.FontGlobalScale = scale_factor;
	// -- END OF BOILERPLATE CODE --


    // CREATE CURSOR
	float screenCursorX = 0.0f, screenCursorY = 0.0f;
	unsigned int cursorVAO, cursorVBO, cursorEBO;
	glGenVertexArrays(1, &cursorVAO);
	glGenBuffers(1, &cursorVBO);
	glGenBuffers(1, &cursorEBO);

	glBindVertexArray(cursorVAO);
    
    // Vertices to VBO
	glBindBuffer(GL_ARRAY_BUFFER, cursorVBO);
	glBufferData(GL_ARRAY_BUFFER, cursorVertices.size() * sizeof(float), cursorVertices.data(), GL_STATIC_DRAW);
    // Indices to EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cursorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cursorIndices.size() * sizeof(unsigned int), cursorIndices.data(), GL_STATIC_DRAW);
    // Configure vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
   

    // CREATE POINT
    unsigned int pointVAO, pointVBO;
    glGenVertexArrays(1, &pointVAO);
    glGenBuffers(1, &pointVBO);
    glBindVertexArray(pointVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
    float pointData[] = { 0.0f, 0.0f, 0.0f };
    glBufferData(GL_ARRAY_BUFFER, sizeof(pointData), pointData, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glEnable(GL_PROGRAM_POINT_SIZE);


    // INITIALIZE SHADERS
    Shader shaderProgram("shader.vs", "shader.fs");
	Shader cursorShaderProgram("cursorShader.vs", "cursorShader.fs");


    // RENDER LOOP
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        // -- IMGUI SETUP -- 
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // Set up position and flags for the control panel
        float panel_width = 500.0f;
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panel_width, (float)display_h), ImGuiCond_Always);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove;

        bool isAnyTorusSelected = false;
        float currentR = default_R, current_r = default_r;
        int currentMeshAcc = default_meshAcc;
        for (auto& obj : sceneObjects)
        {
            if (obj.isSelected && obj.type == objects::ObjectType::Torus)
            {
                isAnyTorusSelected = true;
                currentR = obj.R;
                current_r = obj.r;
                currentMeshAcc = obj.meshAcc;
                break;
            }
        }
        ImGui::Begin("Options", nullptr, window_flags);
        if (isAnyTorusSelected) 
        {
            ImGui::Text("Selected Torus Parameters");
            bool paramsChanged = false;
            if (ImGui::SliderFloat("R", &currentR, 0.3f, 2.0f)) paramsChanged = true;
            if (ImGui::SliderFloat("r", &current_r, 0.1f, 1.0f)) paramsChanged = true;
            if (ImGui::SliderInt("m", &currentMeshAcc, 6, 128)) paramsChanged = true;
            if (paramsChanged) {
                for (auto& obj : sceneObjects) {
                    if (obj.isSelected && obj.type == objects::ObjectType::Torus) {
                        obj.R = currentR;
                        obj.r = current_r;
                        obj.meshAcc = currentMeshAcc;
                        obj.needsUpdate = true;
                    }
                }
            }
        }
        else ImGui::TextDisabled("Select a Torus to edit parameters.");

		ImGui::Separator();
		ImGui::Text("Cursor position");
		ImGui::DragFloat("Scene X", &cursorX, 0.01f);
		ImGui::DragFloat("Scene Y", &cursorY, 0.01f);
		ImGui::DragFloat("Scene Z", &cursorZ, 0.01f);
        bool screenChanged = false;
        if (ImGui::DragFloat("Screen X", &screenCursorX, 1.0f)) screenChanged = true;
        if (ImGui::DragFloat("Screen Y", &screenCursorY, 1.0f)) screenChanged = true;

        ImGui::Separator();
        ImGui::Text("Add objects");
        if (ImGui::Button("Add Torus")) 
        {
            objects::SceneObject obj;
            obj.name = "Torus " + std::to_string(torusCounter++);
            obj.type = objects::ObjectType::Torus;
            obj.transform.shift(cursorX, cursorY, cursorZ);

            obj.R = default_R;
            obj.r = default_r;
            obj.meshAcc = default_meshAcc;
            obj.needsUpdate = true;

            sceneObjects.push_back(obj);
        }
        ImGui::SameLine();
        if (ImGui::Button("Add point")) 
        {
            objects::SceneObject obj;
            obj.name = "Point " + std::to_string(pointCounter++);
            obj.type = objects::ObjectType::Point;
            obj.transform.shift(cursorX, cursorY, cursorZ);
            sceneObjects.push_back(obj);
        }

        ImGui::Separator();
        ImGui::Text("List of scene objects");
        if (ImGui::Button("Delete selected")) 
        {
            for (int i = (int)sceneObjects.size() - 1; i >= 0; --i) 
            {
                if (sceneObjects[i].isSelected) 
                {
                    if (sceneObjects[i].VAO != 0) {
                        glDeleteVertexArrays(1, &sceneObjects[i].VAO);
                        glDeleteBuffers(1, &sceneObjects[i].VBO);
                        glDeleteBuffers(1, &sceneObjects[i].EBO);
                    }
                    sceneObjects.erase(sceneObjects.begin() + i);
                }
            }
        }
        ImGui::BeginChild("ObjectList", ImVec2(0, 200), true);
        for (int i = 0; i < sceneObjects.size(); ++i) 
        {
            ImGui::PushID(i);

            // 1. Checkbox
            ImGui::Checkbox("##sel", &sceneObjects[i].isSelected);
            ImGui::SameLine();

            // 2. Edit name
            char nameBuf[64];
            strncpy_s(nameBuf, sceneObjects[i].name.c_str(), sizeof(nameBuf));
            nameBuf[sizeof(nameBuf) - 1] = '\0';
            ImGui::InputText("##Name", nameBuf, sizeof(nameBuf));

            if (ImGui::IsItemDeactivatedAfterEdit()) 
            {
                std::string newName(nameBuf);
                bool nameExists = false;

                for (int j = 0; j < sceneObjects.size(); ++j) 
                {
                    if (i != j && sceneObjects[j].name == newName)
                    {
                        nameExists = true;
                        break;
                    }
                }

                if (!nameExists && !newName.empty()) 
                {
                    sceneObjects[i].name = newName;
                }
            }

            ImGui::PopID();
        }
        ImGui::EndChild();
        ImGui::Separator();
        ImGui::Text("Transform Pivot Point");
        int pivot = (int)CURRENT_PIVOT;
        ImGui::RadioButton("Local", &pivot, 0); ImGui::SameLine();
        ImGui::RadioButton("Median", &pivot, 1); ImGui::SameLine();
        ImGui::RadioButton("Cursor", &pivot, 2);
        CURRENT_PIVOT = (PivotType)pivot;

        ImGui::Text("Objects Rotation Axis");
        int rotAxis = (int)CURRENT_ROT_AXIS;
        ImGui::RadioButton("Free (XY)", &rotAxis, 0); ImGui::SameLine();
        ImGui::RadioButton("X", &rotAxis, 1); ImGui::SameLine();
        ImGui::RadioButton("Y", &rotAxis, 2); ImGui::SameLine();
        ImGui::RadioButton("Z", &rotAxis, 3);
        CURRENT_ROT_AXIS = (RotationAxis)rotAxis;

        ImGui::End();
        ImGui::Render();

        glClearColor(0.18f, 0.18f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // -- SCENE SETUP --
        pmath::Mat4 sceneModel;
        sceneModel.scale(SCREEN_SCALE, SCREEN_SCALE, SCREEN_SCALE);
        sceneModel.rotateX(ROT_X);
        sceneModel.rotateY(ROT_Y);
        sceneModel.shift(SHIFT_X, SHIFT_Y, 0.0f);
        // View matrix
        pmath::Mat4 view;
		view.shift(0.0f, 0.0f, 5.0f);
        // Projection matrix
		float aspect = (float)display_w / (float)display_h;
		pmath::Mat4 projection = createProjectionMatrix(aspect, pmath::degToRad(45.0f), 0.1f, 100.0f);
        
        shaderProgram.use();
		shaderProgram.setMat4("view", view);
		shaderProgram.setMat4("projection", projection);


        // -- CURSOR SETUP --
        pmath::Mat4 cursorLocal;
        cursorLocal.shift(cursorX, cursorY, cursorZ);

        pmath::Mat4 cursorModel = sceneModel * cursorLocal;
        cursorShaderProgram.use();
        cursorShaderProgram.setMat4("model", cursorModel);
        cursorShaderProgram.setMat4("view", view);
        cursorShaderProgram.setMat4("projection", projection);

        pmath::Vec4 cursorWorldPos(cursorX, cursorY, cursorZ, 1.0f);
        pmath::Vec4 clipSpace = projection * view * sceneModel * cursorWorldPos;
        float ndcX = clipSpace.x / clipSpace.w;
        float ndcY = clipSpace.y / clipSpace.w;
        float ndcZ = clipSpace.z / clipSpace.w;
		// Set scene coordinates if screen cordinates were changed
        if (screenChanged)
        {
            float newNdcX = (screenCursorX / display_w) * 2.0f - 1.0f;
            float newNdcY = 1.0f - (screenCursorY / display_h) * 2.0f;

            pmath::Mat4 invVP = projection * view * sceneModel;
            invVP.inverse();

            pmath::Vec4 newWorld = invVP * pmath::Vec4(newNdcX, newNdcY, ndcZ, 1.0f);
            cursorX = newWorld.x / newWorld.w;
            cursorY = newWorld.y / newWorld.w;
            cursorZ = newWorld.z / newWorld.w;
        }
        // Set 3D cursor coordinates
        else if (SET_CURSOR_FLAG)
        {
            float newNdcX = (TARGET_CURSOR_X / display_w) * 2.0f - 1.0f;
            float newNdcY = 1.0f - (TARGET_CURSOR_Y / display_h) * 2.0f;

            pmath::Mat4 invVP = projection * view * sceneModel;
            invVP.inverse();

            pmath::Vec4 newWorld = invVP * pmath::Vec4(newNdcX, newNdcY, ndcZ, 1.0f);
            cursorX = newWorld.x / newWorld.w;
            cursorY = newWorld.y / newWorld.w;
            cursorZ = newWorld.z / newWorld.w;

            SET_CURSOR_FLAG = false;
        }
        // Set screen coordinates
        else
        {
            screenCursorX = (ndcX + 1.0f) * 0.5f * display_w;
            screenCursorY = (1.0f - ndcY) * 0.5f * display_h;
        }


		// -- SELECTION PROCESSING --
        if (PROCESS_SELECTION) 
        {
            PROCESS_SELECTION = false;

            float ndcX = (SELECT_MOUSE_X / display_w) * 2.0f - 1.0f;
            float ndcY = 1.0f - (SELECT_MOUSE_Y / display_h) * 2.0f;

            pmath::Mat4 invVP = projection * view * sceneModel;
            invVP.inverse();

            pmath::Vec4 nearPoint = invVP * pmath::Vec4(ndcX, ndcY, -1.0f, 1.0f);
            pmath::Vec4 farPoint = invVP * pmath::Vec4(ndcX, ndcY, 1.0f, 1.0f);

            pmath::Vec3 rayOrigin(nearPoint.x / nearPoint.w, nearPoint.y / nearPoint.w, nearPoint.z / nearPoint.w);
            pmath::Vec3 rayEnd(farPoint.x / farPoint.w, farPoint.y / farPoint.w, farPoint.z / farPoint.w);

            pmath::Vec3 rayDir = (rayEnd - rayOrigin).normalize();

            int hitIndex = -1;
            float minDistanceToCamera = 999999.0f;

            for (int i = 0; i < sceneObjects.size(); ++i) 
            {
                pmath::Vec3 objPos(sceneObjects[i].transform.m[0][3], sceneObjects[i].transform.m[1][3], sceneObjects[i].transform.m[2][3]);

				// From camera to object
                pmath::Vec3 v = objPos - rayOrigin;

                float t = v.dot(rayDir);

				if (t > 0.0f) // Object behind the camera is ignored
                { 
                    pmath::Vec3 closestPoint = rayOrigin + (rayDir * t);
                    float distToRay = (objPos - closestPoint).length();

                    float scaleX = std::sqrt(sceneObjects[i].transform.m[0][0] * sceneObjects[i].transform.m[0][0] +
                        sceneObjects[i].transform.m[1][0] * sceneObjects[i].transform.m[1][0] +
                        sceneObjects[i].transform.m[2][0] * sceneObjects[i].transform.m[2][0]);

                    float threshold = (sceneObjects[i].type == objects::ObjectType::Torus) ? (sceneObjects[i].R * scaleX) : (0.3f * scaleX);

                    if (distToRay < threshold && t < minDistanceToCamera)
                    {
                        minDistanceToCamera = t;
                        hitIndex = i;
                    }
                }
            }

			// Update selection based on hit
            if (hitIndex != -1) 
            {
                if (!MULTI_SELECT) 
                {
                    for (auto& obj : sceneObjects) obj.isSelected = false;
                    sceneObjects[hitIndex].isSelected = true;
                }
                else
                    sceneObjects[hitIndex].isSelected = !sceneObjects[hitIndex].isSelected;
            }
            else 
            {
                if (!MULTI_SELECT) 
                    for (auto& obj : sceneObjects) obj.isSelected = false;
            }
        }


		// -- MEDIAN POINT SETUP --
        int selectedCount = 0;
		medianPoint = pmath::Vec3(0.0f, 0.0f, 0.0f);
        for (const auto& obj : sceneObjects)
        {
            if (obj.isSelected) 
            {
                medianPoint = medianPoint + pmath::Vec3(obj.transform.m[0][3], obj.transform.m[1][3], obj.transform.m[2][3]);
                selectedCount++;
            }
        }
        if (selectedCount > 0)
        {
            medianPoint = medianPoint * (1.0f / static_cast<float>(selectedCount));

            pmath::Mat4 medianLocal;
            medianLocal.shift(medianPoint.x, medianPoint.y, medianPoint.z);
            pmath::Mat4 medianModel = sceneModel * medianLocal;

            shaderProgram.use();
            shaderProgram.setMat4("model", medianModel);
            shaderProgram.setVec3("uColor", 1.0f, 0.5f, 0.0f);

            glBindVertexArray(pointVAO);
            glPointSize(15.0f);
            glDrawArrays(GL_POINTS, 0, 1);
            glBindVertexArray(0);
        }


        // -- DRAW OBJECTS --
		cursorShaderProgram.use();
        glLineWidth(3.0f);
        glBindVertexArray(cursorVAO);
        glDrawElements(GL_LINES, cursorIndices.size(), GL_UNSIGNED_INT, 0);
		glLineWidth(1.0f);
        glBindVertexArray(0);

        shaderProgram.use();
        for (auto& obj : sceneObjects) 
        {
            pmath::Mat4 objectModel = sceneModel * obj.transform;
            shaderProgram.setMat4("model", objectModel);

            if (obj.isSelected) shaderProgram.setVec3("uColor", 0.7f, 0.8f, 1.0f);
            else shaderProgram.setVec3("uColor", 1.0f, 1.0f, 1.0f);

            if (obj.type == objects::ObjectType::Torus) 
            {
                if (obj.needsUpdate || obj.VAO == 0)
                {
                    if (obj.VAO == 0) 
                    {
                        glGenVertexArrays(1, &obj.VAO);
                        glGenBuffers(1, &obj.VBO);
                        glGenBuffers(1, &obj.EBO);
                    }

                    objects::Torus t(obj.R, obj.r, obj.meshAcc, obj.meshAcc);
                    glBindVertexArray(obj.VAO);
                    glBindBuffer(GL_ARRAY_BUFFER, obj.VBO);
                    glBufferData(GL_ARRAY_BUFFER, t.vertices.size() * sizeof(float), t.vertices.data(), GL_STATIC_DRAW);
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.EBO);
                    glBufferData(GL_ELEMENT_ARRAY_BUFFER, t.indices.size() * sizeof(unsigned int), t.indices.data(), GL_STATIC_DRAW);
                    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
                    glEnableVertexAttribArray(0);
                    glBindVertexArray(0);

                    obj.indexCount = t.indices.size();
                    obj.needsUpdate = false;
                }

                glBindVertexArray(obj.VAO);
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glDrawElements(GL_TRIANGLES, obj.indexCount, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
            else if (obj.type == objects::ObjectType::Point) 
            {
                glBindVertexArray(pointVAO);
                glPointSize(10.0f);
                glDrawArrays(GL_POINTS, 0, 1);
                glBindVertexArray(0);
            }
        }
        

        // glfw: swap buffers
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}







void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// GLFW scroll handling
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    float minScale = 0.1f;
    float maxScale = 5.0f;

    SCREEN_SCALE += static_cast<float>(yoffset) * SCROLL_SPEED;

    if (SCREEN_SCALE > maxScale)
        SCREEN_SCALE = maxScale;
    else if (SCREEN_SCALE < minScale)
        SCREEN_SCALE = minScale;
}

// GLFW mouse button handling
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    bool alt_pressed = (mods & GLFW_MOD_ALT);
    bool shift_pressed = (mods & GLFW_MOD_SHIFT);
    bool ctrl_pressed = (mods & GLFW_MOD_CONTROL);
    bool s_pressed = (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);
    bool r_pressed = (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS);
    bool t_pressed = (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS);

    if (button == GLFW_MOUSE_BUTTON_LEFT) 
    {
        if (action == GLFW_PRESS) 
        {
            if (alt_pressed) 
            {
                // Camera rotate
                CAM_ROTATE_ACTIVE = true;
                glfwGetCursorPos(window, &LAST_MOUSE_X, &LAST_MOUSE_Y);
            }
            else if (s_pressed || r_pressed || t_pressed)
            {
                if (s_pressed) OBJ_SCALE_ACTIVE = true;
                else if (r_pressed) OBJ_ROTATE_ACTIVE = true;
                else if (t_pressed) OBJ_TRANSLATE_ACTIVE = true;
                glfwGetCursorPos(window, &START_MOUSE_X, &START_MOUSE_Y);

                for (auto &obj : sceneObjects)
                {
                    if (obj.isSelected)
                    {
                        obj.initialTransform = obj.transform;
                    }
				}

                if (CURRENT_PIVOT == PivotType::Median)
                    pivotPoint = medianPoint;
                else if (CURRENT_PIVOT == PivotType::Cursor)
                    pivotPoint = pmath::Vec3(cursorX, cursorY, cursorZ);
			}
            else 
            {
				// Register selection start
                PROCESS_SELECTION = true;
                MULTI_SELECT = ctrl_pressed;
                glfwGetCursorPos(window, &SELECT_MOUSE_X, &SELECT_MOUSE_Y);
                LMB_PRESSED = true;
            }
        }
        else if (action == GLFW_RELEASE) 
        {
            CAM_ROTATE_ACTIVE = false;
            LMB_PRESSED = false;
			OBJ_SCALE_ACTIVE = false;
            OBJ_ROTATE_ACTIVE = false;
            OBJ_TRANSLATE_ACTIVE = false;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT) 
    {
        if (action == GLFW_PRESS) 
        {
            if (alt_pressed) 
            {
                // Camera pan
                CAM_PAN_ACTIVE = true;
                glfwGetCursorPos(window, &LAST_MOUSE_X, &LAST_MOUSE_Y);
            }
            else if (shift_pressed) 
            {
				// Set 3D cursor position
                SET_CURSOR_FLAG = true;
                glfwGetCursorPos(window, &TARGET_CURSOR_X, &TARGET_CURSOR_Y);
            }
            else 
            {
                RMB_PRESSED = true;
            }
        }
        else if (action == GLFW_RELEASE) 
        {
            CAM_PAN_ACTIVE = false;
            RMB_PRESSED = false;
        }
    }
}

// GLFW mouse movement handling
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	double objectDeltaX = xpos - START_MOUSE_X;
	double objectDeltaY = ypos - START_MOUSE_Y;
	double cameraDeltaX = xpos - LAST_MOUSE_X;
	double cameraDeltaY = ypos - LAST_MOUSE_Y;

	if (CAM_ROTATE_ACTIVE) // camera rotation
    {
        ROT_Y += static_cast<float>(-cameraDeltaX) * ROTATE_SPEED;
        ROT_X += static_cast<float>(-cameraDeltaY) * ROTATE_SPEED;

        LAST_MOUSE_X = xpos;
        LAST_MOUSE_Y = ypos;
    }
	else if (CAM_PAN_ACTIVE) // camera panning
    {
        SHIFT_X += static_cast<float>(cameraDeltaX) * PAN_SPEED;
        SHIFT_Y -= static_cast<float>(cameraDeltaY) * PAN_SPEED;

        LAST_MOUSE_X = xpos;
        LAST_MOUSE_Y = ypos;
    }
	else if (OBJ_SCALE_ACTIVE) // object scaling
    {
        float scaleDelta = static_cast<float>(objectDeltaX - objectDeltaY) * 0.005f;
        float factor = 1.0f + scaleDelta;
        if (factor <= 0.01f) factor = 0.01f;

        pmath::Vec3 cursorPt(cursorX, cursorY, cursorZ);

        for (auto& obj : sceneObjects)
        {
            if (obj.isSelected)
            {
                pmath::Vec3 pivot;
                if (CURRENT_PIVOT == PivotType::Local) 
                    pivot = pmath::Vec3(obj.initialTransform.m[0][3], obj.initialTransform.m[1][3], obj.initialTransform.m[2][3]);
                else pivot = pivotPoint;

                pmath::Mat4 delta;
                delta.shift(-pivot.x, -pivot.y, -pivot.z);
                delta.scale(factor, factor, factor);
                delta.shift(pivot.x, pivot.y, pivot.z);

                obj.transform = delta * obj.initialTransform;
            }
        }
    }
	else if (OBJ_ROTATE_ACTIVE) // object rotation
    {
        float rotAngleY = static_cast<float>(-objectDeltaX) * ROTATE_SPEED;
        float rotAngleX = static_cast<float>(-objectDeltaY) * ROTATE_SPEED;
        float combinedAngle = static_cast<float>(objectDeltaY - objectDeltaX) * ROTATE_SPEED;

        pmath::Vec3 cursorPt(cursorX, cursorY, cursorZ);

        for (auto& obj : sceneObjects)
        {
            if (obj.isSelected)
            {
                pmath::Vec3 pivot;
                if (CURRENT_PIVOT == PivotType::Local) 
                    pivot = pmath::Vec3(obj.initialTransform.m[0][3], obj.initialTransform.m[1][3], obj.initialTransform.m[2][3]);
                else pivot = pivotPoint;

                pmath::Mat4 delta;
                delta.shift(-pivot.x, -pivot.y, -pivot.z);
                if (CURRENT_ROT_AXIS == RotationAxis::Free) {
                    delta.rotateX(rotAngleX);
                    delta.rotateY(rotAngleY);
                }
                else if (CURRENT_ROT_AXIS == RotationAxis::X) delta.rotateX(combinedAngle);
                else if (CURRENT_ROT_AXIS == RotationAxis::Y) delta.rotateY(combinedAngle);
                else if (CURRENT_ROT_AXIS == RotationAxis::Z) delta.rotateZ(combinedAngle);

                delta.shift(pivot.x, pivot.y, pivot.z);

                obj.transform = delta * obj.initialTransform;
            }
        }
    }
	else if (OBJ_TRANSLATE_ACTIVE) // object translation
    {
        float moveX = static_cast<float>(objectDeltaX) * PAN_SPEED;
        float moveY = static_cast<float>(-objectDeltaY) * PAN_SPEED;

        pmath::Mat4 invSceneModel;
        invSceneModel.scale(SCREEN_SCALE, SCREEN_SCALE, SCREEN_SCALE);
        invSceneModel.rotateX(ROT_X);
        invSceneModel.rotateY(ROT_Y);
        invSceneModel.inverse();

        pmath::Vec4 moveDir(moveX, moveY, 0.0f, 0.0f);
        pmath::Vec4 worldMoveDir = invSceneModel * moveDir;

        for (auto& obj : sceneObjects)
        {
            if (obj.isSelected)
            {
                pmath::Mat4 delta;
                delta.shift(worldMoveDir.x, worldMoveDir.y, worldMoveDir.z);

                obj.transform = delta * obj.initialTransform;
            }
        }
    }
}