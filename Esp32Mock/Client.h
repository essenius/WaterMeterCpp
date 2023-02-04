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

// Mock implementation of the network client driver for unit testing (not targeting the ESP32)

#ifndef HEADER_CLIENT_H
#define HEADER_CLIENT_H

class Client {
public:
    Client() = default;
    Client(const Client&) = default;
    Client(Client&&) = default;
    Client& operator=(const Client&) = default;
    Client& operator=(Client&&) = default;
    virtual ~Client() = default;
    virtual const char* getType() { return "client"; }
};

#endif
