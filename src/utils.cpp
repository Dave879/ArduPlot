
#include <enumserial.h>

#include "utils.h"

std::vector<std::string> ScanForAvailableBoards()
{
    EnumSerial enumserial;
    return enumserial.EnumSerialPort(); // Enum device driver of serial port
}

template <typename T>
void NewPlot(std::string name, ScrollingBuffer sdata1, T anyData){
        ImGui::Begin(name.c_str());
        static float t = 0;
        t += ImGui::GetIO().DeltaTime;
        sdata1.AddPoint(t, anyData);

        static float history = 10.0f;
        ImGui::SliderFloat("History", &history, 1, 30, "%.1f s");

        static ImPlotAxisFlags rt_axis = ImPlotAxisFlags_None;
        ImPlot::SetNextPlotLimitsX(t - history, t, ImGuiCond_Always);
        ImPlot::SetNextPlotLimitsY(-10, 1100, ImGuiCond_Always);
        if (ImPlot::BeginPlot(name.c_str(), NULL, NULL, ImVec2(-1, -1), 0, rt_axis, rt_axis))
        {
            ImPlot::PlotLine("A0", &sdata1.Data[0].x, &sdata1.Data[0].y, sdata1.Data.size(), sdata1.Offset, 2 * sizeof(float));
            // ImPlot::PlotLine("Mouse y", &sdata2.Data[0].x, &sdata2.Data[0].y, sdata2.Data.size(), sdata2.Offset, 2 * sizeof(float));
            ImPlot::EndPlot();
        }
}
