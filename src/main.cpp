#include <Mahi/Gui.hpp>
#include <Mahi/Util.hpp>
#include <vector>
#include <string>
#include <enumserial.h>
#include <serialport.h>

using namespace mahi::gui;
using namespace mahi::util;

std::vector<std::string> paths;
const char *baudratelist[] = {"110", "300", "600", "1200", "2400", "4800", "9600", "14400", "19200", "38400", "57600", "115200", "128000", "256000"};
const char *current_baudrate = "115200";
std::string current_item = "";
std::string current_buffer = "A";
unsigned char data[100] = {0};
std::string strData = "";
int arduData = -1;
SerialPort serial;

std::vector<std::string> ScanForAvailableBoards()
{
    EnumSerial enumserial;
    return enumserial.EnumSerialPort(); // Enum device driver of serial port
}

// utility structure for realtime plot
struct ScrollingBuffer
{
    int MaxSize;
    int Offset;
    ImVector<ImVec2> Data;
    ScrollingBuffer(int max_size = 2000)
    {
        MaxSize = max_size;
        Offset = 0;
        Data.reserve(MaxSize);
    }
    void AddPoint(float x, float y)
    {
        if (Data.size() < MaxSize)
            Data.push_back(ImVec2(x, y));
        else
        {
            Data[Offset] = ImVec2(x, y);
            Offset = (Offset + 1) % MaxSize;
        }
    }
    void Erase()
    {
        if (Data.size() > 0)
        {
            Data.shrink(0);
            Offset = 0;
        }
    }
};

class MyApp : public Application
{
public:
    MyApp() : Application(500, 300, "ArduPlot")
    {
        ImGui::GetIO().ConfigFlags &= !ImGuiConfigFlags_ViewportsEnable;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }
    void update() override
    {
        ImGui::DockSpaceOverViewport();

        ImGui::Begin("Plot A0");
        static ScrollingBuffer sdata1;
        static float t = 0;
        t += ImGui::GetIO().DeltaTime;
        sdata1.AddPoint(t, arduData);

        static float history = 10.0f;
        ImGui::SliderFloat("History", &history, 1, 30, "%.1f s");

        static ImPlotAxisFlags rt_axis = ImPlotAxisFlags_None;
        ImPlot::SetNextPlotLimitsX(t - history, t, ImGuiCond_Always);
        ImPlot::SetNextPlotLimitsY(-10, 1100, ImGuiCond_Always);
        if (ImPlot::BeginPlot("##Scrolling", NULL, NULL, ImVec2(-1, -1), 0, rt_axis, rt_axis))
        {
            ImPlot::PlotLine("A0", &sdata1.Data[0].x, &sdata1.Data[0].y, sdata1.Data.size(), sdata1.Offset, 2 * sizeof(float));
            // ImPlot::PlotLine("Mouse y", &sdata2.Data[0].x, &sdata2.Data[0].y, sdata2.Data.size(), sdata2.Offset, 2 * sizeof(float));
            ImPlot::EndPlot();
        }

        ImGui::End();

        ImGui::Begin("Port - Baudrate chooser");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x / 2);
        if (ImGui::BeginCombo("##usbcombo", current_item.c_str())) // The second parameter is the label previewed before opening the combo.
        {
            paths = ScanForAvailableBoards();
            for (int n = 0; n < paths.size(); n++)
            {
                bool is_selected = (current_item == paths.at(n)); // You can store your selection however you want, outside or inside your objects
                if (ImGui::Selectable(paths.at(n).c_str(), is_selected))
                    current_item = paths.at(n);
                if (is_selected)
                    ImGui::SetItemDefaultFocus(); // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::BeginCombo("##baudratecombo", current_baudrate)) // The second parameter is the label previewed before opening the combo.
        {
            for (int n = 0; n < IM_ARRAYSIZE(baudratelist); n++)
            {
                bool is_selected = (current_baudrate == baudratelist[n]); // You can store your selection however you want, outside or inside your objects
                if (ImGui::Selectable(baudratelist[n], is_selected))
                    current_baudrate = baudratelist[n];
                if (is_selected)
                    ImGui::SetItemDefaultFocus(); // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Connect", ImGui::GetContentRegionAvail()))
        {

#ifdef _WIN_
            serial.Open(paths[choice_path]);
#elif __APPLE__
            std::string s = "/dev/" + current_item;
            try
            {
                bool res = serial.Open(s.c_str());
                if (res)
                    std::cout << "Successfully connected to " << s << std::endl;
                else
                    std::cout << "There was an error connecting to " << s << std::endl;
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }

#endif
            serial.SetBaudRate(std::stoi(current_baudrate));
            serial.SetParity(8, 1, 'N');
        }
        // unsigned char str[17] = "Hello Terminal \n";
        // serial.Write(str, 17);

        ImGui::End();

        if (serial.Read(data) != -1)
        {
            strData = std::string(data, data + 100);
            arduData = std::atoi(strData.c_str());
        }
    }
};

int main()
{
    MyApp app;
    app.run();
    return 0;
}