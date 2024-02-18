#pragma once

#include <string>

enum InputStatus {
	kOk,
	kGenericError,
	kIOError
};

class InputStream
{
public:
	virtual void DrawGUI() = 0;
	virtual InputStatus GetData(std::string &data) = 0;
	virtual bool IsConnected() = 0;
	virtual ~InputStream(){};
};