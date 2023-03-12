#pragma once
#include <string>

class InputStream
{
private:
	
public:
	InputStream() {};
	virtual ~InputStream() = 0;
	virtual void DrawDataInputPanel() = 0;
	virtual std::string GetData() = 0;
};
