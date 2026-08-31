#include <gtest/gtest.h>
#include "WebSockets/TelemetryWebSocketHandler.h"
#include "Core/AnalysisService.h"
#include "Core/EventService.h"
#include "Mocks/MockLlamaModelProvider.h"
#include "Mocks/MockTelemetryPushNotifier.h"
#include "System/Console.h"
#include "System/Threading/Thread.h"

using namespace DotNetDupe::System;
using namespace DotNetDupe::System::Threading;
using namespace SmartEventViewer;
using namespace SmartEventViewer::Tests;

template<typename T>
using SmartPtr = DotNetDupe::System::SmartPointer<T>;

TEST(PushNotificationTests, GivenTelemetryWebSocketHandler_WhenClientCountQueried_ThenReturnsInitialZero) {
    TelemetryWebSocketHandler handler;
    EXPECT_EQ(handler.GetConnectedClientCount(), 0);
}

TEST(PushNotificationTests, GivenTelemetryWebSocketHandler_WhenBroadcastCalledWithNoClients_ThenExecutesSafely) {
    TelemetryWebSocketHandler handler;
    EXPECT_NO_THROW(handler.BroadcastCategoryUpdate("summary"));
    EXPECT_NO_THROW(handler.BroadcastCategoryUpdate("llm_analysis"));
    EXPECT_EQ(handler.GetConnectedClientCount(), 0);
}

TEST(PushNotificationTests, GivenTelemetryWebSocketHandler_WhenNullContextEventsReceived_ThenHandlesSafely) {
    TelemetryWebSocketHandler handler;
    EXPECT_NO_THROW(handler.OnConnected(nullptr));
    EXPECT_NO_THROW(handler.OnMessage(nullptr, "PING"));
    EXPECT_NO_THROW(handler.OnDisconnected(nullptr));
    EXPECT_EQ(handler.GetConnectedClientCount(), 0);
}

TEST(PushNotificationTests, GivenMockNotifier_WhenAnalysisTaskEnqueued_ThenBroadcastsLlmAnalysisPushEvent) {
    auto spMockProvider = SmartPtr<ILlamaModelProvider>(SmartPtr<MockLlamaModelProvider>::NewShared());
    auto spLlm = SmartPtr<LocalLlmEngine>::NewShared(spMockProvider);
    auto spEvents = SmartPtr<IEventService>(SmartPtr<EventService>::NewShared());
    auto spNotifier = SmartPtr<MockTelemetryPushNotifier>::NewShared();

    AnalysisService service(spLlm, spEvents, spNotifier);

    AnalyzeRequestDto req;
    req.Channel = "Security";
    req.Query = "Detect unauthorized lateral movement";
    auto pending = service.EnqueueTask(req);

    EXPECT_FALSE(pending.TaskId.IsEmpty());
    EXPECT_TRUE(spNotifier->GetBroadcastHistory().Contains("llm_analysis"));
}
