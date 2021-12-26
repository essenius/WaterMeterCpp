#ifndef HEADER_CONNECTIONSTATUS
#define HEADER_CONNECTIONSTATUS

#include "PubSub.h"

class BinaryStatusPublisher
{
public:
    BinaryStatusPublisher(EventServer* eventServer, EventClient* eventClient, Topic offTopic, Topic onTopic);
    bool setState(bool state);
    bool getState() { return _state; }
private:
    EventServer* _eventServer;
    EventClient* _eventClient;
    bool _state = false;
    Topic _offTopic;
    Topic _onTopic;
};

#endif