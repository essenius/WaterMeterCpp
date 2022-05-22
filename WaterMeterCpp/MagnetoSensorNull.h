// Copyright 2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#ifndef HEADER_MAGNETOSENSORNULL
#define HEADER_MAGNETOSENSORNULL

#include "MagnetoSensor.h"

class MagnetoSensorNull : public MagnetoSensor
{
public:
    MagnetoSensorNull() : MagnetoSensor(0) {}

    bool begin() override {
        return false;
    }

    bool configure() const override {
        return false;
    }
    float getGain() const override {
        return 0.0f;
    }

    int getNoiseRange() const override {
        return 0;
    }

    bool isOn() const override {
        return true;
    }

    bool isReal() const override {
        return false;
    }

    bool read(SensorData* sample) const override {
        sample->reset();
        return false;
    }

    void softReset() const override {}
};

#endif
