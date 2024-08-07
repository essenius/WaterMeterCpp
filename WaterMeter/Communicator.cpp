// Copyright 2022-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include "Communicator.h"
#include "Connector.h"
#include "DataQueuePayload.h"
#include "Meter.h"


namespace WaterMeter {
    Communicator::Communicator(EventServer* eventServer, OledDriver* oledDriver, Device* device,
        DataQueue* dataQueue, Serializer* serializer,
        QueueClient* fromSamplerQueueClient, QueueClient* fromConnectorQueueClient) :
        EventClient(eventServer),
        _oledDriver(oledDriver),
        _device(device),
        _dataQueue(dataQueue),
        _serializer(serializer),
        _samplerQueueClient(fromSamplerQueueClient),
        _connectorQueueClient(fromConnectorQueueClient) {}

    void Communicator::begin() const {

        // what can be sent to mqtt (note: nothing sent to the sampler)
        _eventServer->subscribe(_connectorQueueClient, Topic::Alert);
        _eventServer->subscribe(_connectorQueueClient, Topic::BatchSize);
        _eventServer->subscribe(_connectorQueueClient, Topic::FreeHeap);
        _eventServer->subscribe(_connectorQueueClient, Topic::FreeStack);
        _eventServer->subscribe(_connectorQueueClient, Topic::Rate);
        _eventServer->subscribe(_connectorQueueClient, Topic::SensorWasReset);
        _eventServer->subscribe(_connectorQueueClient, Topic::SensorState);
        _eventServer->subscribe(_connectorQueueClient, Topic::NoDisplayFound);
        _eventServer->subscribe(_connectorQueueClient, Topic::MeterPayload);
        _eventServer->subscribe(_serializer, Topic::SensorData);

        // Dependencies are reduced by publishing a 'Begin' event for objects that only need to get initialized.
        // oledDriver begin can publish a NoDisplayFound, so we need that to happen last. We control that via the payload:
        // false for as quickly as possible, true for starting after base services are up.

        _eventServer->publish(Topic::Begin, false);
        _eventServer->publish(Topic::Begin, true);
    }

    void Communicator::loop() const {
        int i = 0;
        while (_samplerQueueClient->receive() || _connectorQueueClient->receive()) {
            i++;
            // make sure to wait occasionally to allow other task to run
            if (i % 4 == 0) delay(5);
        }
        DataQueuePayload* payload;
        while ((payload = _dataQueue->receive()) != nullptr) {
            _eventServer->publish(Topic::SensorData, reinterpret_cast<const char*>(payload));
            delay(5);
        }
        _device->reportHealth();
        const auto waitedTime = _oledDriver->display();
        if (waitedTime < 10) {
            delay(10 - static_cast<int>(waitedTime));
        }
    }

    [[ noreturn ]] void Communicator::task(void* parameter) {
        const auto me = static_cast<Communicator*>(parameter);
        for (;;) {
            me->loop();
        }
    }
}