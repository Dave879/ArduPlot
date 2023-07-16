
#include "arduplot.h"

#if __APPLE__
ArduPlot::ArduPlot() : Application(500, 300, "ArduPlot")
#elif __linux__
ArduPlot::ArduPlot() : Application(1600, 800, "ArduPlot")
#endif
{
	std::string name =
		 R"(
    ___             __      ____  __      __ 
   /   |  _________/ /_  __/ __ \/ /___  / /_
  / /| | / ___/ __  / / / / /_/ / / __ \/ __/
 / ___ |/ /  / /_/ / /_/ / ____/ / /_/ / /_  
/_/  |_/_/   \__,_/\__,_/_/   /_/\____/\__/ v0.3a
                                             
)";

	AP_LOG_b("―――――――――――――――――――――――――――――――――――――――――――――――――");
	AP_LOG_b(name);
	AP_LOG_b("―――――――――――――――――――――――――――――――――――――――――――――――――");
	ImGui::GetIO().ConfigFlags &= !ImGuiConfigFlags_ViewportsEnable;
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	startTime = ImGui::GetTime();
	data_buffer = "";
	current_data_packet = "";
	start_time = std::chrono::steady_clock::now();
	tracy::SetThreadName("Main thread");
}

void ArduPlot::update()
{
	ImGui::DockSpaceOverViewport();
	input_stream.DrawDataInputPanel();
	if (input_stream.IsConnected() && !read_thread_started)
	{
		read_thread_started = true;
		read_thread = std::thread(&ArduPlot::ReadThread, this);
		AP_LOG("Spawned thread")
	}
	if (!input_stream.IsConnected() && read_thread_started)
	{
		AP_LOG("Trying to join read thread...");
		read_thread_started = false;
		read_thread.join();
		AP_LOG("Joined!");
	}

	DrawStatWindow();
	mtx.lock();
	DrawPlots();
	mtx.unlock();
	json_console.Display();
	serial_console.Display();
	seconds_since_start += ImGui::GetIO().DeltaTime;
}

void ArduPlot::ReadThread()
{
	measurement_start_time = std::chrono::system_clock::now() + std::chrono::seconds(1);
	bool b = false;
	tracy::SetThreadName("Serial Read thread");

	while (input_stream.IsConnected() && read_thread_started)
	{
		ZoneScoped;

		current_data_packet = input_stream.GetData();
		Mb_s += current_data_packet.size();
		data_buffer += current_data_packet;

		std::string pkt = "";
		simdjson::dom::object obj;
		simdjson::padded_string padded;
		do
		{
			pkt = GetFirstJsonPacketInBuffer(data_buffer);

			if (pkt != "")
			{
				try
				{
					padded = pkt;
					simdjson::error_code error = parser.parse(padded).get(obj);

					if (error == simdjson::error_code::SUCCESS)
					{
						mtx.lock();
						UpdateDataStructures(obj);
						mtx.unlock();
					}
					pkt_idx_++;
					if (display_Mbps < 50)
					{
						json_console.Add(pkt + "\n");
					}
					else
					{
						if (!b)
						{
							b = true;
							json_console.Add("Data too fast, disabled json packet output!");
						}
					}
				}
				catch (const std::exception &e)
				{
					AP_LOG_r(pkt);
					AP_LOG_r("Exception when parsing json");
					AP_LOG_r(e.what());
				}
			}

		} while (pkt != "");

		count++;
		if (measurement_start_time <= std::chrono::system_clock::now())
		{
			measurement_start_time = std::chrono::system_clock::now() + std::chrono::seconds(1);
			display_Mbps = Mb_s * 8 / 1e+6;
			display_count = count;
			Mb_s = 0;
			count = 0;
		}
	}
	current_data_packet = "";
	data_buffer = "";
	Mb_s = 0;
	display_Mbps = 0;
	count = 0;
	display_count = 0;
}

