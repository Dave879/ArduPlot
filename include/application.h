#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <functional>
#include <chrono>
#include <thread>
#include "utilities.h"

#include "tracy/Tracy.hpp"

#include <implot.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "imgui_custom.h"

class Application
{
private:
	void GetInput();
	void OnScroll(double xoffset, double yoffset);
	void InitImGui();
	void InitOpenGL();
	float GetDpiScale() const;

	GLFWwindow *window;

	ImVec2 mouseScrollOffset;
	double mousePosx;
	double mousePosy;
	int width, height;
	std::string title;
	float xscale = 1, yscale = 1;

	bool mouseScrollEvent;
	bool mouseIsOverViewport;
	bool mouseIsPressedLeft;
	bool mouseIsPressedRight;
	bool mouseIsPressedMiddle;

	bool ShouldClose;

public:
	Application(int width = 1920, int height = 1080, const std::string &title = "Application");
	bool AppShouldClose();
	virtual void update(){};
	void run();
	~Application();
};
