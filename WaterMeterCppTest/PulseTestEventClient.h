#pragma once
#include "TestEventClient.h"


namespace WaterMeterCppTest {
    class PulseTestEventClient final : public TestEventClient {
    public:
        explicit PulseTestEventClient(EventServer* eventServer);
        void update(Topic topic, long payload) override;
        void update(Topic topic, Coordinate payload) override;
        int getExtremeCount(const int extreme) const { return _extremeCount[extreme]; }
        const char* pulseHistory() { return _buffer;  }
    private:
        int _extremeCount[6] = {};
        char _buffer[4096] = {};
        int _sampleNumber = -1;
        Coordinate _currentCoordinate = {};
    };
}
