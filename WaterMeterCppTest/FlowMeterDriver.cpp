#include "pch.h"
#include "FlowMeterDriver.h"

FlowMeterDriver::FlowMeterDriver() {
}

// constructor for ResultWriterTest. Uses fields that are used for reporting
FlowMeterDriver::FlowMeterDriver(int amplitude, int highPass, int lowPassFast, int lowPassSlow, bool exclude, bool excludeAll, bool flow, int lowPassOnHighPass, bool outlier, bool drift, int peak) {
	_amplitude = (float)amplitude;
	_highPass = (float)highPass;
	_lowPassFast = (float)lowPassFast;
	_lowPassSlow = (float)lowPassSlow;
	_exclude = exclude;
	_excludeAll = excludeAll;
	_flow = flow;
	_lowPassOnHighPass = (float)lowPassOnHighPass;
	_outlier = outlier;
	_drift = drift;
	_peak = peak;
}

// constructor for FlowMeterTest. Uses order in which the items are calculated
FlowMeterDriver::FlowMeterDriver(float amplitude, bool outlier, bool firstOutlier, float highPass, float lowPassOnHighPass, bool calulatedFlow, 
	float lowPassSlow, float lowPassFast, float lowPassDifference, float lowPassOnDifference, bool drift, bool exclude, bool excludeAll, bool flow, int peak) {
	_amplitude = amplitude;
	_outlier = outlier;
	_firstOutlier = firstOutlier;
	_highPass = highPass;
	_lowPassOnHighPass = lowPassOnHighPass;
	_calculatedFlow = calulatedFlow;
	_lowPassSlow = lowPassSlow;
	_lowPassFast = lowPassFast;
	_lowPassDifference = lowPassDifference;
	_lowPassOnDifference = lowPassOnDifference;
	_drift = drift;
	_exclude = exclude;
	_excludeAll = excludeAll;
	_flow = flow;
	_peak = peak;
}

float FlowMeterDriver::getHighPass() {
	return _highPass;
}

float FlowMeterDriver::getLowPassDifference() {
	return _lowPassDifference;
}

float FlowMeterDriver::getLowPassOnDifference() {
	return _lowPassOnDifference;
}

float FlowMeterDriver::isFirstOutlier() {
	return _firstOutlier;
}

float FlowMeterDriver::hasCalculatedFlow() {
	return _calculatedFlow;
}

int FlowMeterDriver::hasCalculatedPeak() {
	return _calculatedPeak;
}
