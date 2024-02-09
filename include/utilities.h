#pragma once

#include <vector>
#include <string>
#include <imgui.h>
#include <implot.h>
#include <memory>
#include <cstring>
#include <iostream>
#include <math.h>
#include <regex>

#define COLORS_IN_TERMINAL true

// #define AP_LOG(x) std::cout <<  __FILE__  << "(" << __LINE__ << ") " << x << std::endl;

const std::string red("\033[0;31m");
const std::string green("\033[0;32m");
const std::string blue("\033[0;34m");
const std::string reset("\033[0m");

#define AP_LOG(x) std::cout << x << std::endl;

#if COLORS_IN_TERMINAL == true

#define AP_LOG_r(x) std::cout << red << x << reset << std::endl;
#define AP_LOG_g(x) std::cout << green << x << reset << std::endl;
#define AP_LOG_b(x) std::cout << blue << x << reset << std::endl;

#else

#define AP_LOG_r(x) AP_LOG(x);
#define AP_LOG_g(x) AP_LOG(x);
#define AP_LOG_b(x) AP_LOG(x);

#endif

#define LOGERR(x) AP_LOG_r(x)

#define START_TIMER std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
#define END_TIMER                                                                \
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now(); \
	std::cout << "ΔT = " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() << "[ns] " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs] " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;
#define END_TIMER_MICROSECONDS                                                   \
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now(); \
	std::cout << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << std::endl;


template <typename T>
void printVec(const std::vector<T> &vec)
{
	for (size_t i = 0; i < vec.size(); i++)
	{
		AP_LOG(i << "->" << vec.at(i));
	}
}

void substituteInvisibleChars(char* str);
std::string substituteInvisibleChars(const std::string& str);

// utility structure for realtime plot
struct ScrollingBuffer
{
	static const int MaxSize = 100000;
	int Offset;
	ImVector<ImVec2> Data;
	ImPlotPoint *Ds;
	static const int DownSampleSize = 8192;

	// LTTB implementation from ozlb: https://github.com/epezent/implot/pull/389

	ScrollingBuffer();
	inline ImPlotPoint GetDataAt(int offset, int idx);
	static ImPlotPoint cbGetPlotPointDownSampleAt(int idx, void *data);
	int DownSampleLTTB(int start, int end);
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

	friend std::ostream &operator<<(std::ostream &os, const iDGraphData &data)
	{
		os << "merge_ID: " << data.merge_ID << ", ";
		os << "is_parent: " << std::boolalpha << data.is_parent << ", ";
		os << "is_child: " << std::boolalpha << data.is_child << ", ";
		os << "graphName: " << data.graphName << ", ";
		return os;
	}
};

struct ScatterPlotData
{
	std::string graphName = "";
	std::vector<ImPlotPoint> data;

	ScatterPlotData(std::string name = "Default")
	{
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

	friend std::ostream &operator<<(std::ostream &os, const iiDGraphData &data)
	{
		os << "graphName: " << data.graphName << ", ";
		os << "sizex: " << data.sizex << ", ";
		os << "sizey: " << data.sizey;
		return os;
	}
};

struct sGraphData
{
	std::string graphName = "";
	ScrollingBuffer buffer;
};

class CircularTextBuffer
{
private:
	char *buffer;
	char *end;
	int mem_capacity; // Fixed size of the buffer itself
	int size;			// Size of data inside buffer

public:
	CircularTextBuffer()
	{
	}

	CircularTextBuffer(int capacity) : mem_capacity(capacity)
	{
		buffer = new char[capacity + 1]; // +1 for the null terminator
		clear();
	}

	void append(const char *new_text)
	{
		int new_text_len = strlen(new_text);
		// Truncates beginning part of new_text if it
		// is too big to fit in the circular text buffer
		if (new_text_len > mem_capacity)
		{
			new_text += (new_text_len - mem_capacity);
			new_text_len = mem_capacity;
		}

		// If text too long for the memory area allocated for the buffer
		// reduce old buffer to fit and place it at the beginning
		if (new_text_len > (mem_capacity - size))
		{
			// Remaining size of old buffer, cut off partially
			int remaining_size = mem_capacity - new_text_len;
			// Copy part of the old buffer at the beginning of the memory region
			memcpy(buffer, buffer + new_text_len, remaining_size);

			size = remaining_size;
			end = buffer + size;
		}

		// Appends new text to buffer
		memcpy(end, new_text, new_text_len);
		size += new_text_len;
		end += new_text_len;

		*end = '\0';
	}

	const char *getText() const
	{
		return buffer;
	}

	void clear()
	{
		end = buffer;
		size = 0;
		buffer[0] = '\0';
	}
};

int32_t FindInVec(const std::vector<std::string> &vec, const std::string &s);
