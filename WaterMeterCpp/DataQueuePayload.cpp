#include "DataQueuePayload.h"

size_t DataQueuePayload::size() const {

    // optimizing the use of the buffer by not sending unused parts
    size_t size = sizeof(DataQueuePayload) - sizeof(Content);
    switch (topic) {
    case Topic::Result:
        size += sizeof(ResultData);
        break;
    case Topic::Samples:
        size += sizeof(Samples::count) + buffer.samples.count * sizeof(Samples::value[0]);
        break;
    case Topic::ConnectionError:
    case Topic::Info:
        size += strlen(buffer.message) + 1;
        break; 
    default:
        break;
    }
    return size;
}
