#include "utilities.h"

void substituteInvisibleChars(char *str)
{
    char *p = str;
    while (*p != '\0')
    {
        if (*p == '\\' && *(p + 1) != '\0')
        {
            switch (*(p + 1))
            {
            case 'n':
                *p++ = '\n';
                std::memmove(p, p + 1, std::strlen(p + 1) + 1);
                break;
            case 't':
                *p++ = '\t';
                std::memmove(p, p + 1, std::strlen(p + 1) + 1);
                break;
            case 'b':
                *p++ = '\b';
                std::memmove(p, p + 1, std::strlen(p + 1) + 1);
                break;
            case 'r':
                *p++ = '\r';
                std::memmove(p, p + 1, std::strlen(p + 1) + 1);
                break;
            case 'f':
                *p++ = '\f';
                std::memmove(p, p + 1, std::strlen(p + 1) + 1);
                break;
            case 'v':
                *p++ = '\v';
                std::memmove(p, p + 1, std::strlen(p + 1) + 1);
                break;
            default:
                ++p;
                break;
            }
        }
        else
        {
            ++p;
        }
    }
}

std::string substituteInvisibleChars(const std::string &str)
{
    std::regex newline("\\\\n");
    std::regex tab("\\\\t");
    std::regex backspace("\\\\b");
    std::regex carriage_return("\\\\r");
    std::regex form_feed("\\\\f");
    std::regex vertical_tab("\\\\v");

    std::string result = std::regex_replace(str, newline, "\n");
    result = std::regex_replace(result, tab, "\t");
    result = std::regex_replace(result, backspace, "\b");
    result = std::regex_replace(result, carriage_return, "\r");
    result = std::regex_replace(result, form_feed, "\f");
    result = std::regex_replace(result, vertical_tab, "\v");

    return result;
}

ScrollingBuffer::ScrollingBuffer()
{
    Ds = new ImPlotPoint[DownSampleSize];
    Offset = 0;
    Data.reserve(MaxSize);
}

ImPlotPoint ScrollingBuffer::cbGetPlotPointDownSampleAt(int idx, void *data)
{
    ImPlotPoint *ds = (ImPlotPoint *)data;
    return ImPlotPoint(ds[idx].x, ds[idx].y);
}

inline ImPlotPoint ScrollingBuffer::GetDataAt(int offset, int idx)
{
    return ImPlotPoint(Data[offset + idx].x, Data[offset + idx].y);
}

int ScrollingBuffer::DownSampleLTTB(int start, int end)
{
    // Largest Triangle Three Buckets (LTTB) Downsampling Algorithm
    //  "Downsampling time series for visual representation" by Sveinn Steinarsson.
    //  https://skemman.is/bitstream/1946/15343/3/SS_MSthesis.pdf
    //  https://github.com/sveinn-steinarsson/flot-downsample
    int rawSamplesCount = (end - start) + 1;
    if (rawSamplesCount > DownSampleSize)
    {
        int sampleIdxOffset = start;
        double every = ((double)rawSamplesCount) / ((double)DownSampleSize);
        int aIndex = 0;
        // fill first sample
        Ds[0] = GetDataAt(sampleIdxOffset, 0);
        // loop over samples
        for (int i = 0; i < DownSampleSize - 2; ++i)
        {
            int avgRangeStart = (int)(i * every) + 1;
            int avgRangeEnd = (int)((i + 1) * every) + 1;
            if (avgRangeEnd > DownSampleSize)
                avgRangeEnd = DownSampleSize;

            int avgRangeLength = avgRangeEnd - avgRangeStart;
            double avgX = 0.0;
            double avgY = 0.0;
            for (; avgRangeStart < avgRangeEnd; ++avgRangeStart)
            {
                ImPlotPoint sample = GetDataAt(sampleIdxOffset, avgRangeStart);
                if (sample.y != NAN)
                {
                    avgX += sample.x;
                    avgY += sample.y;
                }
            }
            avgX /= (double)avgRangeLength;
            avgY /= (double)avgRangeLength;

            int rangeOffs = (int)(i * every) + 1;
            int rangeTo = (int)((i + 1) * every) + 1;
            if (rangeTo > DownSampleSize)
                rangeTo = DownSampleSize;
            ImPlotPoint samplePrev = GetDataAt(sampleIdxOffset, aIndex);
            double maxArea = -1.0;
            int nextAIndex = rangeOffs;
            for (; rangeOffs < rangeTo; ++rangeOffs)
            {
                ImPlotPoint sampleAtRangeOffs = GetDataAt(sampleIdxOffset, rangeOffs);
                if (sampleAtRangeOffs.y != NAN)
                {
                    double area = fabs((samplePrev.x - avgX) * (sampleAtRangeOffs.y - samplePrev.y) - (samplePrev.x - sampleAtRangeOffs.x) * (avgY - samplePrev.y)) / 2.0;
                    if (area > maxArea)
                    {
                        maxArea = area;
                        nextAIndex = rangeOffs;
                    }
                }
            }
            Ds[i + 1] = GetDataAt(sampleIdxOffset, nextAIndex);
            aIndex = nextAIndex;
        }
        // fill last sample
        Ds[DownSampleSize - 1] = GetDataAt(sampleIdxOffset, rawSamplesCount - 1);
        return DownSampleSize;
    }
    else
    {
        int sampleIdxOffset = start;
        // loop over samples
        for (int i = 0; i < rawSamplesCount; ++i)
        {
            Ds[i] = GetDataAt(sampleIdxOffset, i);
        }
        return rawSamplesCount;
    }
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
