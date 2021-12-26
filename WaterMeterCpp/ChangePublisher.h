#ifndef HEADER_CHANGEPUBLISHER
#define HEADER_CHANGEPUBLISHER

#include "PubSub.h"

template<class payloadType>
class ChangePublisher{
public:
    ChangePublisher(EventServer* eventServer, EventClient* eventClient, Topic topic) {
        _eventServer = eventServer;
        _eventClient = eventClient;
        _topic = topic;
    }

    payloadType value() { return _payload; }

    payloadType update(payloadType payload) {
        if (payload != _payload) {
            _payload = payload;
            _eventServer->publish<payloadType>(_eventClient, _topic, _payload);
        }
        return _payload;
    }

    void reset() { _payload = payloadType();  }
    void setTopic(Topic topic) { _topic = topic; }

private:
    EventServer* _eventServer;
    EventClient* _eventClient;
    payloadType _payload{};
    Topic _topic;
};

#endif