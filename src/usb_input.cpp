#include "usb_input.h"

USBInput::USBInput()
{
	paths = ScanForAvailableBoards();
	if (paths.size() > 0)
		current_item = paths.at(0);
	else
		current_item = "";
}

USBInput::~USBInput()
{
}

void USBInput::DrawDataInputPanel()
{
	ImGui::Begin("USB input");
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	paths = ScanForAvailableBoards();
	if (paths.size() == 0)
	{
		current_item = "";
		if (connected_to_device)
		{
			closeSerialPort();
			AP_LOG_g("Closed USB connection");
			paths = ScanForAvailableBoards();
			if (paths.size() > 0)
				current_item = paths.at(0);
			else
				current_item = "";
			connected_to_device = false;
		}
	}
	else
	{
		for (uint8_t i = 0; i < paths.size(); i++)
		{
			if (paths.at(i) == last_item)
			{
				current_item = last_item;
				if (!connected_to_device && !pressed_disconnect)
				{
					if (ConnectToUSB(current_item) == 0)
					{
						connected_to_device = true;
					}
					else
					{
						connected_to_device = false;
					}
				}
				break;
			}
		}
	}
	if (ImGui::BeginCombo("##usbdevcombo", current_item.c_str())) // The second parameter is the label previewed before opening the combo.
	{
		for (long unsigned int n = 0; n < paths.size(); n++)
		{
			bool is_selected = (current_item == paths.at(n)); // You can store your selection however you want, outside or inside your objects
			if (ImGui::Selectable(paths.at(n).c_str(), is_selected))
			{
				current_item = paths.at(n);
				last_item = current_item;
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus(); // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
		}
		ImGui::EndCombo();
	}
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

	if (ImGui::Button(!connected_to_device ? "Connect" : "Disconnect", ImGui::GetContentRegionAvail()))
	{
		if (!connected_to_device)
		{
			pressed_disconnect = false;
			if (ConnectToUSB(current_item) == 0)
			{
				connected_to_device = false;
			}
			else
			{
				connected_to_device = true;
			}
		}
		else
		{
			pressed_disconnect = true;
			closeSerialPort();
			AP_LOG_g("Closed USB connection");
			paths = ScanForAvailableBoards();
			if (paths.size() > 0)
				current_item = paths.at(0);
			else
				current_item = "";
		}
		connected_to_device = !connected_to_device;
	}
	ImGui::End();
}
/* Alternative GetData();
std::string USBInput::GetData()
{
	if (connected_to_device)
	{
		length = serialport_read_until(sfd, data, '\n', 500, 100);
		if (length >= 0)
		{
			return std::string(data, data + length);
		}
		else
		{
			return "";
		}
	}
	return "";
}

*/

std::string USBInput::GetData()
{
	if (connected_to_device)
	{
		uint32_t len = Read(sfd, data);
		return std::string(data, data + len);
	}
	return "";
}

uint32_t USBInput::Read(int fd, char *buf)
{
	int32_t n = read(fd, buf, CHAR_BUF_SIZE);
	if (n == -1)
	{
		return 0; // couldn't read
	}
	buf[n + 1] = '\0'; // null terminate the string
	return n;
}

bool USBInput::IsConnected()
{
	return connected_to_device;
}

uint8_t USBInput::ConnectToUSB(std::string port)
{
	std::string s = "/dev/" + port;
	if (s != "/dev/")
	{
		try
		{
			AP_LOG_g("Connecting to " << s << "...");
			sfd = openAndConfigureSerialPort(s.c_str(), 115200); // Fake baudrate, need to implement it correctly for actual Arduino boards with serial to usb chip
			if (sfd > 0)
			{
				AP_LOG_g("Successfully connected to " << s << " Serial file descriptor: " << sfd);
				return 0;
			}
			else
			{
				AP_LOG_r("There was an error connecting to " << s);
				return 1;
			}
		}
		catch (const std::exception &e)
		{
			AP_LOG_r(e.what());
			return 1;
		}
	}
	else
	{
		return 1;
	}
}

std::vector<std::string> USBInput::ScanForAvailableBoards()
{
	EnumSerial enumserial;
	return enumserial.EnumSerialPort(); // Enum device driver of serial port
}
