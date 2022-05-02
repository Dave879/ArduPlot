#include <Mahi/Gui.hpp>
#include <Mahi/Util.hpp>
#include <vector>
#include <string>
#include <enumserial.h>
#include <serialport.h>
#include <json.hpp>

#include "utils.h"

using namespace mahi::gui;
using namespace mahi::util;

class ArduPlot : public Application
{

private:
	std::vector<std::string> paths;
	const char *baudratelist[14] = {"110", "300", "600", "1200", "2400", "4800", "9600", "14400", "19200", "38400", "57600", "115200", "128000", "256000"};
	const char *current_baudrate = "115200";
	std::string current_item = "";
	unsigned char data[100] = {0};
	std::string strData = "";
	json JsonData;
	SerialPort serial;

	void DrawPlots(json j);
	void DrawPortAndBaudrateChooser();

public:
	ArduPlot();
	void update() override;
};
