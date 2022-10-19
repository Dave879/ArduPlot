
#include <imgui.h>

//-----------------------------------------------------------------------------
// [SECTION] Example App: Debug Log / ShowExampleAppLog()
//-----------------------------------------------------------------------------

// Usage:
//  static ExampleAppLog my_log;
//  my_log.AddLog("Hello %d world\n", 123);
//  my_log.Draw("title");
struct ExampleAppLog
{
	ImGuiTextBuffer Buf;
	ImGuiTextFilter Filter;
	ImVector<int> LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
	bool AutoScroll;		   // Keep scrolling if already at the bottom.

	ExampleAppLog();

	void Clear();

	void AddLog(const char *fmt, ...);

	void Draw(const char *title, bool *p_open);

};

// Demonstrate creating a simple log window with basic filtering.
void ShowExampleAppLog(const std::string contents, const bool stopFlow, const bool &newData);

