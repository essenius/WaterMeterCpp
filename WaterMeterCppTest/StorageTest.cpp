#include "pch.h"
#include "CppUnitTest.h"
#include "../WaterMeterCpp/Storage.h"
#include <string.h>
#include <iostream>
#include <fstream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

const char FILENAME[] = "storage.txt";

namespace WaterMeterCppTest {

	TEST_CLASS(StorageTest) {
public:


	TEST_METHOD(StorageClearTest) {
		Storage storage;
		storage.clear();
		Assert::IsFalse(fileExists(FILENAME));
		Assert::AreEqual(-1L, storage.getIdleRate());
		Assert::AreEqual(-1L, storage.getNonIdleRate());
		Assert::AreEqual({ 0xFF }, storage.getLogRate());
		Assert::AreEqual(321L, storage.getIdleRate(321L));
		Assert::AreEqual(123L, storage.getNonIdleRate(123L));
		Assert::AreEqual({ 0x12 }, storage.getLogRate(0x12));
	}

	TEST_METHOD(StorageSaveTest) {
		Storage storage;
		storage.clear();
		storage.setIdleRate(6000);
		Assert::IsTrue(fileExists(FILENAME));
		Assert::AreEqual(6000L, storage.getIdleRate());
		Assert::AreEqual(-1L, storage.getNonIdleRate(), L"Non Idle rate @ idle rate save");
		Assert::AreEqual({ 0xFF }, storage.getLogRate());
		storage.setNonIdleRate(100);
		Assert::AreEqual(6000L, storage.getIdleRate());
		Assert::AreEqual(100L, storage.getNonIdleRate());
		Assert::AreEqual({ 0xFF }, storage.getLogRate());
		storage.setLogRate(20);
		Assert::AreEqual(6000L, storage.getIdleRate());
		Assert::AreEqual(100L, storage.getNonIdleRate());
		Assert::AreEqual({ 20 }, storage.getLogRate());
	}

	bool fileExists(const char* fileName) {
		std::ifstream infile(fileName);
		return infile.good();
	}
	};
}