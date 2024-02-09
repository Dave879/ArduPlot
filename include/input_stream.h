#pragma once

#include <string>

class InputStream
{
public:
	virtual void DrawGUI() = 0;
	virtual std::string GetData() = 0;
	virtual bool IsConnected() = 0;
	virtual ~InputStream(){};
};