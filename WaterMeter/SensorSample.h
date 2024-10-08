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

// The sensors deliver 16 bit int coordinates. So X-Y fits in a 32 bit payload, which we use for events.
// We also model saturation here, using the extreme values of 16-bit integers.
// Sensors are responsible to provide their extremes this way.

#ifndef HEADER_INT_COORDINATE
#define HEADER_INT_COORDINATE

#include <Coordinate.h>
#include <climits>

using EllipseMath::Coordinate;

namespace WaterMeter {
    enum class SensorState : int8_t {
        None = 0,
        Ok,
        PowerError,
        BeginError,
        ReadError,
        Saturated,
        NeedsHardReset,
        NeedsSoftReset,
        Resetting,
        FlatLine,
        Outlier
    };

    union SensorSample {
        struct {
            int16_t x;
            int16_t y;
        };

        long l;

        bool operator==(const SensorSample& other) const {
            return x == other.x && y == other.y;
        }

        void set(const int16_t xIn, const int16_t yIn) {
            x = xIn;
            y = yIn;
        }

        // we need this to pass coordinates in a memory efficient way but still keep reasonable accuracy

        static SensorSample times10(const Coordinate& input) {
            return {{static_cast<int16_t>(input.x * 10), static_cast<int16_t>(input.y * 10)}};
        }

        double getDistanceFrom(const SensorSample other) const {
            return toCoordinate().getDistanceFrom(other.toCoordinate());
        }

        Coordinate toCoordinate() const {
            return {static_cast<double>(x), static_cast<double>(y)};
        }

        // We reserve SHRT_MIN to indicate saturated values
        // We can't use another field for quality, because we need to transport the coordinate in 32 bits.
        bool isSaturated() const {
            return x == SHRT_MIN || y == SHRT_MIN;
        }

        // x = SHRT_MAX indicates an error (neither QMC nor HMC delivers this as a valid value)
        // the Y value delivers the error code
        SensorState state() const {
            if (x == SHRT_MAX) {
                return static_cast<SensorState>(y);
            }
            return isSaturated() ? SensorState::Saturated : SensorState::Ok;
        }

        static const char* stateToString(const SensorState state) {
            switch (state) {
                case SensorState::None:
                    return "None";
                case SensorState::Ok:
                    return "Ok";
                case SensorState::PowerError:
                    return "PowerError";
                case SensorState::BeginError:
                    return "BeginError";
                case SensorState::ReadError:
                    return "ReadError";
                case SensorState::Saturated:
                    return "Saturated";
                case SensorState::NeedsHardReset:
                    return "NeedsHardReset";
                case SensorState::NeedsSoftReset:
                    return "NeedsSoftReset";
                case SensorState::Resetting:
                    return "Resetting";
                case SensorState::FlatLine:
                    return "FlatLine";
                case SensorState::Outlier:
                    return "Outlier";
                default:
                    return "Unknown";
            }
        }

        static SensorSample error(const SensorState error) {
            return SensorSample{SHRT_MAX, static_cast<int16_t>(error)};
        }
    };
}
#endif
