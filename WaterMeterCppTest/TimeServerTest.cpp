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
		Assert::IsTrue(timeServer.setTime(), L"Time set");
		Assert::IsTrue(timeServer.timeWasSet(), L"Time was set");

		const char* time = timeServer.get(Topic::Time, nullptr);
		Assert::AreEqual(std::size_t{ 26 }, strlen(time), L"Length of timestamp ok");
		Assert::IsTrue(std::regex_match(time, std::regex(R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}.\d{6})")), L"Time pattern matches");
		time = timeServer.get(Topic::BatchSize, nullptr);
		Assert::IsNull(time, L"Unexpected topic returns default");
	}
	};
}