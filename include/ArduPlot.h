#include <Mahi/Gui.hpp>
#include <Mahi/Util.hpp>
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <enumserial.h>
#include <serialport.h>
#include <json.hpp>

#include "utils.h"

using namespace mahi::gui;
using namespace mahi::util;

class ArduPlot : public Application
{

private:
	Mutex mtx;
	std::vector<std::string> paths;
	const char *baudrate_list[14] = {"110", "300", "600", "1200", "2400", "4800", "9600", "14400", "19200", "38400", "57600", "115200", "128000", "256000"};
	const char *current_baudrate = "115200";
	std::string current_item = "cu.usbmodem110007001";
	std::vector<iDGraphData> id_graphs;
	std::vector<iiDGraphData> iid_graphs;
	std::vector<sGraphData> s_graphs;
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
	std::vector<std::string> ScanForAvailableBoards();
	void DrawPortAndBaudrateChooser();

	//void DrawTooltip();
	//void PlotHeatmap();

public:
	ArduPlot();
	void update() override;
};


float findMin(float arr[], int ArrSize);
float findMax(float arr[], int ArrSize);
