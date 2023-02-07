#include <gtest/gtest.h>
#include <corecrt_math_defines.h>

#include <fstream>
#include "../WaterMeterCpp/FlowDetector.h"
#include "FlowDetectorDriver.h"
#include "PulseTestEventClient.h"
#include "TestHelper.h"

namespace WaterMeterCppTest {
	class FlowDetectorTest : public testing::Test {
	public:
		static EventServer eventServer;
		static EllipseFit ellipseFit;
	protected:
		void expectResult(const FlowDetector* meter, const wchar_t* description, const int index,
			const bool pulse = false, const bool skipped = false, const bool outlier = false) const {
			std::wstring message(description);
			message += std::wstring(L" #") + std::to_wstring(index) + std::wstring(L" @ ");
			EXPECT_EQ(pulse, meter->foundPulse()) << message << "Pulse";
			EXPECT_EQ(skipped, meter->wasSkipped()) << message << "Skipped";
			EXPECT_EQ(outlier, meter->foundAnomaly()) << message << "Anomaly";
		}

		IntCoordinate getSample(const double sampleNumber, const double samplesPerCycle = 32,
			const double angleOffsetSamples = 0) const {
			constexpr double RADIUS = 10.0L;
			constexpr int16_t X_OFFSET = -100;
			constexpr int16_t Y_OFFSET = 100;
			const double angle = (sampleNumber - angleOffsetSamples) * M_PI / samplesPerCycle * 2.0;
			return IntCoordinate{
				{
					static_cast<int16_t>(X_OFFSET + round(sin(angle) * RADIUS)),
					static_cast<int16_t>(Y_OFFSET + round(cos(angle) * RADIUS))
				}
			};
		}

		void expectFlowAnalysis(
			const FlowDetectorDriver* actual,
			const std::string& message,
			const int index,
			const IntCoordinate movingAverage,
			const bool flowStarted = false,
			const bool isPulse = false) const {
			assertIntCoordinatesEqual(movingAverage, actual->_movingAverageArray[index], message + " Moving average #" + std::to_string(index));
			EXPECT_EQ(flowStarted, actual->_justStarted) << message << ": just started";
			EXPECT_EQ(isPulse, actual->_foundPulse) << message << ": pulse detected";

		}

		// run process on test signals with a known number of pulses
		void flowTestWithFile(const char* fileName, const unsigned int firstPulses = 0, const unsigned int nextPulses = 0, const unsigned int anomalies = 0, const unsigned int noFits = 0, const unsigned int noiseLimit = 3) const {
			FlowDetector flowDetector(&eventServer, &ellipseFit);
			const PulseTestEventClient pulseClient(&eventServer);
			flowDetector.begin(noiseLimit);
			IntCoordinate measurement{};
			std::ifstream measurements(fileName);
			EXPECT_TRUE(measurements.is_open()) << "File open";
			measurements.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			while (measurements >> measurement.x) {
				measurements >> measurement.y;
				eventServer.publish(Topic::Sample, measurement);
			}

			EXPECT_EQ(firstPulses, pulseClient.pulses(false)) << "First Pulses";
			EXPECT_EQ(nextPulses, pulseClient.pulses(true)) << "Next Pulses";
			EXPECT_EQ(anomalies, pulseClient.anomalies()) << "Anomalies";
			EXPECT_EQ(noFits, pulseClient.noFits()) << "NoFits";
		}
	};

	EventServer FlowDetectorTest::eventServer;
	EllipseFit FlowDetectorTest::ellipseFit;

	TEST_F(FlowDetectorTest, BiQuadrantTest) {
		// Tests all cases where a quadrant may be skipped close to detection of a pulse or a search start
		// First a circle for fitting, then 12 samples that check whether skipping a quadrant is dealt with right
		// Test is similar to flowTestWithFile, but bypasses the moving average generation for simpler testing.
		FlowDetectorDriver flowDetector(&eventServer, &ellipseFit);
		const PulseTestEventClient pulseClient(&eventServer);
		flowDetector.begin(2);
		Coordinate measurement{};
		std::ifstream measurements("BiQuadrant.txt");
		EXPECT_TRUE(measurements.is_open()) << "File open";
		measurements.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		while (measurements >> measurement.x) {
			measurements >> measurement.y;
			flowDetector.processMovingAverageSample(measurement);
		}

		EXPECT_EQ(1, pulseClient.pulses(false)) << "One pulse from the circle";
		EXPECT_EQ(4, pulseClient.pulses(true)) << "4 pulses from the crafted test samples";
		EXPECT_EQ(0, pulseClient.anomalies()) << "No anomalies";
		EXPECT_EQ(0, pulseClient.noFits()) << "Fit worked";
	}

