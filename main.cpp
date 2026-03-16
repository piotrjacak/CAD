#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui.h"

#include "pmath/pmath.h"
#include "matrices.h"
#include "shader.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);


// global variables
const unsigned int SCR_WIDTH = 1800;
const unsigned int SCR_HEIGHT = 1200;

float SCROLL_SPEED = 0.1f;
float SCREEN_SCALE = 1.0f;
float PAN_SPEED = 0.0025f;

float ROTATE_SPEED = 0.005f;
float ROT_X = 0.0f;
float ROT_Y = 0.0f;

float SHIFT_X = 0.0f;
float SHIFT_Y = 0.0f;

bool LMB_PRESSED = false;
bool RMB_PRESSED = false;
double LAST_MOUSE_X = 0.0;
double LAST_MOUSE_Y = 0.0;


int main()
{
	// -- BOILERPLATE CODE --
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Torus", NULL, NULL);
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
    //glEnable(GL_CULL_FACE);
	// -- END OF BOILERPLATE CODE --


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


    // CREATE TORUS
    float R = 1.0f;
    float r = 0.3f;
    int meshAcc = 64;
    bool changesMade = false;
    pmath::Torus t(R, r, meshAcc, meshAcc);
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Vertices to VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, t.vertices.size() * sizeof(float), t.vertices.data(), GL_STATIC_DRAW);

    // Indices to EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, t.indices.size() * sizeof(unsigned int), t.indices.data(), GL_STATIC_DRAW);

    // Configure vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);


    // INITIALIZE SHADER
    Shader shaderProgram("shader.vs", "shader.fs");



    // RENDER LOOP
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        // IMGUI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // Set up position and flags for the control panel
        //float panel_width = 300.0f;
        float panel_width = 400.0f;
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        ImGui::SetNextWindowPos(ImVec2(display_w - panel_width, 0.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panel_width, (float)display_h), ImGuiCond_Always);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove;

        // Draw control panel
        changesMade = false;
        ImGui::Begin("Options", nullptr, window_flags);
        ImGui::Text("Torus parameters");
		if (ImGui::SliderFloat("R", &R, 0.3f, 2.0f)) changesMade = true;
		if (ImGui::SliderFloat("r", &r, 0.1f, 1.0f)) changesMade = true;
        ImGui::Text("Mesh accuracy");
		if (ImGui::SliderInt("m", &meshAcc, 6, 128)) changesMade = true;
        ImGui::End();
        ImGui::Render();


		// Torus setup if changes were made
        if (changesMade)
        {
            t = pmath::Torus(R, r, meshAcc, meshAcc);
            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, t.vertices.size() * sizeof(float), t.vertices.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, t.indices.size() * sizeof(unsigned int), t.indices.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glBindVertexArray(0);
        }

        // Render
        glClearColor(0.18f, 0.18f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderProgram.use();

        // Model matrix
        pmath::Mat4 model;
        model.scale(SCREEN_SCALE, SCREEN_SCALE, SCREEN_SCALE);
        model.rotateX(ROT_X);
        model.rotateY(ROT_Y);
        model.shift(SHIFT_X, SHIFT_Y, 0.0f);
        // View matrix
        pmath::Mat4 view;
		view.shift(0.0f, 0.0f, 5.0f);
        // Projection matrix
		float aspect = (float)display_w / (float)display_h;
		pmath::Mat4 projection = createProjectionMatrix(aspect, pmath::degToRad(45.0f), 0.1f, 100.0f);
        
		shaderProgram.setMat4("model", model);
		shaderProgram.setMat4("view", view);
		shaderProgram.setMat4("projection", projection);


        // DRAW 
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, t.indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


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

    if (SCREEN_SCALE > maxScale) {
        SCREEN_SCALE = maxScale;
    }
    else if (SCREEN_SCALE < minScale) {
        SCREEN_SCALE = minScale;
    }
}

// GLFW mouse button handling
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            LMB_PRESSED = true;
            glfwGetCursorPos(window, &LAST_MOUSE_X, &LAST_MOUSE_Y);
        }
        else if (action == GLFW_RELEASE) {
            LMB_PRESSED = false;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            RMB_PRESSED = true;
            glfwGetCursorPos(window, &LAST_MOUSE_X, &LAST_MOUSE_Y);
        }
        else if (action == GLFW_RELEASE) {
            RMB_PRESSED = false;
        }
    }
}

// GLFW mouse movement handling
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (LMB_PRESSED) {
        double deltaX = LAST_MOUSE_X - xpos;
        double deltaY = LAST_MOUSE_Y - ypos;

        ROT_Y += static_cast<float>(deltaX) * ROTATE_SPEED;
        ROT_X += static_cast<float>(deltaY) * ROTATE_SPEED;

        LAST_MOUSE_X = xpos;
        LAST_MOUSE_Y = ypos;
    }
    else if (RMB_PRESSED) {
        double deltaX = xpos - LAST_MOUSE_X;
        double deltaY = ypos - LAST_MOUSE_Y;

        SHIFT_X += static_cast<float>(deltaX) * PAN_SPEED;
        SHIFT_Y -= static_cast<float>(deltaY) * PAN_SPEED;

        LAST_MOUSE_X = xpos;
        LAST_MOUSE_Y = ypos;
    }
}