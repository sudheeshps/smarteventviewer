#include <gtest/gtest.h>
#include "TestRestClient.h"
#include "System/Console.h"

using namespace DotNetDupe::System;
using namespace SmartEventViewer;
using namespace SmartEventViewer::IntegrationTests;

static void LogAndAssertEventDto(const EventDto& evt, const String& sExpectedLevel) {
    Console::WriteLine("  [EVENT_DTO] Id={0} | Level={1} | Risk={2} | Provider={3} | Time={4}",
        evt.Id, evt.Level, evt.Risk, evt.Provider, evt.Time);
    Console::WriteLine("  [ASSERT] Id >= 0 (Actual: {0})", evt.Id);
    EXPECT_GE(evt.Id, 0U);
    if (!sExpectedLevel.IsEmpty()) {
        Console::WriteLine("  [ASSERT] Level == '{0}' (Actual: '{1}')", sExpectedLevel, evt.Level);
        EXPECT_EQ(evt.Level, sExpectedLevel);
    }
    EXPECT_FALSE(evt.Level.IsEmpty());
    EXPECT_FALSE(evt.Risk.IsEmpty());
    EXPECT_FALSE(evt.Provider.IsEmpty());
    EXPECT_FALSE(evt.Time.IsEmpty());
    EXPECT_FALSE(evt.Message.IsEmpty());
}

static void LogAndAssertEventSummary(const EventSummaryResponseDto& resp, const String& sExpectedChannel) {
    Console::WriteLine("[SUMMARY_DTO] Channel={0} | Total={1} | Critical={2} | Error={3} | Warning={4} | Info={5} | Verbose={6}",
        resp.Channel, resp.TotalCount, resp.CriticalCount, resp.ErrorCount, resp.WarningCount, resp.InfoCount, resp.VerboseCount);
    Console::WriteLine("[ASSERT] Channel == '{0}' (Actual: '{1}')", sExpectedChannel, resp.Channel);
    EXPECT_EQ(resp.Channel, sExpectedChannel);
    unsigned long long uSum = resp.CriticalCount + resp.ErrorCount + resp.WarningCount + resp.InfoCount + resp.VerboseCount;
    Console::WriteLine("[ASSERT] TotalCount >= LevelSum (Total: {0}, Sum: {1})", resp.TotalCount, uSum);
    EXPECT_GE(resp.TotalCount, uSum);
    EXPECT_GE(resp.CriticalCount, 0ULL);
    EXPECT_GE(resp.ErrorCount, 0ULL);
    EXPECT_GE(resp.WarningCount, 0ULL);
    EXPECT_GE(resp.InfoCount, 0ULL);
    EXPECT_GE(resp.VerboseCount, 0ULL);
}

static void LogAndAssertEventLog(const EventLogResponseDto& resp, const String& sChannel, size_t nPage, size_t nPageSize) {
    Console::WriteLine("[EVENT_LOG_DTO] Channel={0} | Total={1} | Page={2}/{3} | PageSize={4} | ReturnedCount={5}",
        resp.Channel, resp.TotalCount, resp.Page, resp.TotalPages, resp.PageSize, resp.Events.GetCount());
    Console::WriteLine("[ASSERT] Channel == '{0}' | Page == {1} | PageSize == {2}", sChannel, nPage, nPageSize);
    EXPECT_EQ(resp.Channel, sChannel);
    EXPECT_GE(resp.TotalCount, 0ULL);
    EXPECT_EQ(resp.Page, nPage);
    EXPECT_EQ(resp.PageSize, nPageSize);
    EXPECT_GE(resp.TotalPages, 0ULL);
    EXPECT_LE(resp.Events.GetCount(), nPageSize);
}

TEST(EventsControllerTests, GivenRunningServer_WhenGetChannelsCalled_ThenReturnsAvailableChannels) {
    Console::WriteLine("[TEST] Invoking GET /api/channels...");
    TestRestClient client;
    ChannelsResponseDto response = client.GetChannels();
    Console::WriteLine("[CHANNELS_DTO] Count={0}", response.Channels.GetCount());
    Console::WriteLine("[ASSERT] Channels.Count > 0 && Contains 'Application' and 'System'");
    EXPECT_GT(response.Channels.GetCount(), 0);
    EXPECT_TRUE(response.Channels.Contains("Application"));
    EXPECT_TRUE(response.Channels.Contains("System"));
}

TEST(EventsControllerTests, GivenApplicationChannel_WhenGetEventSummaryCalled_ThenReturnsChannelSummary) {
    Console::WriteLine("[TEST] Invoking GET /api/events/summary?channel=Application...");
    TestRestClient client;
    EventSummaryResponseDto response = client.GetEventSummary("Application");
    LogAndAssertEventSummary(response, "Application");
}

TEST(EventsControllerTests, GivenSystemChannel_WhenGetEventSummaryCalled_ThenReturnsChannelSummary) {
    Console::WriteLine("[TEST] Invoking GET /api/events/summary?channel=System...");
    TestRestClient client;
    EventSummaryResponseDto response = client.GetEventSummary("System");
    LogAndAssertEventSummary(response, "System");
}

TEST(EventsControllerTests, GivenEmptyChannel_WhenGetEventSummaryCalled_ThenReturnsAggregatedSummary) {
    Console::WriteLine("[TEST] Invoking GET /api/events/summary (empty channel)...");
    TestRestClient client;
    EventSummaryResponseDto response = client.GetEventSummary("");
    EXPECT_FALSE(response.Channel.IsEmpty());
    LogAndAssertEventSummary(response, response.Channel);
}

