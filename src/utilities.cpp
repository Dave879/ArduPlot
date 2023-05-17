#include "utilities.h"

ScrollingBuffer::ScrollingBuffer(int max_size)
{
    MaxSize = max_size;
    Offset = 0;
    Data.reserve(MaxSize);
}

void ScrollingBuffer::AddPoint(float x, float y)
{
    if (Data.size() < MaxSize)
        Data.push_back(ImVec2(x, y));
    else
    {
        Data[Offset] = ImVec2(x, y);
        Offset = (Offset + 1) % MaxSize;
    }
}
void ScrollingBuffer::Erase()
{
    if (Data.size() > 0)
    {
        Data.shrink(0);
        Offset = 0;
    }
}

int32_t FindInVec(const std::vector<std::string> &vec, const std::string &s)
{
	for (size_t i = 0; i < vec.size(); i++)
	{
		if (vec.at(i) == s)
		{
			return i;
		}
	}
	return -1;
}


