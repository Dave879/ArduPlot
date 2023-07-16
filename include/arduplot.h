#pragma once

#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include <thread>
#define __OPTIMIZE__ 1
#include <simdjson.h>

#include <tracy/Tracy.hpp>

#include <implot.h>

#include "string_utils.h"
#include "usb_input.h"
#include "utilities.h"
#include "application.h"
#include "log.h"

class ArduPlot : public Application
{

public:
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

	TracyLockable(std::mutex, mtx);
	bool read_thread_started = false;
	std::thread read_thread;

	std::vector<std::string> assoc_name_id_bar_line;
	std::vector<std::string> assoc_name_id_heatmap;

	simdjson::padded_string json_data;
	simdjson::dom::parser parser;
	simdjson::ondemand::document doc;
	simdjson::ondemand::value val;

	std::vector<std::string> paths;
	SerialConsole serial_console = SerialConsole("Serial Console");
	SerialConsole json_console = SerialConsole("Json Console");
	uint16_t counter = 0;
	uint16_t tps = 0;
	double startTime;
	std::chrono::steady_clock::time_point start_time;
	USBInput input_stream = USBInput();

	uint64_t pkt_idx_ = 0;
	uint64_t uC_idx = 0;
	uint64_t packets_lost = 0;

	uint64_t count = 0;
	uint64_t display_count = 0;

	double Mb_s = 0;
	double display_Mbps = 0;
	uint64_t B_sum = 0;
	std::chrono::system_clock::time_point measurement_start_time = std::chrono::system_clock::now();

	std::string data_buffer = "";
	std::string current_data_packet = "";

	void ReadThread();

	std::string GetFirstJsonPacketInBuffer(std::string &data_buffer);
	std::vector<iDGraphData> id_graphs;
	std::vector<iiDGraphData> iid_graphs;

   std::vector<ScatterPlotData> scatter_plots;

	std::vector<std::string> msg_box;

	void UpdateDataStructures(simdjson::dom::object &j);
	void DrawPlots();
	void DrawStatWindow();
	double seconds_since_start = 0;

	ArduPlot();
	void update();
	~ArduPlot();
};
