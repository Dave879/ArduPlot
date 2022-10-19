// SerialPort.cpp

#include "serialport.h"

#include <chrono>
#include <fcntl.h> //open()
#include <stdio.h>
#include <termios.h>
#include <unistd.h> //write(), read(), close()
#include <errno.h>  //errno
#include <cstring>

using namespace std::chrono;

// Serial port file descriptor
static const int SFD_UNAVAILABLE = -1;
static int sfd = SFD_UNAVAILABLE;

int openAndConfigureSerialPort(const char *portPath, int baudRate)
{

   // If port is already open, close it
   if (serialPortIsOpen())
   {
      close(sfd);
   }

   // Open port, checking for errors

   sfd = open(portPath, O_RDWR);
   if (sfd == -1)
   {
      printf("Unable to open serial port: %s at baud rate: %d\n", portPath, baudRate);
      return sfd;
   }

   // Configure i/o baud rate settings
   struct termios options;

   // Read in existing settings, and handle any error
   if (tcgetattr(sfd, &options) != 0)
   {
      printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
      return 1;
   }

   // Configure other settings
   // Settings from:
   //   https://github.com/Marzac/rs232/blob/master/rs232-linux.c
   //
   /*
      options.c_iflag &= ~(INLCR | ICRNL);
      options.c_iflag |= IGNPAR | IGNBRK;
      options.c_oflag &= ~(OPOST | ONLCR | OCRNL);
      options.c_cflag &= ~(PARENB | PARODD | CSTOPB | CSIZE | CRTSCTS);
      options.c_cflag |= CLOCAL | CREAD | CS8;
      options.c_lflag &= ~(ICANON | ISIG | ECHO);
      options.c_cc[VTIME] = 1;
      options.c_cc[VMIN] = 0;
   */

   options.c_cflag &= ~PARENB;        // Clear parity bit, disabling parity (most common)
   options.c_cflag &= ~CSTOPB;        // Clear stop field, only one stop bit used in communication (most common)
   options.c_cflag &= ~CSIZE;         // Clear all bits that set the data size
   options.c_cflag |= CS8;            // 8 bits per byte (most common)
   options.c_cflag &= ~CRTSCTS;       // Disable RTS/CTS hardware flow control (most common)
   options.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

   options.c_lflag &= ~ICANON;
   options.c_lflag &= ~ECHO;                                                        // Disable echo
   options.c_lflag &= ~ECHOE;                                                       // Disable erasure
   options.c_lflag &= ~ECHONL;                                                      // Disable new-line echo
   options.c_lflag &= ~ISIG;                                                        // Disable interpretation of INTR, QUIT and SUSP
   options.c_iflag &= ~(IXON | IXOFF | IXANY);                                      // Turn off s/w flow ctrl
   options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Disable any special handling of received bytes

   options.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
   options.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
                          // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
                          // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

   cfsetispeed(&options, baudRate);
   cfsetospeed(&options, baudRate);

   // Apply settings
   // TCSANOW vs TCSAFLUSH? Was using TCSAFLUSH; settings source above
   // uses TCSANOW.
   if (tcsetattr(sfd, TCSANOW, &options) != 0)
   {
      printf("Error setting serial port attributes.\n");
      close(sfd);
      return -2; // Using negative value; -1 used above for different failure
   }

   return sfd;
}

bool serialPortIsOpen()
{
   return sfd != SFD_UNAVAILABLE;
}

milliseconds getSteadyClockTimestampMs()
{
   return duration_cast<milliseconds>(steady_clock::now().time_since_epoch());
}

ssize_t flushSerialData()
{

   // For some reason, setting this too high can cause the serial port to not start again properly...
   float flushDurationMs = 150.0f;

   ssize_t result = 0;
   milliseconds startTimestampMs = getSteadyClockTimestampMs();
   while (getSteadyClockTimestampMs().count() - startTimestampMs.count() < flushDurationMs)
   {
      char buffer[1];
      result = readSerialData(buffer, 1);
      if (result < 0)
      {
         printf("readSerialData() failed. Error: %s\n", strerror(errno));
      }
   };

   return result;
}

// Returns -1 on failure, with errno set appropriately
ssize_t writeSerialData(const char *bytesToWrite, size_t numBytesToWrite)
{

   ssize_t numBytesWritten = write(sfd, bytesToWrite, numBytesToWrite);
   if (numBytesWritten < 0)
   {
      printf("Serial port write() failed. Error: %s", strerror(errno));
   }

   return numBytesWritten;
}

// Returns -1 on failure, with errno set appropriately
ssize_t readSerialData(char *const rxBuffer, size_t numBytesToReceive)
{

   ssize_t numBytesRead = read(sfd, rxBuffer, numBytesToReceive);
   if (numBytesRead < 0)
   {
      printf("Serial port read() failed. Error: %s\n", strerror(errno));
   }

   return numBytesRead;
}

ssize_t closeSerialPort(void)
{
   ssize_t result = 0;
   if (serialPortIsOpen())
   {
      result = close(sfd);
      sfd = SFD_UNAVAILABLE;
   }
   return result;
}

int getSerialFileDescriptor(void)
{
   return sfd;
}