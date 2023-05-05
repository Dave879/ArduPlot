
#include "arduplot.h"

ArduPlot app;
int main()
{
    /*
    std::vector<std::string> paths = app.input_stream.ScanForAvailableBoards();
    AP_LOG(paths);
    app.input_stream.ConnectToUSB(paths.at(0));
    while (true)
    {
    app.update();
    }
    
    */
    app.run();
    return 0;
}