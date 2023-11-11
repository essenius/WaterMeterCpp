// Copyright 2023 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// Mock sensor for testing MagnetoSensorReader

#pragma once
#include "../WaterMeterCpp/MagnetoSensorNull.h"

class MagnetoSensorMock final :
    public MagnetoSensorNull
{
public:
    bool begin() override;
    int beginFailuresLeft() const { return _beginFailuresLeft; }
    double getGain() const override { return 3000.0; }
    int getNoiseRange() const override { return 12; }
    void setFlat(const bool isFlat) { _isFlat = isFlat; }
    bool handlePowerOn() override;
    bool isReal() const override { return true; }
    int powerFailuresLeft() const { return _powerFailuresLeft; }
    bool read(SensorData& sample) override;
    void setBeginFailures(int failures);
    void setPowerOnFailures(int failures);

private:
    int _beginFailuresLeft = 0;
    int _powerFailuresLeft = 0;
    bool _isFlat = false;
};
