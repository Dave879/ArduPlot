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
	// Mutex mtx;
	std::vector<std::string> paths;
	SerialConsole serial_console;
	uint16_t counter = 0;
	uint16_t tps = 0;
	double startTime;
	USBInput input_stream = USBInput();

	void SerialConsoleDisplay(const std::string contents = "");
	/*
		std::vector<iDGraphData> id_graphs;
		std::vector<iiDGraphData> iid_graphs;
		std::thread serial_read_thread;
		float seconds_since_start = 0;
		int16_t bytes_per_second = 0;
		int16_t bytes_received_total = 0;
		int16_t bytes_received_prior = 0;
		float past_measurement_time = 0;
		bool new_data = false;
		bool safe_new_data = true;
		std::string incoming_data = "";
		std::string safe_incoming_data = "";

		json json_data;

		bool connected_to_device = false;
		bool join_read_thread = false;
		void ParseJson();
		void UpdateDataStructures(json &j);
		void DrawPlots();
		void DrawDataInputPanel();
		void DrawMenuBar();
		void DrawTooltip();
		void PlotHeatmap();
	*/

public:
	ArduPlot();
	void update() override;
};
