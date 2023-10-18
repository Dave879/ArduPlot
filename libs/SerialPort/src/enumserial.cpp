#include "enumserial.h"

EnumSerial::EnumSerial()
{
}

std::vector<std::string> EnumSerial::EnumSerialPort()
{
   std::vector<std::string> paths;
#ifdef _WIN_
   // search com

   if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hkey))
   {
      unsigned long index = 0;
      long return_value;

      while ((return_value = RegEnumValue(hkey, index, key_valuename, &len_valuename, 0,
                                          &key_type, (LPBYTE)key_valuedata, &len_valuedata)) == ERROR_SUCCESS)
      {
         if (key_type == REG_SZ)
         {
            std::wstring key_valuedata_ws(key_valuedata);
            std::string key_valuedata_str(key_valuedata_ws.begin(), key_valuedata_ws.end());
            paths.push_back(key_valuedata_str);
         }
         len_valuename = 1000;
         len_valuedata = 1000;
         index++;
      }
   }
   // close key
   RegCloseKey(hkey);

#else
   for (const std::filesystem::directory_entry &dir : std::filesystem::directory_iterator("/dev/")){
      if (dir.path().string().find("ACM") != std::string::npos || dir.path().string().find("cu.usbmodem") != std::string::npos || dir.path().string().find("ttyUSB") != std::string::npos)
         paths.push_back(dir.path().string().substr(5));
   }
#endif

   return paths;
}
