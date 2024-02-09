
#include "application.h"

Application::Application(int width, int height, const std::string &title) : ShouldClose(false), height(height), width(width), title(title)
{
	mINI::INIFile file("config.ini");
	mINI::INIStructure ini;
	file.read(ini);

	try
	{
		setting_scaling = std::stof(ini["settings"]["scaling"]);
	}
	catch (const std::exception &e)
	{
		AP_LOG_r("An error occured while reading the configuration file")
			 setting_scaling = 1;
	}

	InitOpenGL();

#if DEBUG_OPENGL
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(GLDebugCallback, 0);
#endif

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

float Application::GetDpiScale()
{
	auto monitor = glfwGetPrimaryMonitor();
	glfwGetMonitorContentScale(monitor, &xscale, &yscale);
	return xscale;
}

void Application::InitImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();

	ImGuiIO &io = ImGui::GetIO();

	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

	ImFontConfig font_cfg;
	font_cfg.PixelSnapH = true;
	font_cfg.OversampleH = 1;
	font_cfg.OversampleV = 1;
	font_cfg.FontDataOwnedByAtlas = false;

	io.Fonts->AddFontFromFileTTF("./resources/fonts/RobotoMono-Bold.ttf", IM_ROUND(15.0f * xscale * setting_scaling), &font_cfg);

	ImGuiStyle &s = ImGui::GetStyle();

	s.ScaleAllSizes(setting_scaling);

	io.ConfigWindowsMoveFromTitleBarOnly = true;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;		// Enable Docking
	// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

	io.FontGlobalScale = 1.0f / xscale;
	io.DisplayFramebufferScale = ImVec2(xscale, xscale);

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

void Application::run()
{
	while (!ShouldClose)
	{
		ZoneScoped;
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();

		{
			ZoneScopedN("ArduPlot Render");
			update();
		}

		{
			ZoneScopedN("ImGui Render");
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}

		{
			ZoneScopedN("Swap buffers");
			glfwSwapBuffers(window);
		}
		FrameMark;
		{
			ZoneScopedN("Poll events");
			glfwPollEvents();
		}
		if ((glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) || glfwWindowShouldClose(window))
			ShouldClose = true;
	}
}
