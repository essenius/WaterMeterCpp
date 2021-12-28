
#ifndef HEADER_FLOWMETERDRIVER
#define HEADER_FLOWMETERDRIVER

#include "../WaterMeterCpp/FlowMeter.h"

class FlowMeterDriver : public FlowMeter {
public:

	FlowMeterDriver::FlowMeterDriver(int smoothValue, int derivative = 0, int smoothDerivative = 0,
		bool flow = false, bool peak = false, bool outlier = false, bool exclude = false, bool excludeAll = false);

	float isFirstOutlier();
};

#endif
