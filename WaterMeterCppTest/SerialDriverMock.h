#pragma once
#include "..\WaterMeterCpp\SerialDriver.h"

class SerialDriverMock : public SerialDriver {
public:
	void setNextReadResponse(const char* command);
//	virtual bool inputAvailable();
//	virtual void println(const char* output);
	char* getOutput();
	void clearOutput();
};