std::string ArduPlot::GetFirstJsonPacketInBuffer(std::string &data_buffer)
{
	int rogue_packet_finder = data_buffer.find('}'), found_opening = data_buffer.find('{'), found_closing;

	if (found_opening != std::string::npos)
	{
		if (rogue_packet_finder != std::string::npos)
		{
			if (rogue_packet_finder < found_opening)
			{
				serial_console.Add(data_buffer.substr(rogue_packet_finder + 1, found_opening - rogue_packet_finder - 1));
			}
			else
			{
				serial_console.Add(data_buffer.substr(0, found_opening));
			}
		}
		else
		{
			serial_console.Add(data_buffer.substr(0, found_opening));
		}
		data_buffer = data_buffer.substr(found_opening);
	}
	else if (rogue_packet_finder != std::string::npos)
	{
		serial_console.Add(data_buffer.substr(rogue_packet_finder + 1));
		data_buffer = "";
		return "";
	}
	else
	{
		serial_console.Add(data_buffer);
		data_buffer = "";
		return "";
	}

	found_closing = data_buffer.find('}');
	if (found_closing != std::string::npos)
	{
		std::string packet = data_buffer.substr(0, found_closing + 1);
		data_buffer = data_buffer.substr(found_closing + 1);
		return packet;
	}
	return "";
}
void ArduPlot::DrawStatWindow()
{
	ZoneScoped;
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
	ZoneScoped;
	std::vector<std::string> tkn;
	for (auto [key, value] : j)
	{
		tkn = split(std::string(key), '&');

		if (tkn.size() > 2)
		{
			if (tkn.at(tkn_idx_::GRAPHTYPE) == "l" || tkn.at(tkn_idx_::GRAPHTYPE) == "b")
			{
				if (FindInVec(assoc_name_id_bar_line, tkn.at(tkn_idx_::NAME)) < 0)
				{
					assoc_name_id_bar_line.push_back(tkn.at(tkn_idx_::NAME));
				}
			}
			else if (tkn.at(tkn_idx_::GRAPHTYPE) == "h")
			{
				if (FindInVec(assoc_name_id_heatmap, tkn.at(tkn_idx_::NAME)) < 0)
				{
					assoc_name_id_heatmap.push_back(tkn.at(tkn_idx_::NAME));
				}
			}
		}

		try
		{
			switch (tkn.at(tkn_idx_::TYPE).at(0))
			{
			case 'n':
				if (tkn.at(tkn_idx_::GRAPHTYPE) == "l" || tkn.at(tkn_idx_::GRAPHTYPE) == "b") // Update data structure for line or bar graph
				{
					uint16_t graphID = FindInVec(assoc_name_id_bar_line, tkn.at(tkn_idx_::NAME));
					uint16_t merge_ID = std::stoul(tkn.at(tkn_idx_::ID));
					std::string graphName = tkn.at(tkn_idx_::NAME);
					try
					{
						id_graphs.at(graphID).graphName = graphName; // This needs to stay here in order to throw an exception for the first time, when the graph is still not created in the vector

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
						gd.merge_ID = merge_ID;
						if (gd.merge_ID == 0)
							gd.is_parent = true; // The graph is displayed no matter what, can't be a multiline graph
						id_graphs.push_back(gd);
						AP_LOG_b("Created new graph: " << id_graphs.at(graphID).graphName)
					}
				}
				else if (tkn.at(tkn_idx_::GRAPHTYPE) == "h") // Update data structure for heatmap
				{
					uint16_t graphID = FindInVec(assoc_name_id_heatmap, tkn.at(tkn_idx_::NAME)); // std::stoul(tkn.at(tkn_idx_::ID));
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
							if (iid_graphs.at(graphID).buffer.size())
							{
								iid_graphs.at(graphID).buffer.at(i) = (double)element;
							}
							else
							{
								AP_LOG_r("Overflow after heatmap array creation: " << i);
							}
							i++;
						}
					}
					catch (const std::exception &e)
					{
						iiDGraphData gd(graphName);
						iid_graphs.push_back(gd);
						iid_graphs.at(graphID).sizex = std::stoul(tkn.at(heatmap_tkn_idx_::SIZEX));
						iid_graphs.at(graphID).sizey = std::stoul(tkn.at(heatmap_tkn_idx_::SIZEY));
						iid_graphs.at(graphID).buffer.reserve(iid_graphs.at(graphID).sizex * iid_graphs.at(graphID).sizey);

						if (tkn.size() == 8)
						{
							iid_graphs.at(graphID).min = std::stof(tkn.at(heatmap_tkn_idx_::MINH));
							iid_graphs.at(graphID).max = std::stof(tkn.at(heatmap_tkn_idx_::MAXH));
							iid_graphs.at(graphID).has_set_min_max = true;
						}
						size_t i = 0;
						for (auto element : value.get_array())
						{
							if (i < iid_graphs.at(graphID).sizex * iid_graphs.at(graphID).sizey)
							{
								iid_graphs.at(graphID).buffer.push_back(double(element));
							}
							else
							{
								AP_LOG_r("Packet array size doesn't agree with actual array");
							}
							i++;
						}

						AP_LOG_b("Created new heatmap: " << iid_graphs.at(graphID).graphName);
					}
				}
				break;
			case 's':
			{
				std::string_view val = value;
				std::string contents(val);
				if (contents.find("\\n") != std::string::npos)
				{
					contents.replace(contents.find("\\"), 2, "\n");
				}
				serial_console.Add(contents.c_str());
			}
			break;
			case 'i':
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
				break;
			case 'm':
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
				break;
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
		ZoneScoped;
		TracyMessageL(id_graphs.at(i).graphName.c_str());

		for (size_t i = 0; i < id_graphs.size(); i++)
		{
			if (id_graphs.at(i).merge_ID > 0 && !id_graphs.at(i).is_child)
			{
				id_graphs.at(i).is_parent = true;
				for (size_t j = 0; j < id_graphs.size(); j++)
				{
					if (id_graphs.at(i).merge_ID == id_graphs.at(j).merge_ID && i != j)
						id_graphs.at(j).is_child = true;
				}
			}
		}

		if (id_graphs.at(i).is_parent)
		{
			ImGui::Begin(id_graphs.at(i).graphName.c_str());

			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::SliderFloat("", &id_graphs.at(i).history, 1, 100, "History: %.1f s");

			if (ImPlot::BeginPlot(("##" + id_graphs.at(i).graphName).c_str(), ImVec2(-1, -1)))
			{
				ImPlot::SetupAxisLimits(ImAxis_X1, seconds_since_start - id_graphs.at(i).history, seconds_since_start, ImGuiCond_Always);
				if (id_graphs.at(i).has_set_min_max)
				{
					ImPlot::SetupAxisLimits(ImAxis_Y1, id_graphs.at(i).min, id_graphs.at(i).max, ImGuiCond_Always);
				}
				else
				{
					// ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_AutoFit); // Need to experiment with auto fitting graphs, while still retaining the freedom of manually panning
					ImPlot::SetupAxisLimits(ImAxis_Y1, -100, 100);
				}
				if (id_graphs.at(i).merge_ID > 0) // If graph is multiline
				{
					for (size_t m = 0; m < id_graphs.size(); m++)
					{
						if (id_graphs.at(m).merge_ID == id_graphs.at(i).merge_ID)
						{
							ZoneScoped;
							TracyMessageL(id_graphs.at(m).graphName.c_str());
							ImPlot::PlotLine(id_graphs.at(m).graphName.c_str(), &id_graphs.at(m).buffer.Data[0].x, &id_graphs.at(m).buffer.Data[0].y, id_graphs.at(m).buffer.Data.size(), ImPlotAxisFlags_None, id_graphs.at(m).buffer.Offset, 2 * sizeof(float));
						}
					}
				}
				else if (id_graphs.at(i).merge_ID == 0)
				{
					TracyMessageL(id_graphs.at(i).graphName.c_str());
					ImPlot::PlotLine(id_graphs.at(i).graphName.c_str(), &id_graphs.at(i).buffer.Data[0].x, &id_graphs.at(i).buffer.Data[0].y, id_graphs.at(i).buffer.Data.size(), ImPlotAxisFlags_None, id_graphs.at(i).buffer.Offset, 2 * sizeof(float));
				}
				ImPlot::EndPlot();
			}
			ImGui::End();
		}
	}

	for (uint16_t i = 0; i < iid_graphs.size(); i++)
	{
		ImGui::Begin(iid_graphs.at(i).graphName.c_str());
		if (!iid_graphs.at(i).has_set_min_max)
		{
			ImGui::DragFloatRange2("Min / Max", &iid_graphs.at(i).min, &iid_graphs.at(i).max, 0.01f, -1000, 1000);
			ImGui::SameLine();
		}

		static bool checked = false;
		std::string format;

		if (checked)
			format = "%0.f";
		else
			format = "";

		ImGui::Checkbox("Show numbers", &checked);

		ImPlot::PushColormap(ImPlotColormap_Viridis);
		if (ImPlot::BeginPlot("##Heatmap1", ImVec2(-1, -1), ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText))
		{
			ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoDecorations);
			ImPlot::PlotHeatmap("Heatmap", &iid_graphs.at(i).buffer.at(0), iid_graphs.at(i).sizex, iid_graphs.at(i).sizey, iid_graphs.at(i).min, iid_graphs.at(i).max, format.c_str());
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

ArduPlot::~ArduPlot()
{
	AP_LOG_g("Cleaning up...");
	if (read_thread_started)
	{
		read_thread_started = false;
		AP_LOG("Trying to join read thread...");
		read_thread.join();
		AP_LOG("Joined!");
	}
}

/*
	 static ImVector<ImPlotPoint> data;
	 static ImVector<ImPlotRect> rects;
	 static ImPlotRect limits, select;
	 static bool init = true;
	 if (init) {
		  for (int i = 0; i < 50; ++i)
		  {
				double x = RandomRange(0.1, 0.9);
				double y = RandomRange(0.1, 0.9);
				data.push_back(ImPlotPoint(x,y));
		  }
		  init = false;
	 }

	 ImGui::BulletText("Box select and left click mouse to create a new query rect.");
	 ImGui::BulletText("Ctrl + click in the plot area to draw points.");

	 if (ImGui::Button("Clear Queries"))
		  rects.shrink(0);

	 if (ImPlot::BeginPlot("##Centroid")) {
		  ImPlot::SetupAxesLimits(0,1,0,1);
		  if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(0) && ImGui::GetIO().KeyCtrl) {
				ImPlotPoint pt = ImPlot::GetPlotMousePos();
				data.push_back(pt);
		  }

		  ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 2, ImVec4(1, 0, 0, 1), -1.0f, ImVec4(1, 0, 0, 1));
		  ImPlot::PlotScatter("Points", &data[0].x, &data[0].y, data.size(), 0, 0, 2 * sizeof(double));
		  if (ImPlot::IsPlotSelected()) {
				select = ImPlot::GetPlotSelection();
				int cnt;
				ImPlotPoint centroid = FindCentroid(data,select,cnt);
				if (cnt > 0) {
					 ImPlot::SetNextMarkerStyle(ImPlotMarker_Square,6);
					 ImPlot::PlotScatter("Centroid", &centroid.x, &centroid.y, 1);
				}
				if (ImGui::IsMouseClicked(ImPlot::GetInputMap().SelectCancel)) {
					 CancelPlotSelection();
					 rects.push_back(select);
				}
		  }
		  for (int i = 0; i < rects.size(); ++i) {
				int cnt;
				ImPlotPoint centroid = FindCentroid(data,rects[i],cnt);
				if (cnt > 0) {
					 ImPlot::SetNextMarkerStyle(ImPlotMarker_Square,6);
					 ImPlot::PlotScatter("Centroid", &centroid.x, &centroid.y, 1);
				}
				ImPlot::DragRect(i,&rects[i].X.Min,&rects[i].Y.Min,&rects[i].X.Max,&rects[i].Y.Max,ImVec4(1,0,1,1));
		  }
		  limits  = ImPlot::GetPlotLimits();
		  ImPlot::EndPlot();
	 }
*/

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
