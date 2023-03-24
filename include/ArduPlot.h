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
	std::vector<std::string> paths;
	SerialConsole serial_console = SerialConsole("Serial Console");
	SerialConsole json_console = SerialConsole("Json Console");
	uint16_t counter = 0;
	uint16_t tps = 0;
	double startTime;
	USBInput input_stream = USBInput();

	std::string data_buffer = "";
	std::string current_data_packet = "";

	std::string GetFirstJsonPacketInBuffer(std::string &data_buffer);
	json json_data;
	std::vector<iDGraphData> id_graphs;
	std::vector<iiDGraphData> iid_graphs;
	void UpdateDataStructures(json &j);
	void DrawPlots();
	float seconds_since_start = 0;

public:
	ArduPlot();
	void update() override;
};
