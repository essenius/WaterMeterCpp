#include "pch.h"
#include "CppUnitTest.h"
#include "../WaterMeterCpp/TimeServer.h"
#include <regex>
#include "../WaterMeterCpp/ArduinoMock.h"
#include "../WaterMeterCpp/PubSub.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
	TEST_CLASS(TimeServerTest) {
public:
	EventServer eventServer;

	TEST_METHOD(TimeServerScriptTest) {
		TimeServer timeServer(&eventServer);

		Assert::IsFalse(timeServer.timeWasSet(), L"Time was not set");

		timeServer.begin();
		Assert::IsTrue(timeServer.timeWasSet(), L"Time was set");

		// trigger the get function with Topic::Time
		const char* timestamp = eventServer.request(Topic::Time, "");
		Assert::AreEqual(std::size_t{ 26 }, strlen(timestamp), L"Length of timestamp ok");
		Assert::IsTrue(std::regex_match(
			timestamp, 
			std::regex(R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6})")), 
			L"Time pattern matches");

	    timestamp = timeServer.get(Topic::BatchSize, nullptr);
		Assert::IsNull(timestamp, L"Unexpected topic returns default");

		eventServer.cannotProvide(&timeServer);
		timestamp = eventServer.request(Topic::Time, "");
		Assert::AreEqual("", timestamp, "Time no longer available");

		eventServer.provides(&timeServer, Topic::Time);
		timestamp = eventServer.request(Topic::Time, "");
		Assert::AreNotEqual("", timestamp, "Time filled again");

		eventServer.cannotProvide(&timeServer, Topic::Time);
		timestamp = eventServer.request(Topic::Time, "");
		Assert::AreEqual("", timestamp, "Time no longer available");
	}
	};
}