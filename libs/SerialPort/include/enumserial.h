#ifndef __ENUM_SERIAL__
#define __ENUM_SERIAL__

#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN_
#include <atlbase.h>
#include <windows.h>
#include <tchar.h>
#include "dirent.h"

#elif __APPLE__
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#elif __linux__
#include <filesystem>
#endif

class EnumSerial
{
public:
   EnumSerial();
   std::vector<std::string> EnumSerialPort();
#ifdef __APPLE__
   std::string GetRoot()
   {
      return root;
   }
#endif

private:
#ifdef _WIN_
   TCHAR key_valuename[1000];
   unsigned long len_valuename = 1000;
   wchar_t key_valuedata[1000];
   unsigned long len_valuedata = 1000;
   unsigned long key_type;
   HKEY hkey = NULL;
#elif __APPLE__
   std::string root;
#endif
};

#endif
