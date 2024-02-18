#include "usb_input.h"

USBInput::USBInput(std::function<int()> send_callback) : send_command_callback(send_callback)
{
	ScanForAvailableBoards();
	if (paths.size() > 0)
		current_item = paths.at(0);
	else
		current_item = "";
}

USBInput::~USBInput()
{
}

void USBInput::DrawGUI()
{
	ImGui::Begin("USB Input");
	ScanForAvailableBoards();
	if (std::find(paths.begin(), paths.end(), current_item) == paths.end())
	{
		// Device not connected anymore
		if (connected_to_device)
		{
			CloseSerialPort();
			ScanForAvailableBoards();
		}
		current_item = paths.size() == 0 ? "" : paths.at(0);
	}
	else // Device still connected
	{
		for (uint8_t i = 0; i < paths.size(); i++)
		{
			if (paths.at(i) == last_item)
			{
				current_item = last_item;
				if (!connected_to_device && !pressed_disconnect && auto_connect)
				{
					if (Connect(current_item) != InputStatus::kOk)
					{
						// Do something
					}
				}
				break;
			}
			if (current_item == "")
			{
				current_item = paths.at(i);
			}
		}
	}

	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (ImGui::CalcTextSize("40000000000").x + ImGui::GetStyle().FramePadding.y * 7.0f) - ImGui::GetFrameHeight());
	if (ImGui::BeginCombo("##usbdevcombo", current_item.c_str()))
	{
		for (int n = 0; n < paths.size(); n++)
		{
			bool is_selected = (current_item == paths.at(n));

			if (ImGui::Selectable(paths.at(n).c_str(), is_selected))
			{
				std::string temp = current_item;
				current_item = paths.at(n);
				if (temp != paths.at(n))
				{
					CloseSerialPort();
					ConnectRoutine();
				}
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_Stationary))
		ImGui::SetTooltip("Device Port");

	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::CalcTextSize("40000000000").x + ImGui::GetStyle().FramePadding.y * 2.0f);
	if (ImGui::BeginCombo("##baudratecombo", current_baudrate))
	{
		for (int n = 0; n < IM_ARRAYSIZE(baudrate_list); n++)
		{
			bool is_selected = (current_baudrate == baudrate_list[n]);
			if (ImGui::Selectable(baudrate_list[n], is_selected))
			{
				current_baudrate = baudrate_list[n];
				if (connected_to_device)
				{
					AP_LOG_b("--- Baudrate changed, reopening connection automatically ---");
					CloseSerialPort();
					auto_connect = true;
				}
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_Stationary))
		ImGui::SetTooltip("Baudrate");

	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetFrameHeight());
	ImGui::Checkbox("##", &auto_connect);
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_Stationary))
		ImGui::SetTooltip("Auto-reconnect");

	if (ImGui::Button(!connected_to_device ? "Connect" : "Disconnect", ImGui::GetContentRegionAvail()))
	{
		if (!connected_to_device)
		{
			ConnectRoutine();
		}
		else
		{
			pressed_disconnect = true;
			CloseSerialPort();
			ScanForAvailableBoards();
			if (paths.size() > 0)
				current_item = paths.at(0);
			else
				current_item = "";
		}
	}
	ImGui::End();

	/**
	 * USB Output
	 */
	ImGui::SetNextWindowSize({500, 120}, ImGuiCond_::ImGuiCond_FirstUseEver);
	ImGui::Begin("USB Output", nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Send").x - ImGui::GetStyle().FramePadding.y * 4.0f);
	ImGui::InputText("##Output buffer", output_buf, OUTPUT_BUF_SIZE);
	static bool successful = false;
	// Change the color of the button if the message got sent successfully
	time_since_start += ImGui::GetIO().DeltaTime;
	static int counter = 0;
	if (successful)
	{
		ImGui::SetKeyboardFocusHere(-1);
		if (time > time_since_start)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 1.0f, 0.6f, time - time_since_start));		  // Green color
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 1.0f, 0.6f, time - time_since_start)); // Green color
			counter = 2;
		}
		else
		{
			successful = false;
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Send") || send_command_callback())
	{
		if (connected_to_device)
		{
			if (strlen(output_buf) > 0)
			{
				substituteInvisibleChars(output_buf);
#ifdef _WIN32
				if (WriteFile(hSerial, output_buf, strlen(output_buf), nullptr, nullptr))
#else
				if (write(sfd, output_buf, strlen(output_buf)) > 0)
#endif
				{
					successful = true;
					time = time_since_start + 0.2;
				}
				output_buf[0] = '\0';
			}
		}
	}

	ImGui::PopStyleColor(counter); // For ImGuiCol_ButtonHovered and ImGuiCol_Button
	counter = 0;

	ImGui::End();
}

