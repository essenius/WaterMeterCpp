#pragma once

#include "../WaterMeterCpp/Aggregator.h"

namespace WaterMeterCppTest {

    class AggregatorDriver : public Aggregator {
    public:
        AggregatorDriver(EventServer* eventServer);

        static long limit(const long input, const long min, const long max) {
            return Aggregator::limit(input, min, max);
        }

        static long convertToLong(const char* stringParam, long defaultValue) {
            return Aggregator::convertToLong(stringParam, defaultValue);
        }
    };

}
