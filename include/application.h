#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <functional>
#include <chrono>
#include <thread>

#include <mini/ini.h>

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
	void InitImGui();
	void InitOpenGL();
	float GetDpiScale();
	void FramebufferSizeCallback(GLFWwindow *window, int width, int height);
	void draw();

	int width, height;
	std::string title;
	float xscale, yscale;

	float setting_scaling;

	bool ShouldClose;

public:
	Application(int width = 1920, int height = 1080, const std::string &title = "Application");
	bool AppShouldClose();

	GLFWwindow *window;

	virtual void update(){};
	void run();
	~Application();
};
