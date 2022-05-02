#pragma once

#include <vector>
#include <Mahi/Gui.hpp>

// utility structure for realtime plot
struct ScrollingBuffer
{
	int MaxSize;
	int Offset;
	ImVector<ImVec2> Data;
	
	ScrollingBuffer(int max_size = 2000);
	void AddPoint(float x, float y);
	void Erase();
};
