#include <Mahi/Gui.hpp>
#include <Mahi/Util.hpp>
#include <vector>
#include <string>
#include <thread>
#include <enumserial.h>
#include <serialport.h>
#include <json.hpp>

#include "ArduPlot.h"
#include "utils.h"

using namespace mahi::gui;
using namespace mahi::util;

ArduPlot::ArduPlot() : Application(500, 300, "ArduPlot")
{
	ImGui::GetIO().ConfigFlags &= !ImGuiConfigFlags_ViewportsEnable;
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

void ArduPlot::update()
{
	ImGui::DockSpaceOverViewport();

	DrawPortAndBaudrateChooser();

	DrawPlots(JsonData);
}

void ConnectAndReadFromSerial(json &dataRow, bool &joinReadThread, bool &connected, std::string current_item, std::string current_baudrate)
{
	SerialPort serial;
	unsigned char data[1000] = {0};
	std::string strData;
	int length;

#ifdef _WIN_
	serial.Open(paths[choice_path]); // TODO: needs to be fixed
#elif __APPLE__
	std::string s = "/dev/" + current_item;
	try
	{
		bool res = serial.Open(s.c_str());
		if (res)
		{
			std::cout << "Successfully connected to " << s << std::endl;
			connected = true;
		}
		else
			std::cout << "There was an error connecting to " << s << std::endl;
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
	}

#endif
	serial.SetBaudRate(std::stoi(current_baudrate));
	serial.SetParity(8, 1, 'N');

	while (!joinReadThread)
	{
		length = serial.Read(data);
		if (length != -1)
		{
			strData = std::string(&data[0], &data[0] + length);
			// std::cout << strData << std::endl;
			try
			{
				dataRow = json::parse(strData);
			}
			catch (const std::exception &e)
			{
				std::cout << "B|" << strData << "|E" << std::endl;
				//dataRow.clear();
				// std::cerr << e.what() << '\n';
			}
		}
	}
}

void ArduPlot::DrawPlots(json &j)
{

	// even easier with structured bindings (C++17)
	for (auto &[key, value] : j.items())
	{
		std::cout << key << " : " << value << "\n";
	}
	/*
	ImGui::Begin("Plot A0");
	static ScrollingBuffer sdata1;
	static float t = 0;
	t += ImGui::GetIO().DeltaTime;
	sdata1.AddPoint(t, -1); // TODO

	static float history = 10.0f;
	ImGui::SliderFloat("History", &history, 1, 30, "%.1f s");

	static ImPlotAxisFlags rt_axis = ImPlotAxisFlags_None;
	ImPlot::SetNextPlotLimitsX(t - history, t, ImGuiCond_Always);
	ImPlot::SetNextPlotLimitsY(-10, 1100, ImGuiCond_Always);
	if (ImPlot::BeginPlot("##Scrolling", NULL, NULL, ImVec2(-1, -1), 0, rt_axis, rt_axis))
	{
		ImPlot::PlotLine("A0", &sdata1.Data[0].x, &sdata1.Data[0].y, sdata1.Data.size(), sdata1.Offset, 2 * sizeof(float));
		// ImPlot::PlotLine("Mouse y", &sdata2.Data[0].x, &sdata2.Data[0].y, sdata2.Data.size(), sdata2.Offset, 2 * sizeof(float));
		ImPlot::EndPlot();
	}

	ImGui::End();
	*/
}

std::vector<std::string> ArduPlot::ScanForAvailableBoards()
{
	EnumSerial enumserial;
	return enumserial.EnumSerialPort(); // Enum device driver of serial port
}

void ArduPlot::DrawPortAndBaudrateChooser()
{
	ImGui::Begin("Port - Baudrate chooser");
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x / 2);
	if (ImGui::BeginCombo("##usbdevcombo", current_item.c_str())) // The second parameter is the label previewed before opening the combo.
	{
		paths = ScanForAvailableBoards();
		for (int n = 0; n < paths.size(); n++)
		{
			bool is_selected = (current_item == paths.at(n)); // You can store your selection however you want, outside or inside your objects
			if (ImGui::Selectable(paths.at(n).c_str(), is_selected))
				current_item = paths.at(n);
			if (is_selected)
				ImGui::SetItemDefaultFocus(); // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	if (ImGui::BeginCombo("##baudratecombo", current_baudrate)) // The second parameter is the label previewed before opening the combo.
	{
		for (int n = 0; n < IM_ARRAYSIZE(baudratelist); n++)
		{
			bool is_selected = (current_baudrate == baudratelist[n]); // You can store your selection however you want, outside or inside your objects
			if (ImGui::Selectable(baudratelist[n], is_selected))
				current_baudrate = baudratelist[n];
			if (is_selected)
				ImGui::SetItemDefaultFocus(); // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
		}
		ImGui::EndCombo();
	}
	if (!connected)
	{
		if (ImGui::Button("Connect", ImGui::GetContentRegionAvail()))
		{
			SerialReadThread = std::thread(ConnectAndReadFromSerial, std::ref(JsonData), std::ref(joinReadThread), std::ref(connected), current_item, current_baudrate);
		}
	}
	else
	{
		if (ImGui::Button("Disconnect", ImGui::GetContentRegionAvail()))
		{
			joinReadThread = true;
			while (!SerialReadThread.joinable())
			{
				std::cout << "Waiting for thread to be joinable..." << std::endl;
			}
			SerialReadThread.join();
			joinReadThread = false;
			connected = false;
		}
	}
	// unsigned char str[17] = "Hello Terminal \n";
	// serial.Write(str, 17);

	ImGui::End();
}

template <typename T>
void ArduPlot::NewPlot(std::string name, ScrollingBuffer sdata1, T anyData)
{
	ImGui::Begin(name.c_str());
	static float t = 0;
	t += ImGui::GetIO().DeltaTime;
	sdata1.AddPoint(t, anyData);

	static float history = 10.0f;
	ImGui::SliderFloat("History", &history, 1, 30, "%.1f s");

	static ImPlotAxisFlags rt_axis = ImPlotAxisFlags_None;
	ImPlot::SetNextPlotLimitsX(t - history, t, ImGuiCond_Always);
	ImPlot::SetNextPlotLimitsY(-10, 1100, ImGuiCond_Always);
	if (ImPlot::BeginPlot(name.c_str(), NULL, NULL, ImVec2(-1, -1), 0, rt_axis, rt_axis))
	{
		ImPlot::PlotLine(name.c_str(), &sdata1.Data[0].x, &sdata1.Data[0].y, sdata1.Data.size(), sdata1.Offset, 2 * sizeof(float));
		// ImPlot::PlotLine("Mouse y", &sdata2.Data[0].x, &sdata2.Data[0].y, sdata2.Data.size(), sdata2.Offset, 2 * sizeof(float));
		ImPlot::EndPlot();
	}
}