void USBInput::ConnectRoutine()
{
	if (Connect(current_item) == InputStatus::kOk)
	{
		pressed_disconnect = false;

		// Auto-connect logic
		if (first_time) // To avoid "forcing" auto_connect to true every time "Connect" is pressed without proper settings file save
		{
			auto_connect = true;
			first_time = false;
		}
		last_item = current_item;
	}
	else
	{
		connected_to_device = false;
	}
}

InputStatus USBInput::GetData(std::string &data)
{
	if (connected_to_device)
	{
#ifdef _WIN32
		DWORD len = 0;
		if (!ReadFile(hSerial, input_buf, INPUT_BUF_SIZE, &len, nullptr))
		{
			CloseSerialPort();
			data = "";
			return InputStatus::kIOError;
		}
#else
		int len = read(sfd, input_buf, INPUT_BUF_SIZE);
		if (len == -1)
		{
			data = "";
			return InputStatus::kIOError;
		}
#endif
		// input_buf[n + 1] = '\0'; // null terminate the string
		data = std::string(input_buf, input_buf + len);
		return InputStatus::kOk;
	}
	return InputStatus::kGenericError;
}

bool USBInput::IsConnected()
{
	return connected_to_device;
}

InputStatus USBInput::Connect(const std::string &port)
{
	const std::string path = split(port, ' ').at(0);
	AP_LOG_g("Connecting to " << path << " with baudrate " << current_baudrate << "...");
	CloseSerialPort();

#ifdef _WIN32
	std::string path_used = strchr(path.c_str(), '\\') ? "" : "\\\\.\\" + path;
	hSerial = CreateFile(path_used.c_str(),
								GENERIC_READ | GENERIC_WRITE,
								0,
								0,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL,
								0);
	if (hSerial == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			AP_LOG_r("Serial port doesn't exist")
				 paths.clear();
			return InputStatus::kIOError;
			// serial port does not exist. Inform user.
		}
		AP_LOG_r("Error occured while connecting") return InputStatus::kIOError;
		// some other error occurred. Inform user.
	}

	DCB dcbSerialParams = {0};
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	if (!GetCommState(hSerial, &dcbSerialParams))
	{
		AP_LOG_r("Error getting dcbSerialParams")
			 // error getting state
			 return InputStatus::kIOError;
	}

	// Thanks putty
	dcbSerialParams.fBinary = true;
	dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
	dcbSerialParams.fDsrSensitivity = false;
	dcbSerialParams.fTXContinueOnXoff = false;
	dcbSerialParams.fOutX = false;
	dcbSerialParams.fInX = false;
	dcbSerialParams.fErrorChar = false;
	dcbSerialParams.fNull = false;
	dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE;
	dcbSerialParams.fAbortOnError = false;
	dcbSerialParams.fOutxCtsFlow = false;
	dcbSerialParams.fOutxDsrFlow = false;

	dcbSerialParams.BaudRate = GetBaud(std::stoi(current_baudrate));
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;
	if (!SetCommState(hSerial, &dcbSerialParams))
	{
		AP_LOG_r("Error setting dcbSerialParams")
			 // error setting serial port state
			 return InputStatus::kIOError;
	}

	COMMTIMEOUTS timeouts = {0};
	timeouts.ReadIntervalTimeout = 1;
	timeouts.ReadTotalTimeoutConstant = 1;
	timeouts.ReadTotalTimeoutMultiplier = 1;
	timeouts.WriteTotalTimeoutConstant = 1;
	timeouts.WriteTotalTimeoutMultiplier = 1;
	if (!SetCommTimeouts(hSerial, &timeouts))
	{
		AP_LOG_r("Error setting commtimeouts")
			 // error occureed. Inform user
			 return InputStatus::kIOError;
	}
#else
	sfd = open(path.c_str(), O_RDWR | O_NOCTTY);
	if (sfd == -1)
	{
		return InputStatus::kIOError;
	}

	fcntl(sfd, F_SETFL, 0);

	// Configure i/o baud rate settings
	struct termios options;

	// Read in existing settings, and handle any error
	if (tcgetattr(sfd, &options) != 0)
	{
		printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
		return InputStatus::kIOError;
	}

	options.c_cflag &= ~PARENB;		  // Clear parity bit, disabling parity (most common)
	options.c_cflag &= ~CSTOPB;		  // Clear stop field, only one stop bit used in communication (most common)
	options.c_cflag &= ~CSIZE;			  // Clear all bits that set the data size
	options.c_cflag |= CS8;				  // 8 bits per byte (most common)
	options.c_cflag &= ~CRTSCTS;		  // Disable RTS/CTS hardware flow control (most common)
	options.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

	// Disable echo, erasure, interpretation of INTR, QUIT and SUSP, new-line echo
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | ECHONL);

	options.c_iflag &= ~(IXON | IXOFF | IXANY);													// Turn off s/w flow ctrl
	options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Disable any special handling of received bytes

	// When the OPOST option is disabled, all other option bits in c_oflag are ignored.
	options.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)

	options.c_cc[VMIN] = 0;	  // VTIME becomes the overall time since read() gets called
	options.c_cc[VTIME] = 10; // Timeout of 1 second

	cfsetispeed(&options, GetBaud(std::stoi(current_baudrate)));
	cfsetospeed(&options, GetBaud(std::stoi(current_baudrate)));

	if (tcsetattr(sfd, TCSANOW, &options) != 0)
	{
		printf("Error setting serial port attributes.\n");
		close(sfd);
		return InputStatus::kIOError;
	}

	// Read in existing settings, and handle any error
	if (tcgetattr(sfd, &options) != 0)
	{
		printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
		return InputStatus::kIOError;
	}
