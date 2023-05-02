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
	int height = 400;
	if (!glfwInit()) std::cout << "GLFW init failed" << std::endl;
	window = glfwCreateWindow(width, height, "GLFW window", 0, 0);
    glfwMakeContextCurrent(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    if (!ImGui_ImplOpenGL3_Init("#version 330")) std::cout << "ImGui OpenGL init failed" << std::endl;

	// The values to be manipulated
    float temp_val = 0;
    glm::vec2 temp_val2 (0, 0);
	// Crate Sequencer object. Can also accept fps in its constructor
	// So to make it run 120 frames per second, it's sufficient to call
	// VRaF::Sequencer sequencer(120); 
    VRaF::Sequencer sequencer;  // Defalut fps is 30
	sequencer.track("Test value", &temp_val);
	sequencer.track("Test val2", &temp_val2);
    
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// User Interface setup
		ImGui::Begin("Sequencer Options");
		ImGui::SliderFloat("Animation value", (float*)&temp_val, 0, 10);
		ImGui::SliderFloat2("Animation value", (float*)&temp_val2, 0, 10);
		ImGui::End();
		ImGui::Begin("Sequencer", 0, 16);
		sequencer.draw();
		ImGui::End();
		// We want to let the sequencer know the current time,
		// so that it's able to run properly
		sequencer.update((float)glfwGetTime());

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
}