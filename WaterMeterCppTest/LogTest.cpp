#include "pch.h"

#include <regex>

#include "CppUnitTest.h"
#include "../WaterMeterCpp/Log.h"
#include "../WaterMeterCpp/PubSub.h"
#include "../WaterMeterCpp/TimeServer.h"
#include "../WaterMeterCpp/ArduinoMock.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
	TEST_CLASS(LogTest) {
public:
	EventServer eventServer;

	TEST_METHOD(LogScriptTest) {
		Log log(&eventServer);
		TimeServer timeServer(&eventServer);
		timeServer.begin();
		log.begin();
		Serial.begin(9600);
		eventServer.publish(Topic::Connected, 0);

		Assert::IsTrue(std::regex_match(
			Serial.getOutput(),
			std::regex(R"(\[\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}\]\sConnected\n)")),
			L"Connected logs OK");

		Serial.clearOutput();
		eventServer.publish(Topic::Error, "My Message");
		Assert::IsTrue(std::regex_match(
			Serial.getOutput(),
			std::regex(R"(\[.*\]\sError:\sMy\sMessage\n)")), 
			L"Error logs OK");
		Serial.clearOutput();

	    eventServer.publish(Topic::Disconnected, 24);
		Assert::IsTrue(std::regex_match(
			Serial.getOutput(),
			std::regex(R"(\[.*\]\sDisconnected\n)")), 
			L"Disconnected logs OK");

		Serial.clearOutput();

		Assert::AreEqual("", Serial.getOutput());

		eventServer.publish(Topic::Info, 24);
		Assert::IsTrue(std::regex_match(
			Serial.getOutput(),
			std::regex(R"(\[.*\]\sInfo:\s24\n)")), 
			L"Info logs long OK");
		Serial.clearOutput();

		log.update(Topic::BatchSize, 24L);

		Assert::IsTrue(std::regex_match(
			Serial.getOutput(),
			std::regex(R"(\[.*\]\sUpexpected topic\s'1',\spayload\s'24'\n)")), L"Unexpected topic handled OK");
		Serial.clearOutput();
	}
	};
}