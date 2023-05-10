
#include "arduplot.h"

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
	startTime = ImGui::GetTime();
	data_buffer = "";
	current_data_packet = "";
	start_time = std::chrono::steady_clock::now();
}

void ArduPlot::update()
{
	//AP_LOG("Render loop started");
	ImGui::DockSpaceOverViewport();
	//AP_LOG("1");
	input_stream.DrawDataInputPanel();
	//AP_LOG("2");
	if (input_stream.IsConnected() && !read_thread_started)
	{
		AP_LOG("Spawned thread")
		read_thread = std::thread(&ArduPlot::ReadThread, this);
		read_thread_started = true;
	}
	if (!input_stream.IsConnected() && read_thread_started)
	{
		AP_LOG("Try to join read thread...");
		read_thread.join();
		AP_LOG("Joined!");
	}

	//AP_LOG("3");
	//mtx.lock();
	//AP_LOG("4");
	DrawStatWindow();
	//AP_LOG("5");
	DrawPlots();
	//AP_LOG("6");
	json_console.Display();
	//AP_LOG("7");
	serial_console.Display();
	//AP_LOG("8");
	//mtx.unlock();
	//AP_LOG("9");
	seconds_since_start += ImGui::GetIO().DeltaTime;
	//AP_LOG("Render loop finished");
}

void ArduPlot::ReadThread()
{
	AP_LOG("Read thread started")
	measurement_start_time = std::chrono::system_clock::now() + std::chrono::seconds(1);

	while (input_stream.IsConnected())
	{
		current_data_packet = input_stream.GetData();
		Mb_s += current_data_packet.size();
		data_buffer += current_data_packet;
		if (current_data_packet.size() > 0)
		{
			AP_LOG(current_data_packet.size());
		}

		std::string pkt = "";
		do
		{
			
			pkt = GetFirstJsonPacketInBuffer(data_buffer);
			if (pkt != "")
			{
				AP_LOG(pkt)
				try
				{
					simdjson::padded_string json_data(pkt);
					simdjson::dom::object obj;
					auto error = parser.parse(json_data).get(obj);
					if (error == simdjson::error_code::SUCCESS)
					{
						AP_LOG("UpdateDataStructures calleds")
						UpdateDataStructures(obj);
					} else{
						AP_LOG_r(error)
					}
					pkt_idx_++;
					json_console.Add(pkt + "\n");
				}
				catch (const std::exception &e)
				{
					AP_LOG_r(pkt);
					AP_LOG_r("Exception when parsing json");
					AP_LOG_r(e.what());
				}
			}

			if (measurement_start_time <= std::chrono::system_clock::now())
			{
				measurement_start_time = std::chrono::system_clock::now() + std::chrono::seconds(1);
				Mb_s = Mb_s * 8 / 1e+6;
				display_Mbps = Mb_s;
				display_count = count;
				Mb_s = 0;
				count = 0;
			}
			count++;
		} while (pkt != "");
		current_data_packet = "";
		data_buffer = "";
		Mb_s = 0;
	}
	AP_LOG("Thread closed");
}

std::string ArduPlot::GetFirstJsonPacketInBuffer(std::string &data_buffer)
{
	const int32_t found_opening = data_buffer.find('{'), found_closing = data_buffer.find('}', found_opening);
	if (found_opening != std::string::npos && found_closing != std::string::npos)
	{
		std::string packet = data_buffer.substr(found_opening, (found_closing - found_opening) + 1);
		data_buffer = data_buffer.substr(found_closing + 1);
		return packet;
	}
	return "";
}

void ArduPlot::DrawStatWindow()
{
	ImGui::Begin("Stats");
	ImGui::Text("Packets dropped: %llu", packets_lost);
	if (ImGui::BeginTable("Index", 2))
	{
		ImGui::TableNextColumn();
		ImGui::Text("µC index: %llu", uC_idx);
		ImGui::TableNextColumn();
		ImGui::Text("Internal index: %llu", pkt_idx_);
		ImGui::EndTable();
	}
	ImGui::Text("Throughput: %.3f Mb/s", display_Mbps);
	ImGui::Text("Cycles: %llu", display_count);

	ImGui::End();
}

