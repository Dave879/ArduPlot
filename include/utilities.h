#pragma once

#include <vector>
#include <Mahi/Gui.hpp>

//#define AP_LOG(x) std::cout <<  __FILE__  << "(" << __LINE__ << ") " << x << std::endl;

const std::string red("\033[0;31m");
const std::string green("\033[0;32m");
const std::string blue("\033[0;34m");
const std::string reset("\033[0m");

#define AP_LOG(x) std::cout << x << std::endl;
#define AP_LOG_r(x) std::cout << red << x << reset << std::endl;
#define AP_LOG_g(x) std::cout << green << x << reset << std::endl;
#define AP_LOG_b(x) std::cout << blue << x << reset << std::endl;

#define START_TIMER std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
#define END_TIMER                                                                 \
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now(); \
	std::cout << "ΔT = " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() << "[ns] " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs] " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;
#define END_TIMER_MICROSECONDS                                                    \
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now(); \
	std::cout << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << std::endl;

// utility structure for realtime plot
struct ScrollingBuffer
{
	int MaxSize;
	int Offset;
	ImVector<ImVec2> Data;

	ScrollingBuffer(int max_size = 5000);
	void AddPoint(float x, float y);
	void Erase();
};

enum GraphType
{
	LINE,
	BAR,
};

struct iDGraphData
{
	float history = 20.0f;
	std::string graphName = "";
	GraphType type;
	ScrollingBuffer buffer;
	int64_t min = 0, max = 0;

	iDGraphData(std::string name = "Default", GraphType type = GraphType::LINE)
	{
		this->graphName = name;
		this->type = type;
	}
};

struct iiDGraphData
{
	std::string graphName = "";
	ScrollingBuffer buffer;
};

struct sGraphData
{
	std::string graphName = "";
	ScrollingBuffer buffer;
};
