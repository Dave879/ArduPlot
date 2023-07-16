#pragma once
#include <vector>
#include <enumserial.h>
#include <serialport.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <atomic>

#include <chrono>
#include <thread>

#include "utilities.h"

#define CHAR_BUF_SIZE 1000000

class USBInput
{
private:
	#ifdef __APPLE__
	const char *baudrate_list[12] = {"110", "300", "600", "1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200", "230400"};
	#else
	const char *baudrate_list[24] = {"110", "300", "600", "1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200", "230400", "460800", "500000", "576000", "921600", "1000000", "1152000", "1500000", "2000000", "2500000", "3000000", "3500000", "4000000"};
	#endif
	const char *current_baudrate = "115200";

	bool auto_connect = false;

	bool first_time = true;

	char data[CHAR_BUF_SIZE] = {0};
	int length;
	std::vector<std::string> paths;
	std::string current_item = "";
	std::string last_item = "";
	bool pressed_disconnect = false;
	std::atomic<bool> connected_to_device = false;
	int sfd = 0;

public:
	USBInput();
	~USBInput();
	void DrawDataInputPanel();
	uint32_t Read(int fd, char *buf);
	std::string GetData();
	uint8_t ConnectToUSB(std::string port);
	bool IsConnected();

	static std::vector<std::string> ScanForAvailableBoards();
};
