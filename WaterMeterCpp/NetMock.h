// Copyright 2021 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#ifndef ESP32

#ifndef HEADER_NETMOCK_H
#define HEADER_NETMOCK_H


#include "ArduinoMock.h"

class Client {};

class WiFiClient: public Client {
public:
	WiFiClient() {}
	bool isConnected() {
		return true;
	}
};

class WiFiClientSecure : public WiFiClient {
public:
	void setCACert(const char* cert) {}
	void setCertificate(const char* cert) {}
	void setPrivateKey(const char* cert) {}
};

class String {
public:
	String(const char* value) { strcpy(_value, value); }
	int toInt() { return atoi(_value); }
	const char* c_str() { return _value; }

private:
	char _value[30];
};

class HTTPClient {
public:
	HTTPClient() {}
	void end() {}
	bool begin(WiFiClient& client, const char* url) { return true; }
	int GET() { return ReturnValue;  };
    String getString() { return {"1"}; }
	static int ReturnValue;
};

#define HTTP_UPDATE_FAILED 0
#define HTTP_UPDATE_NO_UPDATES 1
#define HTTP_UPDATE_OK 2

typedef int t_httpUpdate_return;

class HTTPUpdate {
public:
	t_httpUpdate_return update(WiFiClient& client, const char* url) { return ReturnValue; }
	int getLastError() { return 0; }
	String getLastErrorString() { return String("OK"); }
	static int ReturnValue;
};

extern HTTPUpdate httpUpdate;

class IPAddress {
public:
	IPAddress(uint8_t oct1, uint8_t oct2, uint8_t oct3, uint8_t oct4);
	String toString() { return _value; }
private:
	char _value[20];
};

class WiFiClass {
public:
	WiFiClass();
	void mode(int i) {}
	void begin(const char* ssid, const char* password) { strcpy(_ssid, ssid); }
	bool config(IPAddress localIP, IPAddress gateway, IPAddress subnet, IPAddress primaryDNS) { return true; }
	bool isConnected() { return true; }
	bool setHostname(const char* name);
	const char* getHostname() { return _name; }
	String SSID() { return String(_ssid); }
	String macAddress();
	void macAddress(uint8_t *mac) { memcpy(mac, _mac, 6); }
	int RSSI() { return 1; }
	int channel() { return 13; }
	IPAddress networkID() { return IPAddress(192,168, 1, 0); }
	IPAddress localIP() { return IPAddress(127, 0, 0, 1); }
	IPAddress gatewayIP() { return IPAddress(192, 168, 1, 1); }
	IPAddress dnsIP() { return IPAddress(1, 1, 1, 1); }
	IPAddress subnetMask() { return IPAddress(255, 255, 255, 0); }
	String BSSIDstr() { return String("55:44:33:22:11:00"); }

private:
	char _name[20];
	char _ssid[20];
	byte _mac[6];
};

#define WIFI_STA 1

extern WiFiClass WiFi;

#endif

#endif

