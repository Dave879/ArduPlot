#include "usb_input.h"

USBInput::USBInput(GLFWwindow *w)
{
	this->window = w;
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
	if (std::find(paths.begin(), paths.end(), current_item) == paths.end())
	{
		// Device not connected anymore
		current_item = paths.size() == 0 ? "" : paths.at(0);
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
	else // Device still connected
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

	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (ImGui::CalcTextSize("40000000000").x + ImGui::GetStyle().FramePadding.y * 7.0f) - ImGui::GetFrameHeight());
	if (ImGui::BeginCombo("##usbdevcombo", current_item.c_str()))
	{
		for (long unsigned int n = 0; n < paths.size(); n++)
		{
			bool is_selected = (current_item == paths.at(n));
			if (ImGui::Selectable(paths.at(n).c_str(), is_selected))
			{
				std::string temp = current_item;
				current_item = paths.at(n);
				if (temp != paths.at(n))
				{
					closeSerialPort();
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

	/**
	 * USB Output
	 */
	// ImGui::SetNextWindowSize({500, 120}, ImGuiCond_::ImGuiCond_FirstUseEver);
	// ImGui::Begin("USB Output", nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollWithMouse);
	// ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Send").x - ImGui::GetStyle().FramePadding.y * 4.0f);
	// ImGui::InputText("##Output buffer", output_buf, OUTPUT_BUF_SIZE);
	// ImGui::SameLine();
	// if (ImGui::Button("Send") || glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
	// {
	// 	if (connected_to_device)
	// 	{
	// 		if (output_buf == "")
	// 		{
	// 			if (write(sfd, output_buf, OUTPUT_BUF_SIZE) > 0) // Successful write
	// 			{
	// 				AP_LOG_g("Successful write")
	// 			}
	// 		}

	// 		AP_LOG("Pressed key enter")
	// 	}
	// }
	// ImGui::End();
}

void USBInput::ConnectRoutine()
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
