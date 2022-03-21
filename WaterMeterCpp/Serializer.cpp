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

Serializer::Serializer(EventServer* eventServer, PayloadBuilder* payloadBuilder) : EventClient(eventServer), _payloadBuilder(payloadBuilder) {}

void Serializer::update(Topic topic, const char* payload) {
    const auto sensorPayload = reinterpret_cast<const DataQueuePayload*>(payload);
    Topic newTopic;
    switch (sensorPayload->topic) {
    case Topic::Result:
        convertResult(sensorPayload);
        newTopic = Topic::ResultFormatted;
        break;
    case Topic::Samples:
        convertMeasurements(sensorPayload);
        newTopic = Topic::SamplesFormatted;
        break;
    case Topic::ConnectionError:
        convertString(sensorPayload);
        newTopic = Topic::ErrorFormatted;
        break;
    case Topic::Info:
        convertString(sensorPayload);
        newTopic = Topic::MessageFormatted;
        break;
    default:
        return;
    }
    _eventServer->publish(this, newTopic, _payloadBuilder->toString());
}

void Serializer::convertMeasurements(const DataQueuePayload* payload) const {
    _payloadBuilder->initialize();
    _payloadBuilder->writeTimestampParam("timestamp", payload->timestamp);
    _payloadBuilder->writeArrayStart("measurements");
    const auto samples = payload->buffer.samples;
    for (int i = 0; i < samples.count; i++) {
        _payloadBuilder->writeArrayValue(samples.value[i]);
    }
    _payloadBuilder->writeArrayEnd();
    _payloadBuilder->writeGroupEnd();
}

void Serializer::convertResult(const DataQueuePayload* payload) const {
    _payloadBuilder->initialize();
    _payloadBuilder->writeTimestampParam("timestamp", payload->timestamp);

    const auto result = payload->buffer.result;
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
    _payloadBuilder->writeParam("resets", result.resetCount);
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
}

void Serializer::convertString(const DataQueuePayload* data) const {
    _payloadBuilder->initialize(0);
    if (data->topic == Topic::ConnectionError) {
        _payloadBuilder->writeText("Error: ");
    }
    _payloadBuilder->writeText(data->buffer.message);
}
