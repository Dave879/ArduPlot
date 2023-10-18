// SerialPort.cpp

#include "serialport.h"

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

    sfd = open(portPath, O_RDWR | O_NOCTTY);
    if (sfd == -1)
    {
        printf("Unable to open serial port: %s at baud rate: %d\n", portPath, baudRate);
        return sfd;
    }

    fcntl(sfd, F_SETFL, 0);

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

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | ECHONL);
    // options.c_lflag &= ~ICANON;
    // options.c_lflag &= ~ECHO;                                                        // Disable echo
    // options.c_lflag &= ~ECHOE;                                                       // Disable erasure
    // options.c_lflag &= ~ISIG;                                                        // Disable interpretation of INTR, QUIT and SUSP
    // options.c_lflag &= ~ECHONL;                                                      // Disable new-line echo
    options.c_iflag &= ~(IXON | IXOFF | IXANY);                                      // Turn off s/w flow ctrl
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Disable any special handling of received bytes

    // When the OPOST option is disabled, all other option bits in c_oflag are ignored.
    options.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    // options.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
    //  tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
    //  tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

    options.c_cc[VMIN] = 0; // VTIME becomes the overall time since read() gets called
    options.c_cc[VTIME] = 10; // Timeout of 1 second

    cfsetispeed(&options, get_baud(baudRate));
    cfsetospeed(&options, get_baud(baudRate));

    if (tcsetattr(sfd, TCSANOW, &options) != 0)
    {
        printf("Error setting serial port attributes.\n");
        close(sfd);
        return -2; // Using negative value; -1 used above for different failure
    }

    // Read in existing settings, and handle any error
    if (tcgetattr(sfd, &options) != 0)
    {
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
        return 1;
    }

    return sfd;
}

/**
 * If baudrate is not valid, returns B115200
 */
//
int get_baud(int baud)
{
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
}

bool serialPortIsOpen()
{
    return sfd != SFD_UNAVAILABLE;
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