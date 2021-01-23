#ifndef HEADER_RESULTWRITERDRIVER
#define HEADER_RESULTWRITERRIVER

#include <stdio.h>
#include "../WaterMeterCpp/resultWriter.h"

class ResultWriterDriver :  public ResultWriter {
public:
	ResultWriterDriver();
	char* getPrintBuffer();
	char* getOutput();
	virtual void flush();
protected:
	char _output[BatchWriter::PRINT_BUFFER_SIZE];
};

#endif

