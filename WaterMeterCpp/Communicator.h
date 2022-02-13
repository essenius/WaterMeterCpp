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

#ifndef HEADER_COMMUNICATOR
#define HEADER_COMMUNICATOR
#include "DataQueue.h"
#include "Device.h"
#include "LedDriver.h"
#include "Log.h"
#include "QueueClient.h"
#include "Serializer.h"

class Communicator : public EventClient {
public:
    Communicator(EventServer* eventServer, Log* logger, LedDriver* ledDriver, Device* device, 
        DataQueue* dataQueue, Serializer* serializer, QueueClient* fromSamplerQueueClient, QueueClient* fromConnectorQueueClient);
    void loop() const;
    void setup() const;
    static void task(void* parameter);

private:
    Log* _logger;
    LedDriver* _ledDriver;
    Device* _device;
    DataQueue* _dataQueue;
    Serializer* _serializer;
    QueueClient* _samplerQueueClient;
    QueueClient* _connectorQueueClient;
};
#endif
