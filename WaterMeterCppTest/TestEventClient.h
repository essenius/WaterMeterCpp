#pragma once
#include "..\WaterMeterCpp\EventServer.h"
class TestEventClient : public EventClient {
public:
    TestEventClient(const char* name, EventServer* eventServer) : EventClient(name, eventServer) {
        _payload[0] = 0;
    }

    void reset();
    virtual void update(Topic topic, const char* payload);
    virtual void update(Topic topic, long payload);
    Topic getTopic();
    char* getPayload();
    int getCallCount();
private:
    int _callCount = 0;
    Topic _topic = Topic::None;
    char _payload[512];
};
