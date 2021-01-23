#include "pch.h"
#include <string.h>
#include "../WaterMeterCpp/ResultWriter.h"
#include "../WaterMeterCpp/SerialDriver.h"
#include "../WaterMeterCpp/Storage.h"
#include "ResultWriterDriver.h"

Storage storageResultWriter;

ResultWriterDriver::ResultWriterDriver() : ResultWriter(&storageResultWriter, 10000) {
	_output[0] = '\0';
	storageResultWriter.clear();

	setDesiredFlushRate(6000);
}

char* ResultWriterDriver::getPrintBuffer() {
	return _printBuffer;
}

char* ResultWriterDriver::getOutput() {
	return _output;
}

void ResultWriterDriver::flush() {
	strcpy_s(_output, BatchWriter::PRINT_BUFFER_SIZE, _printBuffer);
	ResultWriter::flush();
}
