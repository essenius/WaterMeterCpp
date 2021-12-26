#include "pch.h"
#include "TestEventClient.h"


void TestEventClient::reset() {
    _callCount = 0;
    _topic = Topic::None;
    _payload[0] = 0;
}

void TestEventClient::update(Topic topic, const char* payload) {
    _callCount++;
    _topic = topic;
    strcpy(_payload, payload);
}

void TestEventClient::update(Topic topic, long payload) {
    _callCount++;
    _topic = topic;
    sprintf(_payload, "%ld", payload);
}

Topic TestEventClient::getTopic() { return _topic; }

char* TestEventClient::getPayload() { return _payload; }

int TestEventClient::getCallCount() { return _callCount; }

