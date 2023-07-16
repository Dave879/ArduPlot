#pragma once
#include <imgui.h>
#include <string>

#include <tracy/Tracy.hpp>

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
	ImGuiTextBuffer Buf;
	ImGuiTextFilter Filter;
	ImVector<int> LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
	bool AutoScroll;				// Keep scrolling if already at the bottom.

	void Clear();
	void AddLog(const char *fmt, ...);
	void Draw(const char *title, bool *p_open);

public:
	SerialConsole(std::string console_name);
	void Display();
	void Add(const std::string content);
};
