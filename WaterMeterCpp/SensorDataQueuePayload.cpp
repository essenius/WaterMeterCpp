#include "SensorDataQueuePayload.h"

size_t SensorDataQueuePayload::size() const {

    // optimizing the use of the buffer by not sending unused parts
    size_t size = sizeof(SensorDataQueuePayload) - sizeof(Content);
    switch (topic) {
    case Topic::Result:
        size += sizeof(ResultData);
        break;
    case Topic::Samples:
        size += sizeof(Samples::count) + buffer.samples.count * sizeof(Samples::value[0]);
        break;
        case Topic::SamplingError:
    case Topic::Info:
        size += strlen(buffer.message);
        break; 
    default:
        break;
    }
    return size;
}
