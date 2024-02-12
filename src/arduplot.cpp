
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
/_/  |_/_/   \__,_/\__,_/_/   /_/\____/\__/ v0.4d
                                             
)";

	AP_LOG_b("―――――――――――――――――――――――――――――――――――――――――――――――――");
	AP_LOG_b(name);
	AP_LOG_b("―――――――――――――――――――――――――――――――――――――――――――――――――");
	ImGui::GetIO().ConfigFlags &= !ImGuiConfigFlags_ViewportsEnable;
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	input_stream = new USBInput(std::bind(glfwGetKey, window, GLFW_KEY_ENTER));
	data_buffer = "";
	current_data_packet = "";
	start_time = std::chrono::steady_clock::now();
	tracy::SetThreadName("ArduPlot GUI");
}

void ArduPlot::update()
{
	ImGui::DockSpaceOverViewport();

	input_stream->DrawGUI();

	// DrawDebugWindow();

	if (input_stream->IsConnected() && !read_thread_started)
	{
		read_thread_started = true;
		read_thread = std::thread(&ArduPlot::ReadThread, this);
		AP_LOG("Spawned thread");
	}
	if (!input_stream->IsConnected() && read_thread_started)
	{
		packets_lost = 0;
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
	tracy::SetThreadName("ArduPlot data reader");

	while (input_stream->IsConnected() && read_thread_started)
	{
		ZoneScoped;
		current_data_packet = input_stream->GetData();
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
					else
					{
						packets_lost++;
						AP_LOG_r("SimdJson error: " << error);
					}

					json_console.Add(pkt + "\n");
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
	ZoneScoped;
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
	ImGui::Text("Packets dropped: %lu", packets_lost); // On MacOS %lu wants to become %llu
	ImGui::Text("Throughput: %.3f Mb/s", display_Mbps);
	ImGui::Text("Cycles: %lu", display_count); // On MacOS %lu wants to become %llu
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::End();
}

void ArduPlot::DrawMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "CTRL+Z"))
			{
			}
			if (ImGui::MenuItem("Redo", "CTRL+Y", false, false))
			{
			} // Disabled item
			ImGui::Separator();
			if (ImGui::MenuItem("Cut", "CTRL+X"))
			{
			}
			if (ImGui::MenuItem("Copy", "CTRL+C"))
			{
			}
			if (ImGui::MenuItem("Paste", "CTRL+V"))
			{
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

bool ArduPlot::ValidateHeatmapPacket(std::vector<std::string> &tkn, simdjson::dom::element &value)
{
	int sizex = std::stoul(tkn.at(heatmap_tkn_idx_::SIZEX));
	int sizey = std::stoul(tkn.at(heatmap_tkn_idx_::SIZEY));

	if (value.get_array().size() != sizex * sizey)
	{
		packets_lost++;
		AP_LOG_r("[" << tkn.at(tkn_idx_::NAME) << "]-> Expected array size doesn't agree with actual array: expected " << sizex * sizey << ", got " << value.get_array().size()) return false;
	}
	return true;
}

void ArduPlot::UpdateDataStructures(simdjson::dom::object &j)
{
	ZoneScoped;
	std::vector<std::string> tkn;
	for (auto [key, value] : j)
	{
		tkn = split(std::string(key), '&');
		static bool packet_valid = false;
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
				packet_valid = ValidateHeatmapPacket(tkn, value);
				if (packet_valid && FindInVec(assoc_name_id_heatmap, tkn.at(tkn_idx_::NAME)) < 0)
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
						gd.buffer.AddPoint(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time).count() / 1e9, double(value));
						gd.merge_ID = merge_ID;
						if (gd.merge_ID == 0)
							gd.is_parent = true; // The graph is displayed no matter what, can't be a multiline graph
						id_graphs.push_back(gd);
						AP_LOG_b("Created new graph: " << id_graphs.at(graphID).graphName)
					}
				}
				else if (tkn.at(tkn_idx_::GRAPHTYPE) == "h") // Update data structure for heatmap
				{
					uint16_t graphID = FindInVec(assoc_name_id_heatmap, tkn.at(tkn_idx_::NAME));
					if (!packet_valid)
						continue;

					std::string graphName = tkn.at(tkn_idx_::NAME);
					if (graphID >= iid_graphs.size())
					{
						CreateHeatmap(graphName, tkn, value);
					}
					else
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
							if (i < iid_graphs.at(graphID).buffer.size())
							{
								iid_graphs.at(graphID).buffer.at(i) = (double)element;
							}
							else
							{
								packets_lost++;
								AP_LOG_r("[" << iid_graphs.at(graphID).graphName << "]-> Heatmap array packet overflow. Packet array size: " << value.get_array().size() << " Expected array size: " << iid_graphs.at(graphID).buffer.size());
								break;
							}
							i++;
						}
					}
				}
				break;
			case 's':
			{
				std::string_view val = value;
				std::string contents(val);
				// TODO
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
			packets_lost++;
			AP_LOG_r("Exception in UpdateDataStructures:");
			AP_LOG_r(e.what());
		}
	}
}

