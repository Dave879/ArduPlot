#pragma once

#include <vector>
#include <algorithm> // For std::find
#include <atomic>
#include <chrono>
#include <functional>
#include <filesystem>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <malloc.h>
#include <tchar.h>
#include <string.h>
#else
#include <fcntl.h> //open()
#include <termios.h>
#include <unistd.h> //write(), read(), close()
#endif

#ifdef __linux__
#include <systemd/sd-device.h>
#endif

#include "utilities.h"
#include "string_utils.h"
#include "input_stream.h"

#define INPUT_BUF_SIZE 4096 // Size of linux serial buffer

#define OUTPUT_BUF_SIZE 2048 // Arbitrary value

class USBInput : public InputStream
{
private:
#ifdef _WIN32
	const char *baudrate_list[11] = {"110", "300", "600", "1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200"};
#elif __APPLE__
	const char *baudrate_list[12] = {"110", "300", "600", "1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200", "230400"};
#else
	const char *baudrate_list[24] = {"110", "300", "600", "1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200", "230400", "460800", "500000", "576000", "921600", "1000000", "1152000", "1500000", "2000000", "2500000", "3000000", "3500000", "4000000"};
#endif
	const char *current_baudrate = "115200";

#ifdef _WIN32
#define MAX_KEY_LENGTH 255
#define MAX_VALUE_LENGTH 16383
	HANDLE hSerial;
	char keyvalue[MAX_KEY_LENGTH];
	unsigned char valuedata[MAX_VALUE_LENGTH];
#else
	static const int SFD_UNAVAILABLE = -1;
	int sfd = SFD_UNAVAILABLE;
#endif

	char output_buf[OUTPUT_BUF_SIZE] = {0};

	bool auto_connect = false;
	bool first_time = true;

	char input_buf[INPUT_BUF_SIZE] = {0};
	int length;
	std::vector<std::string> paths;
	std::string current_item = "";
	std::string last_item = "";
	bool pressed_disconnect = false;
	std::atomic<bool> connected_to_device = false;

	void ConnectRoutine();
	InputStatus Connect(const std::string &port);

	std::chrono::system_clock::time_point rescan_time = std::chrono::system_clock::now();
	void ScanForAvailableBoards();

	static int GetBaud(const unsigned int baud);

	void CloseSerialPort();

	double time = ImGui::GetIO().DeltaTime;
	double time_since_start = ImGui::GetIO().DeltaTime;

	std::function<int()> send_command_callback;

public:
	USBInput(std::function<int()> send_callback = []()
				{ return 0; });
	~USBInput();
	void DrawGUI() override;
	InputStatus GetData(std::string &data) override;
	bool IsConnected() override;
};
