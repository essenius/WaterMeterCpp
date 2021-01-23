#include "pch.h"
#include <string.h>
#include "BatchWriterDriver.h"
#include "../WaterMeterCpp/BatchWriter.h"
#include "../WaterMeterCpp/SerialDriver.h"

BatchWriterDriver::BatchWriterDriver() : BatchWriter(10, 'B') {
	_output[0] = '\0';
}

char *BatchWriterDriver::toStringDriver(float input, byte fractionDigits)
{
	return toString(input, fractionDigits);
}

char* BatchWriterDriver::getOutput()
{
	return _output;
}

void BatchWriterDriver::flush()
{
	strcpy_s(_output, BatchWriter::PRINT_BUFFER_SIZE, _printBuffer);
	BatchWriter::flush();
}

