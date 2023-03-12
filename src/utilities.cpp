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

float findMin(float arr[], int ArrSize)
{
	float min = arr[0];
	for (size_t i = 1; i < ArrSize; i++)
	{
		if (arr[i] < min)
		{
			min = arr[i];
		}
		// std::cout << "i: " << i << " -> " << arr[i] << std::endl;
	}
	return min;
}

float findMax(float arr[], int ArrSize)
{
	float max = arr[0];
	for (size_t i = 1; i < ArrSize; i++)
	{
		if (arr[i] > max)
			max = arr[i];
	}
	return max;
}
