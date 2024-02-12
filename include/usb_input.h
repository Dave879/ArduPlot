#pragma once

#include <vector>
#include <algorithm> // For std::find
#include <atomic>

#ifdef _WIN_
#include <atlbase.h>
#include <windows.h>
#include <tchar.h>
#include "dirent.h"

#else
#include <filesystem>
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
#ifdef __APPLE__
	const char *baudrate_list[12] = {"110", "300", "600", "1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200", "230400"};
#else
	const char *baudrate_list[24] = {"110", "300", "600", "1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200", "230400", "460800", "500000", "576000", "921600", "1000000", "1152000", "1500000", "2000000", "2500000", "3000000", "3500000", "4000000"};
#endif
	const char *current_baudrate = "115200";

	static const int SFD_UNAVAILABLE = -1;

	int sfd = SFD_UNAVAILABLE;

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
	uint32_t Read(int fd, char *buf);
	uint8_t ConnectToUSB(const std::string &port);

	std::chrono::system_clock::time_point rescan_time = std::chrono::system_clock::now();
	void ScanForAvailableBoards();

	int OpenAndConfigureSerialPort(const char *portPath, int baudRate);

	static int GetBaud(int baud);

	bool SerialPortIsOpen();

	int CloseSerialPort();

	double time = ImGui::GetIO().DeltaTime;
	double time_since_start = ImGui::GetIO().DeltaTime;

	std::function<int()> send_command_callback;

public:
	USBInput(std::function<int()> send_callback = []()
				{ return 0; });
	~USBInput();
	void DrawGUI() override;
	std::string GetData() override;
	bool IsConnected() override;
};
