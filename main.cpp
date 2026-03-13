#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui.h"
#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1800;
const unsigned int SCR_HEIGHT = 1200;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Torus", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
    // Laptop screen setting
    float scale_factor = 1.5f;
    ImGui::GetStyle().ScaleAllSizes(scale_factor);
    io.FontGlobalScale = scale_factor;

    // render loop
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
        ImGui::Begin("Opcje", nullptr, window_flags);
        ImGui::Text("Parametry torusa:");
        //ImGui::SliderFloat("a", &a, 0.0f, 1.0f);
        //ImGui::SliderFloat("b", &b, 0.0f, 1.0f);
        //ImGui::SliderFloat("c", &c, 0.0f, 1.0f);
        //ImGui::Text("Parametr siatki");
        //ImGui::SliderFloat("m", &m_shiness, 0.0f, 10.0f);
        ImGui::End();
        ImGui::Render();

        // render
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // glfw: swap buffers
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}