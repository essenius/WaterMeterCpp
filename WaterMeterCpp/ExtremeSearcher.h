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

#ifndef HEADER_EXTREMESEARCHER
#define HEADER_EXTREMESEARCHER

#include "EventClient.h"

enum SearchTarget : uint8_t {
    None = 0,
    MaxY,
    MaxX,
    MinY,
    MinX
};

class ExtremeSearcher {
public:
    ExtremeSearcher(const SearchTarget target, const FloatCoordinate initValue, const float noiseThreshold, ExtremeSearcher* next) :
    _target(target), _initValue(initValue), _extreme(initValue), _noiseThreshold(noiseThreshold), _nextSearcher(next) {}
    void reset();
    bool isExtreme(FloatCoordinate sample) const;
    void addMeasurement(FloatCoordinate sample);
    bool foundExtreme() const;
    ExtremeSearcher* next() const;
    FloatCoordinate extreme() const;
    SearchTarget target() const { return _target; }

private:
	bool _foundCandidate = false;
	bool _wasFound = false;
    SearchTarget _target;
    FloatCoordinate _initValue;
    FloatCoordinate _extreme;
    float _noiseThreshold;
    ExtremeSearcher* _nextSearcher;
};

#endif