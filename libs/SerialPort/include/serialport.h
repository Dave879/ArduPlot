// SerialPort.hpp

#ifndef __SERIAL_PORT_H__
#define __SERIAL_PORT_H__

#include <unistd.h> //ssize_t
#include <fcntl.h> //open()
#include <stdio.h>
#include <termios.h>
#include <unistd.h> //write(), read(), close()
#include <errno.h>  //errno
#include <cstring>


int openAndConfigureSerialPort(const char *portPath, int baudRate);

int get_baud(int baud);

bool serialPortIsOpen();

ssize_t flushSerialData();

ssize_t writeSerialData(const char *bytes, size_t length);

ssize_t readSerialData(char *bytes, size_t length);

ssize_t closeSerialPort(void);

int getSerialFileDescriptor(void);

#endif //__SERIAL_PORT_H__