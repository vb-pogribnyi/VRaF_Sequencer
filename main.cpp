#include <iostream>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <VRaFSequencer.h>

int main() {
	GLFWwindow* window;
	int width = 900;
	int height = 900;

	if (!glfwInit()) std::cout << "GLFW init failed" << std::endl;
	window = glfwCreateWindow(width, height, "GLFW window", 0, 0);
    glfwMakeContextCurrent(window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    if (!ImGui_ImplOpenGL3_Init("#version 330")) std::cout << "ImGui OpenGL init failed" << std::endl;

    float temp_val = 0;
    glm::vec2 temp_val2 (0, 0);
    VRaF::Sequencer sequencer;
	sequencer.track("Test value", &temp_val);
	sequencer.track("Test val2", &temp_val2);
    
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT);
		float now = glfwGetTime();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("Sequencer Options");
		ImGui::DragFloat("Animation value", (float*)&temp_val, 0.01f, 0, 10);
		ImGui::DragFloat2("Animation value", (float*)&temp_val2, 0.01f, 0, 10);
		ImGui::End();
		ImGui::Begin("Sequencer", 0, 16);
		sequencer.draw();
		ImGui::End();
		sequencer.update(now);
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		//std::cout << "Hello" << std::endl;
		glfwSwapBuffers(window);
	}
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
}