void ArduPlot::DrawDebugWindow()
{
	ImGui::Begin("Debug Window");
	for (size_t i = 0; i < iid_graphs.size(); i++)
	{
		ImGui::TextUnformatted(iid_graphs.at(i).graphName.c_str());
		ImGui::SameLine();
		ImGui::Text("w:%i, h:%i, size:%li, tsize:%i", iid_graphs.at(i).sizex, iid_graphs.at(i).sizey, iid_graphs.at(i).buffer.size(), iid_graphs.at(i).sizex * iid_graphs.at(i).sizey);
	}

	ImGui::Separator();
	for (size_t i = 0; i < id_graphs.size(); i++)
	{
		ImGui::TextUnformatted(id_graphs.at(i).graphName.c_str());
	}

	ImGui::End();
}

void ArduPlot::CreateHeatmap(std::string &graphName, std::vector<std::string> &tkn, simdjson::dom::element &value)
{
	iiDGraphData gd(graphName);
	gd.sizex = std::stoul(tkn.at(heatmap_tkn_idx_::SIZEX));
	gd.sizey = std::stoul(tkn.at(heatmap_tkn_idx_::SIZEY));
	gd.buffer.reserve(gd.sizex * gd.sizey);

	if (tkn.size() == 8)
	{
		gd.min = std::stof(tkn.at(heatmap_tkn_idx_::MINH));
		gd.max = std::stof(tkn.at(heatmap_tkn_idx_::MAXH));
		gd.has_set_min_max = true;
	}

	if (value.get_array().size() != gd.sizex * gd.sizey)
	{
		packets_lost++;
		AP_LOG_r("[" << gd.graphName << "]-> Expected array size doesn't agree with actual array: expected " << gd.sizex * gd.sizey << ", got " << value.get_array().size()) return;
	}

	for (auto element : value.get_array())
	{
		gd.buffer.push_back(double(element));
	}
	iid_graphs.push_back(gd);
	AP_LOG_b("Created new heatmap: " << gd.graphName);
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
			ImGui::SetNextWindowSize({1000.0f, 1000.0f}, ImGuiCond_::ImGuiCond_FirstUseEver);
			ImGui::Begin(id_graphs.at(i).graphName.c_str());

			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::SliderFloat("##", &id_graphs.at(i).history, 1, 100, "History: %.1f s");

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
		ZoneScopedN("IIDgraphs");
		ImGui::SetNextWindowSize({1000.0f, 1000.0f}, ImGuiCond_::ImGuiCond_FirstUseEver);
		ImGui::Begin(iid_graphs.at(i).graphName.c_str());
		if (!iid_graphs.at(i).has_set_min_max)
		{
			ImGui::DragFloatRange2("Min / Max", &iid_graphs.at(i).min, &iid_graphs.at(i).max, 0.01f, -1000, 1000);
			ImGui::SameLine();
		}

		static bool checked = false;
		static const char *format;

		if (checked)
			format = "%0.f";
		else
			format = nullptr;

		ImGui::Checkbox("Show numbers", &checked);

		ImPlot::PushColormap(ImPlotColormap_Viridis);
		if (ImPlot::BeginPlot("##Heatmap1", ImVec2(-1, -1), ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText))
		{
			ZoneScopedN("Single heatmap plot");
			ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
			// printVec(iid_graphs);
			ImPlot::PlotHeatmap(iid_graphs.at(i).graphName.c_str(), &iid_graphs.at(i).buffer.at(0), iid_graphs.at(i).sizex, iid_graphs.at(i).sizey, iid_graphs.at(i).min, iid_graphs.at(i).max, format);
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
