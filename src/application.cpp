
#include "application.h"

Application::Application(int width, int height, const std::string &title) : mouseScrollEvent(false), ShouldClose(false), height(height), width(width), title(title)
{
	InitOpenGL();

#if DEBUG_OPENGL
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(GLDebugCallback, 0);
#endif

	// glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	//    https://stackoverflow.com/questions/7676971/pointing-to-a-function-that-is-a-class-member-glfw-setkeycallback
	glfwSetWindowUserPointer(window, this);
	auto func = [](GLFWwindow *window, double xoffset, double yoffset)
	{
		static_cast<Application *>(glfwGetWindowUserPointer(window))->OnScroll(xoffset, yoffset);
	};

	glfwSetScrollCallback(window, func);
	InitImGui();
}

Application::~Application()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();
	glfwTerminate();
}

bool Application::AppShouldClose()
{
	return ShouldClose;
}

void Application::InitOpenGL()
{
	if (glfwInit() == GLFW_FALSE)
	{
		LOGERR("Error initializing glfw!")
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	GetDpiScale();
	window = glfwCreateWindow(width * xscale, height * yscale, title.c_str(), NULL, NULL);

	if (window == NULL)
	{
		LOGERR("Failed to initialize window")
		glfwTerminate();
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		LOGERR("Failed to initialize GLAD")
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
}

float Application::GetDpiScale() const
{
	auto monitor = glfwGetPrimaryMonitor();
	float xscale = 1, yscale = 1;
	glfwGetMonitorContentScale(monitor, &xscale, &yscale);
	return xscale;
}

void Application::InitImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();

	ImGuiIO &io = ImGui::GetIO();

	ImFontConfig font_cfg;
	font_cfg.PixelSnapH = true;
	font_cfg.OversampleH = 1;
	font_cfg.OversampleV = 1;
	font_cfg.FontDataOwnedByAtlas = false;
	io.Fonts->AddFontFromFileTTF("./resources/fonts/RobotoMono-Bold.ttf", IM_ROUND(15.0f * xscale), &font_cfg);

	io.ConfigWindowsMoveFromTitleBarOnly = true;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;		// Enable Docking
	// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

	ImGui::StyleColorsMahiDark4();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
#if __APPLE__
	// GL 3.2 + GLSL 150
	const char *glsl_version = "#version 150";
#else
	const char *glsl_version = "#version 330";
#endif
	ImGui_ImplOpenGL3_Init(glsl_version);
}

void Application::OnScroll(double xoffset, double yoffset)
{
	mouseScrollEvent = true;
	mouseScrollOffset.x = xoffset;
	mouseScrollOffset.y = yoffset;
}

void Application::GetInput()
{
	glfwGetCursorPos(window, &mousePosx, &mousePosy);
	glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ? mouseIsPressedLeft = true : mouseIsPressedLeft = false;
	glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS ? mouseIsPressedRight = true : mouseIsPressedRight = false;
	glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS ? mouseIsPressedMiddle = true : mouseIsPressedMiddle = false;
}

void Application::run()
{
	while (!ShouldClose)
	{
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		GetInput();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();

		update();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		FrameMark;
		glfwPollEvents();
		if (/*glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS || */ glfwWindowShouldClose(window))
			ShouldClose = true;

		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		std::this_thread::sleep_for(std::chrono::milliseconds(14) - std::chrono::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()));
	}
}
