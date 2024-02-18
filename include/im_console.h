#pragma once
#include <imgui.h>
#include <string>
#include <tracy/Tracy.hpp>

#include "utilities.h"

//-----------------------------------------------------------------------------
// [SECTION] Example App: Debug Log / ShowExampleAppLog()
//-----------------------------------------------------------------------------

// Usage:
//  static ExampleAppLog my_log;
//  my_log.AddLog("Hello %d world\n", 123);
//  my_log.Draw("title");
class SerialConsole
{
private:
	std::string name;
	unsigned int buf_size;
	ImGuiTextBuffer Buf;
	ImGuiTextFilter Filter;
	ImVector<int> LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
	bool AutoScroll;				// Keep scrolling if already at the bottom.

	void Clear();
	void AddLog(const char *fmt, ...);
	void Draw(const char *title, bool *p_open);

public:
	/**
	 * @param console_name Name of the console window
	 * @param buffer_size The size of the text buffer, if omitted defaults to unlimited buffer
	 */
	SerialConsole(std::string console_name, unsigned int buffer_size = 0);
	void Display();
	void Add(const std::string content);
};

class FixedBufferSerialConsole
{
private:
	std::string name;
	unsigned int buf_size;
	CircularTextBuffer Buf;
	bool AutoScroll; // Keep scrolling if already at the bottom.

	void Clear();
	void AddLog(const char *b);
	void Draw(const char *title, bool *p_open);

public:
	/**
	 * @param console_name Name of the console window
	 * @param buffer_size The size of the text buffer, if omitted defaults to unlimited buffer
	 */
	FixedBufferSerialConsole(std::string console_name, int buffer_size = 50000);
	void Display();
	void Add(const std::string content);
};
