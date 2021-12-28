
 #include "pch.h"

#include "FlowMeterDriver.h"

// constructor for ResultWriterTest. Uses fields that are used for reporting
FlowMeterDriver::FlowMeterDriver(int smoothValue, int derivative, int smoothDerivative,
	bool flow, bool peak, bool outlier, bool exclude, bool excludeAll) {
	_smoothValue = static_cast<float>(smoothValue);
	_derivative = static_cast<float>(derivative);
	_smoothDerivative = static_cast<float>(smoothDerivative);
	_exclude = exclude;
	_excludeAll = excludeAll;
	_flow = flow;
	_outlier = outlier;
	_peak = peak;
}


float FlowMeterDriver::isFirstOutlier() {
	return _firstOutlier;
}
