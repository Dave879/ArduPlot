#include <Mahi/Gui.hpp>
#include <Mahi/Util.hpp>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <enumserial.h>
#include <serialport.h>
#include <json.hpp>
#include <imgui.h>

#include "ArduPlot.h"
#include "utils.h"

#include "log.h"

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
	if (connected_to_device)
		ParseJson();

	DrawPlots();
	DrawPortAndBaudrateChooser();
	ShowExampleAppLog(json_data.dump(4), !connected_to_device, safe_new_data);
	// PlotHeatmap();
}

int serialport_read_until(int fd, char *buf, char until, int buf_max, int timeout)
{
	char b[1]; // read expects an array, so we give it a 1-byte array
	int i = 0;
	do
	{
		int n = read(fd, b, 1); // read a char at a time
		if (n == -1)
			return -1; // couldn't read
		if (n == 0)
		{
			usleep(1000); // wait 1000 usec and try again
			timeout--;
			if (timeout == 0)
				return -2;
			continue;
		}
#ifdef SERIALPORTDEBUG
		printf("serialport_read_until: i=%d, n=%d b='%c'\n", i, n, b[0]); // debug
#endif
		buf[i] = b[0];
		i++;
	} while (b[0] != until && i < buf_max && timeout > 0);

	buf[i] = '\0'; // null terminate the string
	return i;
}

void ConnectAndReadFromSerial(bool &join_read_thread, bool &connected, Lockable &lockable, std::string port, std::string baudrate, std::string &USB_data, bool &new_data)
{
	char data[500] = {0};
	int length;

	std::string s = "/dev/" + port;
	try
	{
		bool res = openAndConfigureSerialPort(s.c_str(), std::stoi(baudrate));
		if (res)
		{
			AP_LOG_g("Successfully connected to " << s)
				connected = true;
		}
		else
			AP_LOG_r("There was an error connecting to " << s)
	}
	catch (const std::exception &e)
	{
		AP_LOG_r(e.what());
	}
	flushSerialData();

	uint64_t bit_sum_in_one_second = 0;
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	while (!join_read_thread)
	{
		length = serialport_read_until(6, data, '\n', 500, 100);
		if (length >= 0)
		{
			Lock lock(lockable);
			//new_data = true;
			//USB_data = std::string(data, data + length);
			end = std::chrono::steady_clock::now();
			if (std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() > 1000) // 1s
			{
				float mb_s = bit_sum_in_one_second * 0.000001f;
				AP_LOG_r(bit_sum_in_one_second << "b/s = " << mb_s << "Mb/s")
				begin = std::chrono::steady_clock::now();
				bit_sum_in_one_second = 0;
			} else {
				bit_sum_in_one_second += length * 8;
			}
		}
		new_data = false;
	}
	closeSerialPort();
}

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
		for (int n = 0; n < IM_ARRAYSIZE(baudrate_list); n++)
		{
			bool is_selected = (current_baudrate == baudrate_list[n]); // You can store your selection however you want, outside or inside your objects
			if (ImGui::Selectable(baudrate_list[n], is_selected))
				current_baudrate = baudrate_list[n];
			if (is_selected)
				ImGui::SetItemDefaultFocus(); // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
		}
		ImGui::EndCombo();
	}
	if (ImGui::Button(!connected_to_device ? "Connect" : "Disconnect", ImGui::GetContentRegionAvail()))
	{
		if (!connected_to_device)
		{
			serial_read_thread = std::thread(ConnectAndReadFromSerial, std::ref(join_read_thread), std::ref(connected_to_device), std::ref(mtx), current_item, current_baudrate, std::ref(incoming_data), std::ref(new_data));
		}
		else
		{
			join_read_thread = true;
			while (!serial_read_thread.joinable())
			{
				AP_LOG("Waiting for thread to be joinable...")
			}
			serial_read_thread.join();
			join_read_thread = false;
			connected_to_device = false;
		}
	}
	ImGui::End();

	ImGui::Begin("Performance profiler");
	ImGui::Text("%i b/s", bytes_per_second * 8);
	float kb = (float)bytes_per_second * 8 * 0.0001;
	float mb = (float)bytes_per_second * 8 * 0.0000001;
	ImGui::Text("%f kb/s", kb);
	ImGui::Text("%f Mb/s", mb);
	ImGui::End();
}

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
