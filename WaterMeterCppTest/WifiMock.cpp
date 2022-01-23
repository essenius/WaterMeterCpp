#include "pch.h"
#include "WifiMock.h"
WifiMock::WifiMock(EventServer* eventServer): Wifi(eventServer, &_config) {}

