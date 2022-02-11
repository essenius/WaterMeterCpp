#pragma once
#include "DataQueue.h"
#include "LongChangePublisher.h"

class SensorDataQueue : public DataQueue {
public:
    SensorDataQueue(EventServer* eventServer, SensorDataQueuePayload* payload);
    size_t freeSpace() override;

protected:
    LongChangePublisher _freeSpace;
};

