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
				if (!connected_to_device && !pressed_disconnect && auto_connect)
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
			if (current_item == "")
			{
				current_item = paths.at(i);
			}
		}
	}
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 117);
	if (ImGui::BeginCombo("##usbdevcombo", current_item.c_str()))
	{
		for (long unsigned int n = 0; n < paths.size(); n++)
		{
			bool is_selected = (current_item == paths.at(n));
			if (ImGui::Selectable(paths.at(n).c_str(), is_selected))
			{
				current_item = paths.at(n);
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_Stationary))
		ImGui::SetTooltip("Device Port");

	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
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
					closeSerialPort();
					connected_to_device = false;
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
	ImGui::SetNextItemWidth(30);
	ImGui::Checkbox("", &auto_connect);
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_Stationary))
		ImGui::SetTooltip("Auto-reconnect");

	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	if (ImGui::Button(!connected_to_device ? "Connect" : "Disconnect", ImGui::GetContentRegionAvail()))
	{
		if (!connected_to_device)
		{
			if (ConnectToUSB(current_item) == 0)
			{
				connected_to_device = true;
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
		else
		{
			pressed_disconnect = true;
			closeSerialPort();
			connected_to_device = false;
			AP_LOG_g("Closed USB connection");
			paths = ScanForAvailableBoards();
			if (paths.size() > 0)
				current_item = paths.at(0);
			else
				current_item = "";
		}
	}
	ImGui::End();
}

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

/**
 * @returns 0 on successful connection, 1 if an error occured
 */
uint8_t USBInput::ConnectToUSB(std::string port)
{
	std::string s = "/dev/" + port;
	if (s != "/dev/")
	{
		try
		{
			AP_LOG_g("Connecting to " << s << " with baudrate " << std::stoi(current_baudrate) << "...");
			sfd = openAndConfigureSerialPort(s.c_str(), std::stoi(current_baudrate));
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
