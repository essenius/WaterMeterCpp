#include "SensorDataQueue.h"

SensorDataQueue::SensorDataQueue(EventServer* eventServer, SensorDataQueuePayload* payload) :
    DataQueue(eventServer, payload, 40960),
    _freeSpace(eventServer, this, Topic::FreeQueue, 1000, 2000) {}

size_t SensorDataQueue::freeSpace() {
    const auto space = DataQueue::freeSpace();
    _freeSpace = static_cast<long>(space);
    return space;
}
