#include <Mahi/Gui.hpp>
#include <Mahi/Util.hpp>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <json.hpp>
#include <imgui.h>

#include "arduplot.h"
#include "utilities.h"
#include "usb_input.h"

#include "log.h"

using namespace mahi::gui;
using namespace mahi::util;

#if __APPLE__
ArduPlot::ArduPlot() : Application(500, 300, "ArduPlot")
#elif __linux__
ArduPlot::ArduPlot() : Application(1200, 500, "ArduPlot")
#endif
{
	ImGui::GetIO().ConfigFlags &= !ImGuiConfigFlags_ViewportsEnable;
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

void ArduPlot::update()
{
	ImGui::DockSpaceOverViewport();
	ImGui::ShowDemoWindow();

	SerialConsoleDisplay("a");
}

void ArduPlot::SerialConsoleDisplay(const std::string contents)
{
	// For the demo: add a debug button _BEFORE_ the normal log window contents
	// We take advantage of a rarely used feature: multiple calls to Begin()/End() are appending to the _same_ window.
	// Most of the contents of the window will be added by the log.Draw() call.
	ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
	ImGui::Begin("Serial Console", nullptr);
	if (contents != "")
		serial_console.AddLog(contents.c_str());

	ImGui::End();
	// Actually call in the regular Log helper (which will Begin() into the same window as we just did)
	serial_console.Draw("Serial Console", nullptr);
}

/*
void ArduPlot::UpdateDataStructures(json &j)
{
	for (auto &[key, value] : j.items())
	{
		try
		{
			if (key.substr(4, 1) == "n")
			{
				if (key.substr(5, 1) == "l" || key.substr(5, 1) == "b") // Update data structure for line or bar graph
				{
					uint16_t graphID = std::stoul(key.substr(0, 4), nullptr, 16);
					std::string graphName = key.substr(key.find(">") + 1, key.size() - key.find(">"));
					try
					{
						id_graphs.at(graphID).graphName = graphName;
						id_graphs.at(graphID).buffer.AddPoint(seconds_since_start, std::stof(value.dump()));
					}
					catch (const std::exception &e)
					{
						iDGraphData gd(graphName, GraphType::LINE);
						if (key.substr(5, 1) == "b")
							gd.type = GraphType::BAR;

						gd.buffer.AddPoint(seconds_since_start, std::stof(value.dump()));
						id_graphs.push_back(gd);
						AP_LOG_b("New graph data structure was created")
					}
				}
				else if (key.substr(5, 1) == "h") // Update data structure for heatmap
				{
				}
			}
			else if (key.substr(4, 1) == "s")
			{
			}
			else if (key.substr(4, 1) == "b")
			{
			}
		}
		catch (const std::exception &e)
		{
			AP_LOG_r(e.what())
		}
	}
}

void ArduPlot::ParseJson()
{
	{
		Lock lock(mtx);
		if (new_data)
		{
			AP_LOG_b("Nuovi dati " << seconds_since_start << "s")
				 safe_incoming_data = incoming_data;
			safe_new_data = new_data;
		}
	}
	if (safe_new_data)
		if (new_data)
		{
			try
			{
				bytes_received_total += safe_incoming_data.size();
				if (seconds_since_start - past_measurement_time > 1)
				{
					past_measurement_time++;
					bytes_per_second = bytes_received_total - bytes_received_prior;
					bytes_received_prior = bytes_received_total;
				}

				json_data = json::parse(safe_incoming_data);
				// AP_LOG_g(json_data.dump(4));
				UpdateDataStructures(json_data);
			}
			catch (json::parse_error &ex)
			{
				AP_LOG_r("safe_incoming_data")
					 AP_LOG_r(safe_incoming_data)
			}
		}
}

void ArduPlot::DrawPlots()
{
	for (uint16_t i = 0; i < id_graphs.size(); i++)
	{
		ImGui::Begin(id_graphs.at(i).graphName.c_str());

		seconds_since_start += ImGui::GetIO().DeltaTime;

		ImGui::SliderFloat("History", &id_graphs.at(i).history, 1, 135, "%.1f s");

		static ImPlotAxisFlags rt_axis = ImPlotAxisFlags_None;
		ImPlot::SetNextPlotLimitsX(seconds_since_start - id_graphs.at(i).history, seconds_since_start, ImGuiCond_Always);
		ImPlot::SetNextPlotLimitsY(-10, 4000);
		if (ImPlot::BeginPlot("##Scrolling", NULL, NULL, ImVec2(-1, -1), 0, rt_axis, rt_axis))
		{
			ImPlot::PlotLine(id_graphs.at(i).graphName.c_str(), &id_graphs.at(i).buffer.Data[0].x, &id_graphs.at(i).buffer.Data[0].y, id_graphs.at(i).buffer.Data.size(), id_graphs.at(i).buffer.Offset, 2 * sizeof(float));
			ImPlot::EndPlot();
		}
		ImGui::End();
	}
}

// clang-format off
void ArduPlot::DrawMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Select binary location"))
		{
			if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
			if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {} // Disabled item
			ImGui::Separator();
			if (ImGui::MenuItem("Cut", "CTRL+X")) {}
			if (ImGui::MenuItem("Copy", "CTRL+C")) {}
			if (ImGui::MenuItem("Paste", "CTRL+V")) {}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Option")) {
			if (ImGui::MenuItem("Test", "CTRL+SOS")) {}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}
// clang-format on

void ArduPlot::DrawDataInputPanel()
{

	ImGui::Begin("Performance profiler");
	ImGui::Text("%i b/s", bytes_per_second * 8);
	float kb = (float)bytes_per_second * 8 * 0.0001;
	float mb = (float)bytes_per_second * 8 * 0.0000001;
	ImGui::Text("%f kb/s", kb);
	ImGui::Text("%f Mb/s", mb);
	ImGui::End();
}
*/

/* PlotHeatmap
void ArduPlot::PlotHeatmap()
{
#define HEATMAP_SIZE 8
	ImGui::Begin("Heatmap");

	float scale_min = 0;	// findMin(values1, 64);
	float scale_max = 1000; // findMax(values1, 64);
	if ((int)scale_max == (int)scale_min)
		scale_max++;

	static const char *xlabels[] = {"0", "1", "2", "3", "4", "5", "6", "7"};
	static const char *ylabels[] = {"7", "6", "5", "4", "3", "2", "1", "0"};

	static ImPlotColormap map = ImPlotColormap_Viridis;
	if (ImGui::Button("Change Colormap", ImVec2(225, 0)))
		map = (map + 1) % ImPlotColormap_COUNT;

	ImGui::SameLine();
	ImGui::LabelText("##Colormap Index", "%s", ImPlot::GetColormapName(map));
	ImGui::SetNextItemWidth(400);
	ImGui::DragFloatRange2("Min / Max", &scale_min, &scale_max, 0.01f, -20, 300);
	static ImPlotAxisFlags axes_flags = ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks;

	ImPlot::PushColormap(map);
	ImPlot::SetNextPlotTicksX(0 + 1.0 / 14.0, 1 - 1.0 / 14.0, HEATMAP_SIZE, xlabels);
	ImPlot::SetNextPlotTicksY(1 - 1.0 / 14.0, 0 + 1.0 / 14.0, HEATMAP_SIZE, ylabels);
	if (ImPlot::BeginPlot("##Heatmap1", NULL, NULL, ImVec2(400, 400), ImPlotFlags_NoLegend | ImPlotFlags_NoMousePos, axes_flags, axes_flags | ImPlotAxisFlags_Invert))
	{
		ImPlot::PlotHeatmap("heat", values1, HEATMAP_SIZE, HEATMAP_SIZE, scale_min, scale_max);
		ImPlot::EndPlot();
	}
	ImGui::SameLine();
	ImPlot::ShowColormapScale(scale_min, scale_max, 400);
	ImPlot::PopColormap();
	ImGui::End();
}
*/

/* DrawTooltip

void ArduPlot::DrawTooltip()
{
	//             MyImPlot::PlotCandlestick("GOOGL",dates, opens, closes, lows, highs, 218, tooltip, 0.25f, bullCol, bearCol);
	// get ImGui window DrawList
	ImDrawList* draw_list = ImPlot::GetPlotDrawList();
	// calc real value width
	double half_width = count > 1 ? (xs[1] - xs[0]) * width_percent : width_percent;

	// custom tool
	if (ImPlot::IsPlotHovered()) {
		ImPlotPoint mouse   = ImPlot::GetPlotMousePos();
		mouse.x             = ImPlot::RoundTime(ImPlotTime::FromDouble(mouse.x), ImPlotTimeUnit_Day).ToDouble();
		float  tool_l       = ImPlot::PlotToPixels(mouse.x - half_width * 1.5, mouse.y).x;
		float  tool_r       = ImPlot::PlotToPixels(mouse.x + half_width * 1.5, mouse.y).x;
		float  tool_t       = ImPlot::GetPlotPos().y;
		float  tool_b       = tool_t + ImPlot::GetPlotSize().y;
		ImPlot::PushPlotClipRect();
		draw_list->AddRectFilled(ImVec2(tool_l, tool_t), ImVec2(tool_r, tool_b), IM_COL32(128,128,128,64));
		ImPlot::PopPlotClipRect();
		// find mouse location index
		int idx = BinarySearch(xs, 0, count - 1, mouse.x);
		// render tool tip (won't be affected by plot clip rect)
		if (idx != -1) {
			ImGui::BeginTooltip();
			char buff[32];
			ImPlot::FormatDate(ImPlotTime::FromDouble(xs[idx]),buff,32,ImPlotDateFmt_DayMoYr,ImPlot::GetStyle().UseISO8601);
			ImGui::Text("Day:   %s",  buff);
			ImGui::Text("Open:  $%.2f", opens[idx]);
			ImGui::Text("Close: $%.2f", closes[idx]);
			ImGui::Text("Low:   $%.2f", lows[idx]);
			ImGui::Text("High:  $%.2f", highs[idx]);
			ImGui::EndTooltip();
		}
	}
}
*/
