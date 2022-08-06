#include "ExtremeSearcher.h"

void ExtremeSearcher::begin(const float maxNoiseDistance) {
    _maxNoiseDistance = maxNoiseDistance;
    _wasFound = false;
    _foundCandidate = false;
    _extreme = _initValue;
}

bool ExtremeSearcher::isExtreme(const FloatCoordinate sample) const {
    switch(_target) {
    case MaxY:
        return sample.y > _extreme.y;
    case MaxX:
        return sample.x > _extreme.x;
    case MinY:
        return sample.y < _extreme.y;
    case MinX:
        return sample.x < _extreme.x;
    default:
        return false;
    }
}

void ExtremeSearcher::addMeasurement(const FloatCoordinate sample) {
    if (isExtreme(sample)) {
        _extreme = sample;
        _foundCandidate = true;
    } else if (_foundCandidate && _extreme.distanceFrom(sample) > _maxNoiseDistance) {
        _wasFound = true;
        _foundCandidate = false;
    }
}

bool ExtremeSearcher::foundExtreme() const {
    return _wasFound;
}

ExtremeSearcher* ExtremeSearcher::next() const {
    _nextSearcher->begin(_maxNoiseDistance);
    return _nextSearcher;
}

FloatCoordinate ExtremeSearcher::extreme() const {
    return _extreme;
}
