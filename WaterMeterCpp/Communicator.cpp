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

#include "Communicator.h"
#include "Connector.h"
#include "DataQueuePayload.h"

Communicator::Communicator(EventServer* eventServer, Log* logger, LedDriver* ledDriver, OledDriver* oledDriver, Meter* meter, Device* device,
                           DataQueue* dataQueue, Serializer* serializer,
                           QueueClient* fromSamplerQueueClient, QueueClient* fromConnectorQueueClient) :
    EventClient(eventServer),
    _logger(logger),
    _ledDriver(ledDriver),
    _oledDriver(oledDriver),
    _meter(meter),
    _device(device),
    _dataQueue(dataQueue),
    _serializer(serializer),
    _samplerQueueClient(fromSamplerQueueClient),
    _connectorQueueClient(fromConnectorQueueClient) {}

void Communicator::loop() const {
    int i = 0;
    while (_samplerQueueClient->receive() || _connectorQueueClient->receive()) { 
      i++;
      // make sure to wait occasionally to allow other task to run
      if (i%5 == 0) delay(5); 
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

void Communicator::setup() const {
    _logger->begin();
    _ledDriver->begin();
    _meter->begin();

    // what can be sent to mqtt (note: nothing sent to the sampler)
    _eventServer->subscribe(_connectorQueueClient, Topic::Alert);
    _eventServer->subscribe(_connectorQueueClient, Topic::BatchSize);
    _eventServer->subscribe(_connectorQueueClient, Topic::FreeHeap);
    _eventServer->subscribe(_connectorQueueClient, Topic::FreeStack);
    _eventServer->subscribe(_connectorQueueClient, Topic::Rate);
    _eventServer->subscribe(_connectorQueueClient, Topic::SensorWasReset);
    _eventServer->subscribe(_connectorQueueClient, Topic::NoSensorFound);
    _eventServer->subscribe(_connectorQueueClient, Topic::NoDisplayFound);
    _eventServer->subscribe(_serializer, Topic::SensorData);
    
    // can publish NoDisplayFound
    _oledDriver->begin();
}

[[ noreturn ]] void Communicator::task(void* parameter) {
    const auto me = static_cast<Communicator*>(parameter);
    for (;;) {
        me->loop();
    }
}