	TEST_F(FlowDetectorTest, VerySlowFlowTest) {
		flowTestWithFile("verySlow.txt", 1,0, 0);
	}

	TEST_F(FlowDetectorTest, NoFlowTest) {
		flowTestWithFile("noise.txt", 0, 0, 0);
	}

	TEST_F(FlowDetectorTest, FastFlowTest) {
		flowTestWithFile("fast.txt", 2, 75, 0);
	}

	TEST_F(FlowDetectorTest, SlowFastFlowTest) {
		flowTestWithFile("slowFast.txt", 1, 11, 0);
	}
	TEST_F(FlowDetectorTest, SlowFlowTest) {
		flowTestWithFile("slow.txt", 1, 1, 0);
	}

	TEST_F(FlowDetectorTest, SlowestFlowTest) {
		flowTestWithFile("slowest.txt", 1, 0, 0);
	}

	TEST_F(FlowDetectorTest, FastFlowThenNoisyTest) {
		// should not trigger on the noisy data
		flowTestWithFile("fastThenNoisy.txt", 2, 3, 0, 0, 12);
	}

	TEST_F(FlowDetectorTest, AnomalyTest) {
		// should not trigger on non-typical movement
		flowTestWithFile("anomaly.txt", 1, 3, 661);
	}

	TEST_F(FlowDetectorTest, 60CyclesTest) {
		flowTestWithFile("60cycles.txt", 1, 59, 0);
	}

	TEST_F(FlowDetectorTest, NoiseAtEndTest) {
		flowTestWithFile("noiseAtEnd.txt", 1, 5, 0);
	}

	TEST_F(FlowDetectorTest, NoFitTest) {
		flowTestWithFile("forceNoFit.txt", 1, 0, 0, 1);
	}

	TEST_F(FlowDetectorTest, FlushTest) {
		flowTestWithFile("flush.txt", 2, 37, 325, 0, 11);
	}

	TEST_F(FlowDetectorTest, WrongOutlierTest) {
		flowTestWithFile("wrong outliers.txt", 1, 61, 10, 0, 3);
	}


	TEST_F(FlowDetectorTest, SensorWasResetTest) {
		FlowDetector flowDetector(&eventServer, &ellipseFit);
		flowDetector.begin(3);
		constexpr int RADIUS = 20;
		for (int pass = 0; pass < 2; pass++) {
			EXPECT_TRUE(flowDetector.wasReset()) << "Flow detector reset before adding samples pass " << pass;
			unsigned int skipped = 0;

			for (int i = 0; i < 30; i++) {
				const double angle = i * M_PI / 16.0;
				eventServer.publish(Topic::Sample, IntCoordinate{ {static_cast <int16_t>(cos(angle) * RADIUS), static_cast <int16_t>(sin(angle) * RADIUS)} });
				if (flowDetector.wasSkipped()) skipped++;
			}

			EXPECT_EQ(8u, skipped) << "8 values skipped pass" << pass;
			skipped = 0;
			EXPECT_FALSE(flowDetector.wasReset()) << "Flow detector not reset after adding samples pass " << pass;
			if (pass == 0) {
				eventServer.publish(Topic::SensorWasReset, true);
			}
		}
		eventServer.publish(Topic::Sample, IntCoordinate{ {RADIUS, 0} });
		EXPECT_FALSE(flowDetector.wasReset()) << "Flow detector not reset at end";
		EXPECT_FALSE(flowDetector.wasSkipped()) << "sample not skipped at end";
	}

	TEST_F(FlowDetectorTest, SaturatedValuesIgnoredTest) {
		FlowDetector flowDetector(&eventServer, &ellipseFit);
		flowDetector.begin(3);
		eventServer.publish(Topic::Sample, IntCoordinate{{-32768, 32767}});
		ASSERT_TRUE(flowDetector.wasReset());
		eventServer.publish(Topic::Sample, IntCoordinate{{0, -32768}});
		ASSERT_TRUE(flowDetector.wasReset());
		eventServer.publish(Topic::Sample, IntCoordinate{{32767, 0}});
		ASSERT_TRUE(flowDetector.wasReset());
		eventServer.publish(Topic::Sample, IntCoordinate{ {-32768, 0} });
		ASSERT_TRUE(flowDetector.wasReset());
	}
}