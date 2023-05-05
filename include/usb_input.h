#pragma once
#include <vector>
#include <Mahi/Util.hpp>
#include <enumserial.h>
#include <serialport.h>
#include <sys/ioctl.h>
#include <stdio.h>

#include "utilities.h"

#define CHAR_BUF_SIZE 1000000

class USBInput
{
private:
	// const char *baudrate_list[14] = {"110", "300", "600", "1200", "2400", "4800", "9600", "14400", "19200", "38400", "57600", "115200", "128000", "256000"};
	// const char *current_baudrate = "115200";

	char data[CHAR_BUF_SIZE] = {0};
	int length;
	std::vector<std::string> paths;
	std::string current_item = "";
	std::string last_item = "";
	bool pressed_disconnect = false;
	bool connected_to_device = false;
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
