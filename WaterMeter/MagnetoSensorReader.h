// Copyright 2021-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// Hide the actual implementation of the sensors
// This also takes care of detecting anomalies like flatlines (indicating the sensor might need a reboot).
// It can also do an externally requested sensor reboot by listening to the ResetSensor event.

#ifndef HEADER_MAGNETO_SENSOR_READER
#define HEADER_MAGNETO_SENSOR_READER

#include <MagnetoSensor.h>
#include "ChangePublisher.h"
#include "EventServer.h"
#include "SensorSample.h"

namespace WaterMeter {
    using MagnetoSensors::MagnetoSensor;

    constexpr int SoftReset = 1;
    constexpr int HardReset = 2;

    class MagnetoSensorReader : public EventClient {
    public:
        explicit MagnetoSensorReader(EventServer* eventServer);
        bool begin(MagnetoSensor* sensor[], size_t listSize);
        void configurePowerPort(uint8_t port);
        double getGain() const;
        int getNoiseRange() const;
        bool hardReset();
        SensorSample read() const;
        bool softReset();
        SensorState getState() { return _sensorState; }
        SensorState validate(const SensorSample& sample);

        void update(Topic topic, long payload) override;
        static constexpr byte DefaultPowerPort = 15;
        bool isResetting() const { return _isHardResetting || _isSoftResetting; }

    protected:
        // Both HMC and QMC datasheets report 50 ms startup time.
        static constexpr unsigned long StartupMicros = 50000;
        static constexpr int FlatlineStreak = 250;
        static constexpr int MaxStreaksToAlert = 10;
        static constexpr int DelaySensorMillis = 5;

        bool setSensor();
        SensorState setPower(uint8_t state);

        MagnetoSensor* _sensor = nullptr;
        ChangePublisher<SensorState> _sensorState;
        int _consecutiveStreakCount = 0;
        SensorSample _previousSample = { {0, 0} };
        int _flatlineCount = 0;
        uint8_t _powerPort = DefaultPowerPort;
        MagnetoSensor** _sensorList = nullptr;
        size_t _sensorListSize = 0;
        bool _isHardResetting = true;
        bool _isSoftResetting = true;
    };
}
#endif
