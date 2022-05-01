#ifndef __SERIAL_PORT__
#define __SERIAL_PORT__

#include <iostream>
#include <string> 

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cerrno>

#ifdef _WIN_

#include <windows.h>
#include <tchar.h>
#elif __APPLE__

#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>

#endif

class SerialPort
{
   public:

      SerialPort();
      SerialPort(const char*,int,int,int,int);
      ~SerialPort();
#ifdef _WIN_
      bool Open(const std::wstring);
#elif __APPLE__
      bool Open(const char*);
#endif
      bool SetBaudRate(int);
      bool SetParity(int,int,int);
      void Close();
      void Write(unsigned char*,int);
      void Read(unsigned char*,int);
      int Read(unsigned char*);

   private:

      const char *path;
#ifdef _WIN_
      HANDLE com;
      DCB dcb;
      std::wstring com_path;
      COMMTIMEOUTS timeouts;
#elif __APPLE__
      int device;
#endif
      int baud_rate;
      int databits;
      int stopbits;
      int parity;
};

#endif
