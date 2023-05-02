
#include "arduplot.h"

ArduPlot app;
int main()
{
    std::vector<std::string> paths = app.input_stream.ScanForAvailableBoards();
    AP_LOG(paths);
    app.input_stream.ConnectToUSB("ttyACM0");
    while (true)
    {
    app.update();
    }
    
    return 0;
}