void ArduPlot::UpdateDataStructures(simdjson::dom::object &j)
{
	for (auto [key, value] : j)
	{
		std::vector<std::string> tkn = split(std::string(key), "£");
		try
		{
			if (tkn.at(tkn_idx_::TYPE) == "n") // If graph has numerical data
			{
				if (tkn.at(tkn_idx_::GRAPHTYPE) == "l" || tkn.at(tkn_idx_::GRAPHTYPE) == "b") // Update data structure for line or bar graph
				{
					uint16_t graphID = std::stoul(tkn.at(tkn_idx_::ID));
					std::string graphName = tkn.at(tkn_idx_::NAME);
					try
					{
						id_graphs.at(graphID).graphName = graphName;
						id_graphs.at(graphID).buffer.AddPoint(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time).count() / 1e9, double(value));
						if (tkn.size() == 6)
						{
							id_graphs.at(graphID).min = std::stoi(tkn.at(line_tkn_idx_::MIN));
							id_graphs.at(graphID).max = std::stoi(tkn.at(line_tkn_idx_::MAX));
							id_graphs.at(graphID).has_set_min_max = true;
						}
					}
					catch (const std::exception &e)
					{
						iDGraphData gd(graphName, GraphType::LINE);
						if (tkn.at(tkn_idx_::GRAPHTYPE) == "b")
							gd.type = GraphType::BAR;
						gd.buffer.AddPoint(seconds_since_start, double(value));
						id_graphs.push_back(gd);
						AP_LOG_b("Created new graph: " << id_graphs.at(graphID).graphName)
					}
				}
				else if (tkn.at(tkn_idx_::GRAPHTYPE) == "h") // Update data structure for heatmap
				{
					uint16_t graphID = std::stoul(tkn.at(tkn_idx_::ID));
					std::string graphName = tkn.at(tkn_idx_::NAME);
					try
					{
						iid_graphs.at(graphID).graphName = graphName;
						iid_graphs.at(graphID).sizex = std::stoul(tkn.at(heatmap_tkn_idx_::SIZEX));
						iid_graphs.at(graphID).sizey = std::stoul(tkn.at(heatmap_tkn_idx_::SIZEY));
						if (tkn.size() == 8)
						{
							iid_graphs.at(graphID).min = std::stoi(tkn.at(heatmap_tkn_idx_::MINH));
							iid_graphs.at(graphID).max = std::stoi(tkn.at(heatmap_tkn_idx_::MAXH));
							iid_graphs.at(graphID).has_set_min_max = true;
						}
						size_t i = 0;
						for (auto element : value.get_array())
						{
							iid_graphs.at(graphID).buffer.at(i) = double(element);
							i++;
						}
					}
					catch (const std::exception &e)
					{
						iiDGraphData gd(graphName);
						iid_graphs.push_back(gd);
						iid_graphs.at(graphID).sizex = std::stoul(tkn.at(heatmap_tkn_idx_::SIZEX));
						iid_graphs.at(graphID).sizey = std::stoul(tkn.at(heatmap_tkn_idx_::SIZEY));

						if (tkn.size() == 8)
						{
							iid_graphs.at(graphID).min = std::stof(tkn.at(heatmap_tkn_idx_::MINH));
							iid_graphs.at(graphID).max = std::stof(tkn.at(heatmap_tkn_idx_::MAXH));
							iid_graphs.at(graphID).has_set_min_max = true;
						}
						size_t i = 0;
						for (auto element : value.get_array())
						{
							iid_graphs.at(graphID).buffer.push_back(double(element));
							i++;
						}

						AP_LOG_b("Created new heatmap: " << iid_graphs.at(graphID).graphName);
					}
				}
			}
			else if (tkn.at(tkn_idx_::TYPE) == "s")
			{
				std::string_view val = value;
				std::string contents(val);
				if (contents.find("\\n") != std::string::npos)
				{
					contents.replace(contents.find("\\"), 2, "\n");
				}
				serial_console.Add(contents.c_str());
			}
			else if (tkn.at(tkn_idx_::TYPE) == "i")
			{
				uC_idx = value;
				if (uC_idx < pkt_idx_) // Microcontroller reflashed/rebooted/crashed/power was unplugged
				{
					pkt_idx_ = uC_idx;
					packets_lost = 0;
				}

				if (abs((int64_t)uC_idx - (int64_t)pkt_idx_) > 1000) // Kind of bad solution
				{
					pkt_idx_ = uC_idx;
					packets_lost = 0;
					AP_LOG("Greater than 1000, possible uC reset or disconnection")
				}
				else if (uC_idx == pkt_idx_)
				{
				}
				else
				{
					AP_LOG("Lost packets... Resetting internal index")
					packets_lost += uC_idx - pkt_idx_;
					pkt_idx_ = uC_idx;
				}
			}
			else if (tkn.at(tkn_idx_::TYPE) == "m")
			{
				uint16_t graphID = std::stoul(tkn.at(tkn_idx_::ID));
				std::string_view val = value;
				std::string contents(val);
				try
				{
					msg_box.at(graphID) = contents;
				}
				catch (const std::exception &e)
				{
					msg_box.push_back(contents);
				}
			}
		}
		catch (const std::exception &e)
		{
			AP_LOG_r("Exception in UpdateDataStructures:");
			AP_LOG_r(e.what());
		}
	}
}
void ArduPlot::DrawPlots()
{
	for (uint16_t i = 0; i < id_graphs.size(); i++)
	{
		ImGui::Begin(id_graphs.at(i).graphName.c_str());

		ImGui::SliderFloat("History", &id_graphs.at(i).history, 1, 135, "%.1f s");

		static ImPlotAxisFlags rt_axis = ImPlotAxisFlags_None;
		ImPlot::SetNextPlotLimitsX(seconds_since_start - id_graphs.at(i).history, seconds_since_start, ImGuiCond_Always);

		if (id_graphs.at(i).has_set_min_max)
		{
			ImPlot::SetNextPlotLimitsY(id_graphs.at(i).min, id_graphs.at(i).max, ImGuiCond_Always);
		}
		else
		{
			ImPlot::SetNextPlotLimitsY(-100, 100);
		}
		if (ImPlot::BeginPlot("##Scrolling", NULL, NULL, ImVec2(-1, -1), 0, rt_axis, rt_axis))
		{
			ImPlot::PlotLine(id_graphs.at(i).graphName.c_str(), &id_graphs.at(i).buffer.Data[0].x, &id_graphs.at(i).buffer.Data[0].y, id_graphs.at(i).buffer.Data.size(), id_graphs.at(i).buffer.Offset, 2 * sizeof(float));
			ImPlot::EndPlot();
		}
		ImGui::End();
	}

	for (uint16_t i = 0; i < iid_graphs.size(); i++)
	{
		ImGui::Begin(iid_graphs.at(i).graphName.c_str());

		ImGui::DragFloatRange2("Min / Max", &iid_graphs.at(i).min, &iid_graphs.at(i).max, 0.01f, -1000, 1000);

		ImPlot::PushColormap(ImPlotColormap_Viridis);
		if (ImPlot::BeginPlot("##Heatmap1", NULL, NULL, ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), ImPlotFlags_NoLegend | ImPlotFlags_NoMousePos, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_Invert))
		{
			ImPlot::PlotHeatmap("heat", &iid_graphs.at(i).buffer.at(0), iid_graphs.at(i).sizex, iid_graphs.at(i).sizey, iid_graphs.at(i).min, iid_graphs.at(i).max, "%.0f");
			ImPlot::EndPlot();
		}
		ImPlot::PopColormap();

		ImGui::End();
	}

	if (msg_box.size() > 0)
	{
		ImGui::Begin("Repeated Messages");
		for (size_t i = 0; i < msg_box.size(); i++)
		{
			ImGui::Text("%s", msg_box.at(i).c_str());
		}
		ImGui::End();
	}
}

/*

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
