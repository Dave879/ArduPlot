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
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x / 2);
	if (ImGui::BeginCombo("##usbdevcombo", current_item.c_str())) // The second parameter is the label previewed before opening the combo.
	{
		paths = ScanForAvailableBoards();
		for (int n = 0; n < paths.size(); n++)
		{
			bool is_selected = (current_item == paths.at(n)); // You can store your selection however you want, outside or inside your objects
			if (ImGui::Selectable(paths.at(n).c_str(), is_selected))
				current_item = paths.at(n);
			if (is_selected)
				ImGui::SetItemDefaultFocus(); // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

	if (ImGui::Button(!connected_to_device ? "Connect" : "Disconnect", ImGui::GetContentRegionAvail()))
	{
		if (!connected_to_device)
		{
			ConnectToUSB(current_item);
		}
		else
		{
			closeSerialPort();
			AP_LOG_g("Closed USB connection")
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

int USBInput::serialport_read_until(int fd, char *buf, char until, int buf_max, int timeout)
{
	char b[1]; // read expects an array, so we give it a 1-byte array
	int i = 0;
	do
	{
		int n = read(fd, b, 1); // read a char at a time
		if (n == -1)
			return -1; // couldn't read
		if (n == 0)
		{
			// usleep(1000); // wait 1000 usec and try again // whyyyyyyyy - Dave 14 nov 2022
			timeout--;
			if (timeout == 0)
				return -2;
			continue;
		}
#ifdef SERIALPORTDEBUG
		printf("serialport_read_until: i=%d, n=%d b='%c'\n", i, n, b[0]); // debug
#endif
		buf[i] = b[0];
		i++;
	} while (b[0] != until && i < buf_max && timeout > 0);

	buf[i] = '\0'; // null terminate the string
	return i;
}

uint32_t USBInput::Read(int fd, char *buf)
{ // TODO: make sure bytes_available < buffer size
	uint32_t bytes_available;
	ioctl(fd, FIONREAD, &bytes_available);
	int n = read(fd, buf, bytes_available); // read a char at a time
	if (n == -1)
		return 0;		// couldn't read
	buf[bytes_available + 1] = '\0'; // null terminate the string
	return bytes_available;
}

void USBInput::ConnectToUSB(std::string port)
{
	char data[500] = {0};
	int length;

	std::string s = "/dev/" + port;
	try
	{
		sfd = openAndConfigureSerialPort(s.c_str(), 115200); // Fake baudrate, need to implement it correctly for actual Arduino boards with serial to usb chip
		if (sfd > 0)
		{
			AP_LOG_g("Successfully connected to " << s << " Serial file descriptor: " << sfd)
		}
		else
		{
			AP_LOG_r("There was an error connecting to " << s)
		}
	}
	catch (const std::exception &e)
	{
		AP_LOG_r(e.what());
	}
	flushSerialData();
}

std::vector<std::string> USBInput::ScanForAvailableBoards()
{
	EnumSerial enumserial;
	return enumserial.EnumSerialPort(); // Enum device driver of serial port
}
