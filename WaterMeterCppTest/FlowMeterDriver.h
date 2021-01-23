#ifndef HEADER_FLOWMETERDRIVER
#define HEADER_FLOWMETERDRIVER

#include "../WaterMeterCpp/FlowMeter.h"

class FlowMeterDriver : public FlowMeter {
public:
	FlowMeterDriver();
	FlowMeterDriver::FlowMeterDriver(int amplitude, int highPass, int lowPassFast, int lowPassSlow,
		bool exclude, bool excludeAll, bool flow, int lowPassOnHighPass, bool outlier, bool drift, int peak);

	FlowMeterDriver::FlowMeterDriver(float amplitude, bool outlier, bool firstOutlier, float highPass, float lowPassOnHighPass, bool calulatedFlow,
		float lowPassSlow, float lowPassFast, float lowPassDifference, float lowPassOnDifference, bool drift, bool exclude, bool excludeAll, bool flow, int peak);
	int hasCalculatedPeak();
	float getHighPass();
	float getLowPassDifference();
	float getLowPassOnDifference();
	float isFirstOutlier();
	float hasCalculatedFlow();
};

#endif