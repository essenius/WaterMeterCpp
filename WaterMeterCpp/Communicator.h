// Copyright 2022-2023 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// Communicator runs the process that communicates with the user via logging to serial, displaying on OLED, driving LEDs
// It has its own event server so we keep it separate of the connector and the sampler.

#ifndef HEADER_COMMUNICATOR
#define HEADER_COMMUNICATOR

#include "DataQueue.h"
#include "Device.h"
#include "Meter.h"
#include "OledDriver.h"
#include "QueueClient.h"
#include "Serializer.h"

namespace WaterMeter {
    class Communicator final : public EventClient {
    public:
        Communicator(EventServer* eventServer, OledDriver* oledDriver, Device* device,
            DataQueue* dataQueue, Serializer* serializer,
            QueueClient* fromSamplerQueueClient, QueueClient* fromConnectorQueueClient);
        void begin() const;
        void loop() const;
        static void task(void* parameter);

    private:
        OledDriver* _oledDriver;
        Device* _device;
        DataQueue* _dataQueue;
        Serializer* _serializer;
        QueueClient* _samplerQueueClient;
        QueueClient* _connectorQueueClient;
    };
}
#endif