TEST(EventsControllerTests, GivenInvalidChannel_WhenGetEventSummaryCalled_ThenHandlesGracefully) {
    Console::WriteLine("[TEST] Invoking GET /api/events/summary?channel=NonExistentChannel_XYZ_12345...");
    TestRestClient client;
    EventSummaryResponseDto response = client.GetEventSummary("NonExistentChannel_XYZ_12345");
    LogAndAssertEventSummary(response, "NonExistentChannel_XYZ_12345");
}

TEST(EventsControllerTests, GivenApplicationChannel_WhenQueryingCriticalLevel_ThenReturnsFilteredEvents) {
    Console::WriteLine("[TEST] Invoking GET /api/events?channel=Application&level=CRITICAL&page=1&pageSize=10...");
    TestRestClient client;
    EventLogResponseDto response = client.GetEvents("Application", "CRITICAL", 1, 10);
    LogAndAssertEventLog(response, "Application", 1, 10);
    for (int i = 0; i < response.Events.GetCount(); ++i) {
        LogAndAssertEventDto(response.Events[i], "Critical");
    }
}

TEST(EventsControllerTests, GivenApplicationChannel_WhenQueryingErrorLevel_ThenReturnsFilteredEvents) {
    Console::WriteLine("[TEST] Invoking GET /api/events?channel=Application&level=ERROR&page=1&pageSize=10...");
    TestRestClient client;
    EventLogResponseDto response = client.GetEvents("Application", "ERROR", 1, 10);
    LogAndAssertEventLog(response, "Application", 1, 10);
    for (int i = 0; i < response.Events.GetCount(); ++i) {
        LogAndAssertEventDto(response.Events[i], "Error");
    }
}

TEST(EventsControllerTests, GivenApplicationChannel_WhenQueryingWarningLevel_ThenReturnsFilteredEvents) {
    Console::WriteLine("[TEST] Invoking GET /api/events?channel=Application&level=WARNING&page=1&pageSize=10...");
    TestRestClient client;
    EventLogResponseDto response = client.GetEvents("Application", "WARNING", 1, 10);
    LogAndAssertEventLog(response, "Application", 1, 10);
    for (int i = 0; i < response.Events.GetCount(); ++i) {
        LogAndAssertEventDto(response.Events[i], "Warning");
    }
}

TEST(EventsControllerTests, GivenSystemChannel_WhenQueryingInfoLevel_ThenReturnsFilteredEvents) {
    Console::WriteLine("[TEST] Invoking GET /api/events?channel=System&level=INFO&page=1&pageSize=10...");
    TestRestClient client;
    EventLogResponseDto response = client.GetEvents("System", "INFO", 1, 10);
    LogAndAssertEventLog(response, "System", 1, 10);
    for (int i = 0; i < response.Events.GetCount(); ++i) {
        LogAndAssertEventDto(response.Events[i], "Information");
    }
}

TEST(EventsControllerTests, GivenSystemChannel_WhenQueryingVerboseLevel_ThenReturnsFilteredEvents) {
    Console::WriteLine("[TEST] Invoking GET /api/events?channel=System&level=VERBOSE&page=1&pageSize=10...");
    TestRestClient client;
    EventLogResponseDto response = client.GetEvents("System", "VERBOSE", 1, 10);
    LogAndAssertEventLog(response, "System", 1, 10);
    for (int i = 0; i < response.Events.GetCount(); ++i) {
        LogAndAssertEventDto(response.Events[i], "Verbose");
    }
}

TEST(EventsControllerTests, GivenApplicationChannel_WhenQueryingAllEvents_ThenReturnsEventList) {
    Console::WriteLine("[TEST] Invoking GET /api/events?channel=Application&level=ALL&page=1&pageSize=20...");
    TestRestClient client;
    EventLogResponseDto response = client.GetEvents("Application", "ALL", 1, 20);
    LogAndAssertEventLog(response, "Application", 1, 20);
    for (int i = 0; i < response.Events.GetCount(); ++i) {
        LogAndAssertEventDto(response.Events[i], "");
    }
}

TEST(EventsControllerTests, GivenPaginationParameters_WhenGetEventsCalled_ThenReturnsPaginatedSlice) {
    Console::WriteLine("[TEST] Invoking GET /api/events with pagination (Page 1 vs Page 2)...");
    TestRestClient client;
    EventLogResponseDto page1 = client.GetEvents("Application", "ALL", 1, 5);
    EventLogResponseDto page2 = client.GetEvents("Application", "ALL", 2, 5);
    LogAndAssertEventLog(page1, "Application", 1, 5);
    LogAndAssertEventLog(page2, "Application", 2, 5);
}

TEST(EventsControllerTests, GivenNonExistentChannel_WhenGetEventsCalled_ThenReturnsEmptyEventList) {
    Console::WriteLine("[TEST] Invoking GET /api/events?channel=NonExistentChannel_XYZ_12345...");
    TestRestClient client;
    EventLogResponseDto response = client.GetEvents("NonExistentChannel_XYZ_12345", "ALL", 1, 10);
    LogAndAssertEventLog(response, "NonExistentChannel_XYZ_12345", 1, 10);
    Console::WriteLine("[ASSERT] Events.Count == 0 (Actual: {0})", response.Events.GetCount());
    EXPECT_EQ(response.Events.GetCount(), 0);
}
