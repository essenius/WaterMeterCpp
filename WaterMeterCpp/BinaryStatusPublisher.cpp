#include "BinaryStatusPublisher.h"

BinaryStatusPublisher::BinaryStatusPublisher(EventServer* eventServer, EventClient* eventClient, Topic offTopic, Topic onTopic) {
    _eventServer = eventServer;
    _eventClient = eventClient;
    _offTopic = offTopic;
    _onTopic = onTopic;
}

bool BinaryStatusPublisher::setState(bool state) {
    if (state != _state) {
        _state = state;
        _eventServer->publish(_eventClient, _state ? _onTopic : _offTopic, 1);
    }
    return _state;
}

