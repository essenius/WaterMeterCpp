#include "pch.h"
#include <string.h>
#include "../WaterMeterCpp/MeasurementWriter.h"
#include "../WaterMeterCpp/SerialDriver.h"
#include "../WaterMeterCpp/Storage.h"
#include "MeasurementWriterDriver.h"

Storage storage1(1);

MeasurementWriterDriver::MeasurementWriterDriver() : MeasurementWriter(&storage1) {
	_output[0] = '\0';
	storage1.clear();
	setDesiredFlushRate("DEFAULT");

}

char* MeasurementWriterDriver::getPrintBuffer() {
	return _printBuffer;
}

char* MeasurementWriterDriver::getOutput() {
	return _output;
}

void MeasurementWriterDriver::flush()
{
	strcpy_s(_output, BatchWriter::PRINT_BUFFER_SIZE, _printBuffer);
	MeasurementWriter::flush();
}

