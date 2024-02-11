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
