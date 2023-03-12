#include "usb_input.h"

USBInput::USBInput()
{
}

USBInput::~USBInput()
{
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

void USBInput::ConnectAndReadFromSerial(bool &join_read_thread, bool &connected, mahi::util::Lockable &lockable, std::string port, std::string baudrate, std::string &USB_data, bool &new_data)
{
	char data[500] = {0};
	int length;

	std::string s = "/dev/" + port;
	int sfd = 0;
	try
	{
		sfd = openAndConfigureSerialPort(s.c_str(), std::stoi(baudrate));
		if (sfd > 0)
		{
			AP_LOG_g("Successfully connected to " << s << " Serial file descriptor: " << sfd)
				 connected = true;
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
	// flushSerialData();

	uint64_t bit_sum_in_one_second = 0;
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	while (!join_read_thread)
	{
		length = serialport_read_until(sfd, data, '\n', 500, 100);
		if (length >= 0)
		{
			mahi::util::Lock lock(lockable);
			new_data = true;
			USB_data = std::string(data, data + length);
			end = std::chrono::steady_clock::now();
			if (std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() > 1000) // 1s
			{
				float mb_s = bit_sum_in_one_second * 0.000001f;
				AP_LOG_r(bit_sum_in_one_second << "b/s = " << mb_s << "Mb/s")
					 begin = std::chrono::steady_clock::now();
				bit_sum_in_one_second = 0;
			}
			else
			{
				bit_sum_in_one_second += length * 8;
			}
		}
		new_data = false;
	}
	closeSerialPort();
}

std::vector<std::string> USBInput::ScanForAvailableBoards()
{
	EnumSerial enumserial;
	return enumserial.EnumSerialPort(); // Enum device driver of serial port
}