#endif

	AP_LOG_g("Successfully connected to " << path);
	connected_to_device = true;
	return InputStatus::kOk;
}

void USBInput::CloseSerialPort()
{
	if (connected_to_device)
	{
		connected_to_device = false;
#ifdef _WIN32
		try
		{
			CloseHandle(hSerial);
		}
		catch (const std::exception &)
		{
			AP_LOG_r("Error closing handle, probably device got disconnected")
		}
		hSerial = nullptr;
#else
		close(sfd);
		sfd = SFD_UNAVAILABLE;
#endif
		AP_LOG_r("Closed connection")
	}
}

void USBInput::ScanForAvailableBoards()
{
	if (rescan_time > std::chrono::system_clock::now())
		return;

	rescan_time = std::chrono::system_clock::now() + std::chrono::seconds(1);
	paths.clear();

#ifdef _WIN32
	HKEY hSERIALCOMM = nullptr;
	int ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_READ, &hSERIALCOMM);
	if (ret == ERROR_SUCCESS)
	{
		long ris = 0;
		long index = 0;
		do
		{
			DWORD keyvaluesize = MAX_KEY_LENGTH;
			DWORD valuedatasize = MAX_VALUE_LENGTH;
			ris = RegEnumValue(hSERIALCOMM, index, keyvalue, &keyvaluesize, nullptr, nullptr, valuedata, &valuedatasize);
			if (ris == ERROR_SUCCESS)
			{
				paths.push_back(std::string(reinterpret_cast<char const *>(valuedata)) + " " + std::string(keyvalue));
			}
			index++;
		} while (ris != ERROR_NO_MORE_ITEMS);
	}

	RegCloseKey(hSERIALCOMM);
#else

	for (const std::filesystem::directory_entry &dir : std::filesystem::directory_iterator("/dev/"))
	{
		bool should_save = false;
		should_save |= dir.path().string().find("ACM") != std::string::npos;
		should_save |= dir.path().string().find("cu.usbmodem") != std::string::npos;
		should_save |= dir.path().string().find("ttyUSB") != std::string::npos;

		if (should_save)
		{
			std::string cmp;
			const char *r1, *r2;
			sd_device *dev = nullptr;
			std::string path = "/sys/class/tty/" + dir.path().string().substr(5);
			std::string tmp = "";
			int status = sd_device_new_from_syspath(&dev, path.c_str());
			if (status != 0)
				continue;

			status = sd_device_get_property_value(dev, "ID_VENDOR", &r1);
			if (status != 0)
				continue;

			status = sd_device_get_property_value(dev, "ID_MODEL", &r2);
			if (status != 0)
				continue;
			cmp = dir.path().string() + " " + std::string(r1) + " " + std::string(r2);

			paths.push_back(cmp);
		}
	}
#endif
}

/**
 * If baudrate is not valid, returns B115200
 */
int USBInput::GetBaud(const unsigned int baud)
{
#ifdef _WIN32
	// This switch case is overkill, could probably just do: return baud; and trust that baud is a valid baudrate
	switch (baud)
	{
	case 110:
		return CBR_110;
	case 300:
		return CBR_300;
	case 600:
		return CBR_600;
	case 1200:
		return CBR_1200;
	case 2400:
		return CBR_2400;
	case 4800:
		return CBR_4800;
	case 9600:
		return CBR_9600;
	case 19200:
		return CBR_19200;
	case 38400:
		return CBR_38400;
	case 57600:
		return CBR_57600;
	case 115200:
		return CBR_115200;
	default:
		return CBR_115200;
	}
#else
	switch (baud)
	{
	case 110:
		return B110;
	case 300:
		return B300;
	case 600:
		return B600;
	case 1200:
		return B1200;
	case 2400:
		return B2400;
	case 4800:
		return B4800;
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	case 460800:
#ifndef __APPLE__
		return B460800;
	case 500000:
		return B500000;
	case 576000:
		return B576000;
	case 921600:
		return B921600;
	case 1000000:
		return B1000000;
	case 1152000:
		return B1152000;
	case 1500000:
		return B1500000;
	case 2000000:
		return B2000000;
	case 2500000:
		return B2500000;
	case 3000000:
		return B3000000;
	case 3500000:
		return B3500000;
	case 4000000:
		return B4000000;
#endif
	default:
		return B115200;
	}
#endif
}
