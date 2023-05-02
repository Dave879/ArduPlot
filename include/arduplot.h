#pragma once
#include <Mahi/Gui.hpp>
#include <Mahi/Util.hpp>
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <json.hpp>

#include "usb_input.h"
#include "utilities.h"
#include "log.h"

using namespace mahi::gui;
using namespace mahi::util;

class ArduPlot : public Application
{

private:
	// Heatmap
	// 0  1    2         3    4     5     (6) (7)
	// ID:TYPE:GRAPHTYPE:NAME:SIZEX:SIZEY:MIN:MAX

	// Line/Bar
	// 0  1    2         3    (4) (5)
	// ID:TYPE:GRAPHTYPE:NAME:MIN:MAX

	enum heatmap_tkn_idx_
	{
		SIZEX = 4,
		SIZEY,
		MINH,
		MAXH
	};

	enum line_tkn_idx_
	{
		MIN = 4,
		MAX
	};

	enum tkn_idx_
	{
		ID,
		TYPE,
		GRAPHTYPE,
		NAME,
	};

	std::vector<std::string> paths;
	SerialConsole serial_console = SerialConsole("Serial Console");
	SerialConsole json_console = SerialConsole("Json Console");
	uint16_t counter = 0;
	uint16_t tps = 0;
	double startTime;
	std::chrono::steady_clock::time_point start_time;
	USBInput input_stream = USBInput();

	uint64_t pkt_idx_ = 0;
	uint64_t packets_lost = 0;

	std::string data_buffer = "";
	std::string current_data_packet = "";

	std::string GetFirstJsonPacketInBuffer(std::string &data_buffer);
	json json_data;
	std::vector<iDGraphData> id_graphs;
	std::vector<iiDGraphData> iid_graphs;
	void UpdateDataStructures(json &j);
	void DrawPlots();
	void DrawStatWindow();
	double seconds_since_start = 0;

public:
	ArduPlot();
	void update() override;
};
