// Copyright 2022 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#include "Communicator.h"
#include "Connector.h"
#include "SensorDataQueuePayload.h"

Communicator::Communicator(EventServer* eventServer, Log* logger, LedDriver* ledDriver, Device* device,
    DataQueue* dataQueue, Serializer* serializer,
    QueueClient* fromSamplerQueueClient, QueueClient* fromConnectorQueueClient) :
    EventClient(eventServer),
    _logger(logger),
    _ledDriver(ledDriver),
    _device(device),
    _dataQueue(dataQueue),
    _serializer(serializer),
    _fromSamplerQueueClient(fromSamplerQueueClient),
    _fromConnectorQueueClient(fromConnectorQueueClient) {}

void Communicator::loop() const {
    while (_fromSamplerQueueClient->receive()) { delay(5); }
    while (_fromConnectorQueueClient->receive()) { delay(5); }
    SensorDataQueuePayload* payload;
    while ((payload = _dataQueue->receive()) != nullptr) {
        _eventServer->publish(Topic::SensorData, reinterpret_cast<const char*>(payload));
        delay(5);
    }
    _device->reportHealth();
    delay(10);
}

void Communicator::setup() const {
    _logger->begin();
    _ledDriver->begin();

    // TODO rename queues
    // what can be sent to mqtt (note: nothing sent to the sampler)
    _eventServer->subscribe(_fromConnectorQueueClient, Topic::Alert);
    _eventServer->subscribe(_fromConnectorQueueClient, Topic::BatchSize);
    _eventServer->subscribe(_fromConnectorQueueClient, Topic::FreeHeap);
    _eventServer->subscribe(_fromConnectorQueueClient, Topic::FreeStackSampler);
    _eventServer->subscribe(_fromConnectorQueueClient, Topic::FreeStackCommunicator);
    _eventServer->subscribe(_fromConnectorQueueClient, Topic::FreeStackConnector);
    _eventServer->subscribe(_fromConnectorQueueClient, Topic::Rate);
    _eventServer->subscribe(_fromConnectorQueueClient, Topic::SensorWasReset);
    _eventServer->subscribe(_serializer, Topic::SensorData);
}

void Communicator::task(void* parameter) {
    const auto me = static_cast<Communicator*>(parameter);
    for (;;) {
        me->loop();
    }
}
