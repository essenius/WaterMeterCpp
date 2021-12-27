#include "pch.h"

#include "CppUnitTest.h"
#include "TestEventClient.h"
#include "../WaterMeterCpp/PubSub.h"
#include "TopicHelper.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(PubSubTest) {
public:

    TEST_METHOD(pubSubScriptTest) {
        EventServer server(LogLevel::On);
        TestEventClient
            client1("Client1", &server),
            client2("Client2", &server),
            client3("Client3", &server),
            logger("Logger", &server);
        server.subscribe(&logger, Topic::Info);
        Assert::AreEqual("", logger.getPayload(), L"No log just after subscribing");

        // subscribe all clients to topics, ensure some overlap
        server.subscribe(&client1, Topic::BatchSize);
        Assert::AreEqual("Client1 subscribes to 1\n", logger.getPayload(), L"log 1 subscribes to batch");
        server.subscribe(&client1, Topic::IdleRate);
        Assert::AreEqual("Client1 subscribes to 4\n", logger.getPayload(), L"log 1 subscribes to idle");
        server.subscribe(&client2, Topic::IdleRate);
        server.subscribe(&client3, Topic::IdleRate);
        server.subscribe(&client3, Topic::FreeHeap);

        // Check whether the publication arrives at the correct recipients
        logger.reset();
        server.publish(&client1, Topic::IdleRate, "Hi");
        Assert::AreEqual("Updating Client3 on 4 with 'Hi'\n", logger.getPayload(), L"log 1 updates idle");
        Assert::AreEqual(2, logger.getCallCount(), L"Logger called twice");
        Assert::AreEqual(1, client2.getCallCount(), L"client 2 got called");
        Assert::AreEqual(Topic::IdleRate, client2.getTopic(), L"client 2 got a notification on topic idle");
        Assert::AreEqual("Hi", client2.getPayload(), L"client 2 received the right payload");
        Assert::AreEqual(1, client3.getCallCount(), L"client 3 was notified as well");
        Assert::AreEqual(0, client1.getCallCount(), L"client 1 was not notified (no loopback to sender)");

        // Check whether unsubscribe works OK. Also switch off logging after we established it works.
        client2.reset();
        client3.reset();
        server.unsubscribe(&client1, Topic::IdleRate);
        Assert::AreEqual("Client1 unsubscribes from 4\n", logger.getPayload(), L"log 1 unsubscribes from idle");
        logger.reset();
        server.setLogLevel(LogLevel::Off);

        server.unsubscribe(&client3, Topic::IdleRate);
        Assert::AreEqual("", logger.getPayload(), L"no unsubscribe log with log level off");
        server.publish(&client2, Topic::FreeHeap, "Hello");
        Assert::AreEqual("", logger.getPayload(), L"no publish log with log level off");
        Assert::AreEqual(1, client3.getCallCount(), L"client 3 got called");
        Assert::AreEqual(Topic::FreeHeap, client3.getTopic(), L"client 3 got a notification on topic 1");
        Assert::AreEqual("Hello", client3.getPayload(), L"client 3 received the right payload");
        Assert::AreEqual(0, client1.getCallCount(), L"client 1 was not notified");
        Assert::AreEqual(0, client2.getCallCount(), L"client 2 was not notified");
        client3.reset();
        server.publish(&client3, Topic::BatchSize, "Hola");
        Assert::AreEqual(1, client1.getCallCount(), L"client 1 got called");
        Assert::AreEqual(Topic::BatchSize, client1.getTopic(), L"client 1 got a notification on topic 2");
        Assert::AreEqual("Hola", client1.getPayload(), L"client 1 received the right payload");
        Assert::AreEqual(0, client2.getCallCount(), L"client 2 was not notified");
        Assert::AreEqual(0, client3.getCallCount(), L"client 3 was not notified");
        client1.reset();

        // subscribing twice still sends one
        server.subscribe(&client1, Topic::BatchSize);
        Assert::AreEqual("", logger.getPayload(), L"no subscribe log with log level off");
        server.publish(&client2, Topic::BatchSize, "Get once");
        Assert::AreEqual(1, client1.getCallCount(), L"client 1 got called once");
        client1.reset();

        // unsubscribing stops update
        server.unsubscribe(&client2, Topic::IdleRate);
        server.publish(&client3, Topic::IdleRate, "Don't see");
        Assert::AreEqual(0, client2.getCallCount(), L"client 2 did not get called anymore");
        // unsubscribing a topic not subscribed to is handled gracefully
        server.unsubscribe(&client2, Topic::IdleRate);
        server.publish(&client1, Topic::IdleRate, "Still don't see");
        Assert::AreEqual(0, client2.getCallCount(), L"client 2 did not get called after second unsubscribe");
        server.subscribe(&client1, Topic::FreeHeap);
        server.publish(&client2, Topic::FreeHeap, "See twice");
        Assert::AreEqual(1, client1.getCallCount(), L"Two subscribed clients - client1");
        Assert::AreEqual(1, client3.getCallCount(), L"Two subscribed clients - client3");
        client1.reset();
        client3.reset();
        // unsubscribe all works
        server.unsubscribe(&client1);
        server.publish(&client2, Topic::FreeHeap, 1L);
        server.publish(&client3, Topic::BatchSize, 0L);
        Assert::AreEqual(0, client1.getCallCount(), L"Client 1 not subscribed anymore");
        Assert::AreEqual(1, client3.getCallCount(), L"client3 still subscribed");
    }

    TEST_METHOD(pubSubErrorTest) {
        EventServer server(LogLevel::Off);
        TestEventClient
            client1("Client1", &server),
            logger("Logger", &server);
        server.subscribe(&logger, Topic::Info);
        Assert::AreEqual("", logger.getPayload(), L"logger off");
        server.subscribe(&client1, Topic::Error);
        server.setLogLevel(LogLevel::On);
        server.publish(nullptr, Topic::Error, "My Error");
        Assert::AreEqual("Updating Client1 on 17 with 'My Error'\n", logger.getPayload(), L"error received");
        Assert::AreEqual("My Error", client1.getPayload(), L"error received");
        Assert::AreEqual(1, logger.getCallCount(), L"one call to logger");
        Assert::AreEqual(1, client1.getCallCount(), L"one call to client1");
    }

    TEST_METHOD(pubSubDefaultGetTest) {
        EventServer server(LogLevel::Off);
        EventClient client1("Client1", &server);
        Assert::AreEqual("x", client1.get(Topic::Error, "x"), L"Default get char* OK");
        Assert::AreEqual(25L, client1.get(Topic::Info, 25L), L"default get long ok");
    }
    };
}