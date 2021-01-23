#ifndef HEADER_MEASUREMENTWRITERDRIVER
#define HEADER_MEASUREMENTWRITERRIVER

#include "../WaterMeterCpp/MeasurementWriter.h"

class MeasurementWriterDriver : public MeasurementWriter {
public:
	MeasurementWriterDriver();
	char* getPrintBuffer();
	char* getOutput();
	virtual void flush();
protected:
	char _output[BatchWriter::PRINT_BUFFER_SIZE];
};
#endif

