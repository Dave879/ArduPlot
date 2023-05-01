#pragma once
#include <vector>
#include <string>

std::vector<std::string> split(std::string s, std::string delimiter)
{
	std::vector<std::string> vec;
	size_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos)
	{
		token = s.substr(0, pos);
		vec.push_back(token);
		s.erase(0, pos + delimiter.length());
	}
	return vec;
}