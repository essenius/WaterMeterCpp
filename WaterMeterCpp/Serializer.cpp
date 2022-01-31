// Copyright 2021-2022 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#include "Serializer.h"

Serializer::Serializer(PayloadBuilder* payloadBuilder) : _payloadBuilder(payloadBuilder) {}

const char* Serializer::convertPayload(RingbufferPayload* data) const {
    switch (data->topic) {
    case Topic::Result:
        return convertResult(data);
    case Topic::Samples:
        return convertMeasurements(data);
    case Topic::SamplingError:
    case Topic::Info:
        return convertString(data);
    }
    return nullptr;
}

const char* Serializer::convertMeasurements(const RingbufferPayload* data) const {
    _payloadBuilder->initialize();
    _payloadBuilder->writeTimestampParam("timestamp", data->timestamp);
    _payloadBuilder->writeArrayStart("measurements");
    const auto samples = data->buffer.samples;
    for (int i = 0; i < samples.count; i++) {
        _payloadBuilder->writeArrayValue(samples.value[i]);
    }
    _payloadBuilder->writeArrayEnd();
    _payloadBuilder->writeGroupEnd();
    return _payloadBuilder->toString();
}

const char* Serializer::convertResult(const RingbufferPayload* data) const {
    _payloadBuilder->initialize();
    _payloadBuilder->writeTimestampParam("timestamp", data->timestamp);

    const auto result = data->buffer.result;
    _payloadBuilder->writeParam("lastValue", result.lastSample);
    _payloadBuilder->writeGroupStart("summaryCount");
    _payloadBuilder->writeParam("samples", result.sampleCount);
    _payloadBuilder->writeParam("peaks", result.peakCount);
    _payloadBuilder->writeParam("flows", result.flowCount);
    _payloadBuilder->writeParam("maxStreak", result.maxStreak);
    _payloadBuilder->writeGroupEnd();
    _payloadBuilder->writeGroupStart("exceptionCount");
    _payloadBuilder->writeParam("outliers", result.outlierCount);
    _payloadBuilder->writeParam("excludes", result.excludeCount);
    _payloadBuilder->writeParam("overruns", result.overrunCount);
    _payloadBuilder->writeGroupEnd();
    _payloadBuilder->writeGroupStart("duration");
    _payloadBuilder->writeParam("total", result.totalDuration);
    _payloadBuilder->writeParam("average", result.averageDuration);
    _payloadBuilder->writeParam("max", result.maxDuration);
    _payloadBuilder->writeGroupEnd();
    _payloadBuilder->writeGroupStart("analysis");
    _payloadBuilder->writeParam("smoothValue", result.smooth);
    _payloadBuilder->writeParam("derivative", result.derivativeSmooth);
    _payloadBuilder->writeParam("smoothDerivative", result.smoothDerivativeSmooth);
    _payloadBuilder->writeParam("smoothAbsDerivative", result.smoothAbsDerivativeSmooth);
    _payloadBuilder->writeGroupEnd();
    _payloadBuilder->writeGroupEnd();
    return _payloadBuilder->toString();
}

const char* Serializer::convertString(const RingbufferPayload* data) const {
    _payloadBuilder->initialize(0);
    _payloadBuilder->writeTimestamp(data->timestamp);
    _payloadBuilder->writeText(" ");
    _payloadBuilder->writeText((data->topic == Topic::SamplingError) ? "Error" : "Info");
    _payloadBuilder->writeText(": ");
    _payloadBuilder->writeText(data->buffer.message);
    return _payloadBuilder->toString();
}
