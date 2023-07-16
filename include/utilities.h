#pragma once

#include <vector>
#include <string>
#include <imgui.h>
#include <implot.h>

// #define AP_LOG(x) std::cout <<  __FILE__  << "(" << __LINE__ << ") " << x << std::endl;

const std::string red("\033[0;31m");
const std::string green("\033[0;32m");
const std::string blue("\033[0;34m");
const std::string reset("\033[0m");

#define AP_LOG(x) std::cout << x << std::endl;
#define AP_LOG_r(x) std::cout << red << x << reset << std::endl;
#define AP_LOG_g(x) std::cout << green << x << reset << std::endl;
#define AP_LOG_b(x) std::cout << blue << x << reset << std::endl;

#define LOGERR(x) AP_LOG_r(x)

#define START_TIMER std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
#define END_TIMER                                                                \
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now(); \
	std::cout << "ΔT = " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() << "[ns] " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs] " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;
#define END_TIMER_MICROSECONDS                                                   \
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now(); \
	std::cout << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << std::endl;

// utility structure for realtime plot
struct ScrollingBuffer
{
	int MaxSize;
	int Offset;
	ImVector<ImVec2> Data;

	ScrollingBuffer(int max_size = 100000);
	void AddPoint(float x, float y);
	void Erase();
};

enum GraphType
{
	LINE,
	BAR,
	HEATMAP
};

struct iDGraphData
{
	int32_t merge_ID = 0;
	bool is_parent = false;
	bool is_child = false;

	float history = 20.0f;
	std::string graphName = "";
	GraphType type;
	ScrollingBuffer buffer;
	int64_t min = 0, max = 0;
	bool has_set_min_max = false;

	iDGraphData(std::string name = "Default", GraphType type = GraphType::LINE)
	{
		this->graphName = name;
		this->type = type;
	}
};

struct ScatterPlotData{
	std::string graphName = "";
	std::vector<ImPlotPoint> data;

	ScatterPlotData(std::string name = "Default"){
		this->graphName = name;
	}
};

struct iiDGraphData
{
	std::string graphName = "";
	std::vector<double> buffer;
	uint64_t sizex, sizey;
	float min = 0, max = 0;
	bool has_set_min_max = false;

	iiDGraphData(std::string name = "Default")
	{
		this->graphName = name;
	}
};

struct sGraphData
{
	std::string graphName = "";
	ScrollingBuffer buffer;
};

int32_t FindInVec(const std::vector<std::string> &vec, const std::string &s);
