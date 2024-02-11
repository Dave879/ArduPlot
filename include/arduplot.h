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
#include "input_stream.h"

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

	TracyLockable(std::mutex, mtx);
	bool read_thread_started = false;
	std::thread read_thread;

	std::vector<std::string> assoc_name_id_bar_line;
	std::vector<std::string> assoc_name_id_heatmap;

	simdjson::padded_string json_data;
	simdjson::dom::parser parser;
	simdjson::ondemand::document doc;
	simdjson::ondemand::value val;

	SerialConsole serial_console = SerialConsole("Serial Console");
	FixedBufferSerialConsole json_console = FixedBufferSerialConsole("Json Console");

	std::chrono::steady_clock::time_point start_time;

	InputStream *input_stream;

	uint64_t packets_lost = 0;

	// Used for performance monitoring of read thread
	std::chrono::system_clock::time_point measurement_start_time = std::chrono::system_clock::now();
	/**
	 * Count how many read thread cycles have been executed in the last second
	 */
	uint64_t count = 0;
	uint64_t display_count = 0;

	double Mb_s = 0;
	double display_Mbps = 0;

	std::string data_buffer = "";
	std::string current_data_packet = "";

	std::vector<iDGraphData> id_graphs;
	std::vector<iiDGraphData> iid_graphs;

	std::vector<ScatterPlotData> scatter_plots;

	std::vector<std::string> msg_box;

	double seconds_since_start = 0;

	void ReadThread();
	std::string GetFirstJsonPacketInBuffer(std::string &data_buffer);
	void UpdateDataStructures(simdjson::dom::object &j);
	void CreateHeatmap(std::string &graphName, std::vector<std::string> &tkn, simdjson::dom::element &value);
	void DrawPlots();
	void DrawStatWindow();
	void DrawMenuBar();
	void DrawDebugWindow();
	bool ValidateHeatmapPacket(std::vector<std::string> &tkn, simdjson::dom::element &value);

public:
	ArduPlot();
	void update();
	~ArduPlot();
};
