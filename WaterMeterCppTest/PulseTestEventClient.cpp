#include "PulseTestEventClient.h"

WaterMeterCppTest::PulseTestEventClient::PulseTestEventClient(EventServer* eventServer): TestEventClient(eventServer) {
    eventServer->subscribe(this, Topic::Pulse);
    eventServer->subscribe(this, Topic::Sample);
}

void WaterMeterCppTest::PulseTestEventClient::update(const Topic topic, const long payload) {
    TestEventClient::update(topic, payload);
    if (topic == Topic::Sample) {

    }

    if (payload > 4 || payload < 0) {
            _extremeCount[5]++;
        }
        else {
            _extremeCount[payload]++;
        }
    char numberBuffer[32];
    safeSprintf(numberBuffer, "[%d:%d,%d]%d", _sampleNumber, _currentCoordinate.x, _currentCoordinate.y, payload);
    safeStrcat(_buffer, numberBuffer);
    if (payload == 4) {
        safeStrcat(_buffer, "\n");
    }
}

void WaterMeterCppTest::PulseTestEventClient::update(const Topic topic, const Coordinate payload) {
    _sampleNumber++;
    _currentCoordinate = payload;
}
