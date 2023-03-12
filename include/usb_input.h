#include <vector>
#include <Mahi/Util.hpp>
#include <enumserial.h>
#include <serialport.h>

#include "utilities.h"
#include "input_stream.h"

class USBInput : public InputStream
{
private:
public:
	USBInput();
	~USBInput();
	static int serialport_read_until(int fd, char *buf, char until, int buf_max, int timeout);
	static void ConnectAndReadFromSerial(bool &join_read_thread, bool &connected, mahi::util::Lockable &lockable, std::string port, std::string baudrate, std::string &USB_data, bool &new_data);
	static std::vector<std::string> ScanForAvailableBoards();
};
