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

// global variables
const unsigned int SCR_WIDTH = 1800;
const unsigned int SCR_HEIGHT = 1200;

float R = 1.0f;
float r = 0.3f;
int meshAcc = 32;

bool changesMade = false;

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
    glEnable(GL_DEPTH_TEST);
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
            pmath::Torus t(R, r, meshAcc, meshAcc);
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