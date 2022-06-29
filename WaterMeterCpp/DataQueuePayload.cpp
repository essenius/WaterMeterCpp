// Copyright 2021-2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include <cstring>
#include "DataQueuePayload.h"

size_t DataQueuePayload::size() const {

    // optimizing the use of the buffer by not sending unused parts
    size_t size = sizeof(DataQueuePayload) - sizeof(Content);
    switch (topic) {
    case Topic::Result:
        size += sizeof(ResultData);
        break;
    case Topic::Samples:
        size += sizeof Samples::count + buffer.samples.count * sizeof Samples::value[0];
        break;
    case Topic::ConnectionError:
    case Topic::Info:
        size += strlen(buffer.message) + 1;
        break;
    default:
        break;
    }
    return size;
}
