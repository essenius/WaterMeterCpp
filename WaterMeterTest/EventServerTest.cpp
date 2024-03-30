// Copyright 2021-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include "gtest/gtest.h"
#include "TestEventClient.h"
#include "../WaterMeter/EventServer.h"

namespace WaterMeterCppTest {
       
    TEST(EventServerTest, eventServerDefaultGetTest) {
        EventServer server;
        EventClient client1(&server);
        EXPECT_STREQ("x", client1.get(Topic::ConnectionError, "x")) << "Default get char* OK";
        EXPECT_EQ(25L, client1.get(Topic::Info, 25L)) << "default get long ok";
    }

    TEST(EventServerTest, eventServerErrorTest) {
        EventServer server;
        TestEventClient client1(&server);
        server.subscribe(&client1, Topic::ConnectionError);
        server.publish(nullptr, Topic::ConnectionError, "My Error");
        EXPECT_STREQ("My Error", client1.getPayload()) << "error received";
        EXPECT_EQ(1, client1.getCallCount()) << "one call to client1";
    }

    // ReSharper disable once CyclomaticComplexity -- caused by EXPECT macros
    TEST(EventServerTest, eventServerScriptTest) {
        EventServer server;
        TestEventClient
            client1(&server),
            client2(&server),
            client3(&server);

        // subscribe all clients to topics, ensure some overlap
        server.subscribe(&client1, Topic::BatchSize);
        server.subscribe(&client1, Topic::IdleRate);
        server.subscribe(&client2, Topic::IdleRate);
        server.subscribe(&client3, Topic::IdleRate);
        server.subscribe(&client3, Topic::FreeHeap);

        // Check whether the publication arrives at the correct recipients
        server.publish(&client1, Topic::IdleRate, "Hi");
        EXPECT_EQ(1, client2.getCallCount()) << "client 2 got called";
        EXPECT_EQ(Topic::IdleRate, client2.getTopic()) << "client 2 got a notification on topic idle";
        EXPECT_STREQ("Hi", client2.getPayload()) << "client 2 received the right payload";
        EXPECT_EQ(1, client3.getCallCount()) << "client 3 was notified as well";
        EXPECT_EQ(0, client1.getCallCount()) << "client 1 was not notified (no loopback to sender)";

        // Check whether unsubscribe works OK. Also switch off logging after we established it works.
        client2.reset();
        client3.reset();
        server.unsubscribe(&client1, Topic::IdleRate);

        server.unsubscribe(&client3, Topic::IdleRate);
        server.publish(&client2, Topic::FreeHeap, "Hello");
        EXPECT_EQ(1, client3.getCallCount()) << "client 3 got called";
        EXPECT_EQ(Topic::FreeHeap, client3.getTopic()) << "client 3 got a notification on topic 1";
        EXPECT_STREQ("Hello", client3.getPayload()) << "client 3 received the right payload";
        EXPECT_EQ(0, client1.getCallCount()) << "client 1 was not notified";
        EXPECT_EQ(0, client2.getCallCount()) << "client 2 was not notified";
        client3.reset();
        server.publish(&client3, Topic::BatchSize, R"(Hola)");
        EXPECT_EQ(1, client1.getCallCount()) << "client 1 got called";
        EXPECT_EQ(Topic::BatchSize, client1.getTopic()) << "client 1 got a notification on topic 2";
        EXPECT_STREQ("Hola", client1.getPayload()) << "client 1 received the right payload";
        EXPECT_EQ(0, client2.getCallCount()) << "client 2 was not notified";
        EXPECT_EQ(0, client3.getCallCount()) << "client 3 was not notified";
        client1.reset();

        // subscribing twice still sends one
        server.subscribe(&client1, Topic::BatchSize);
        server.publish(&client2, Topic::BatchSize, "Get once");
        EXPECT_EQ(1, client1.getCallCount()) << "client 1 got called once";
        client1.reset();

        // unsubscribing stops update
        server.unsubscribe(&client2, Topic::IdleRate);
        server.publish(&client3, Topic::IdleRate, "Don't see");
        EXPECT_EQ(0, client2.getCallCount()) << "client 2 did not get called anymore";
        // unsubscribing a topic not subscribed to is handled gracefully
        server.unsubscribe(&client2, Topic::IdleRate);
        server.publish(&client1, Topic::IdleRate, "Still don't see");
        EXPECT_EQ(0, client2.getCallCount()) << "client 2 did not get called after second unsubscribe";
        server.subscribe(&client1, Topic::FreeHeap);
        server.publish(&client2, Topic::FreeHeap, "See twice");
        EXPECT_EQ(1, client1.getCallCount()) << "Two subscribed clients - client1";
        EXPECT_EQ(1, client3.getCallCount()) << "Two subscribed clients - client3";
        client1.reset();
        client3.reset();
        // unsubscribe all works
        server.unsubscribe(&client1);
        server.publish(&client2, Topic::FreeHeap, 1L);
        server.publish(&client3, Topic::BatchSize, 0L);
        EXPECT_EQ(0, client1.getCallCount()) << "Client 1 not subscribed anymore";
        EXPECT_EQ(1, client3.getCallCount()) << "client3 still subscribed";
    }
}
