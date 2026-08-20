#include <gtest/gtest.h>
#include "Controllers/EventsController.h"
#include "System/Console.h"

using namespace DotNetDupe::System;
using namespace SmartEventViewer;

static void LogAndAssertEventDto(const EventDto& evt, const String& sExpectedLevel) {
    Console::WriteLine("  [EVENT_DTO] Id={0} | Level={1} | Risk={2} | Provider={3} | Time={4}",
        evt.Id, evt.Level, evt.Risk, evt.Provider, evt.Time);
    EXPECT_GE(evt.Id, 0U);
    if (!sExpectedLevel.IsEmpty()) {
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
    EXPECT_EQ(resp.Channel, sExpectedChannel);
    unsigned long long uSum = resp.CriticalCount + resp.ErrorCount + resp.WarningCount + resp.InfoCount + resp.VerboseCount;
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
    EXPECT_EQ(resp.Channel, sChannel);
    EXPECT_GE(resp.TotalCount, 0ULL);
    EXPECT_EQ(resp.Page, nPage);
    EXPECT_EQ(resp.PageSize, nPageSize);
    EXPECT_GE(resp.TotalPages, 0ULL);
    EXPECT_LE(resp.Events.GetCount(), nPageSize);
}

TEST(EventsControllerTests, GivenController_WhenGetChannelsCalled_ThenReturnsAvailableChannels) {
    EventsController controller;
    ChannelsResponseDto response = controller.GetChannels();
    EXPECT_GT(response.Channels.GetCount(), 0);
    EXPECT_TRUE(response.Channels.Contains("Application"));
    EXPECT_TRUE(response.Channels.Contains("System"));
}

TEST(EventsControllerTests, GivenApplicationChannel_WhenGetEventSummaryCalled_ThenReturnsChannelSummary) {
    EventsController controller;
    EventSummaryResponseDto response = controller.GetEventSummary("Application");
    LogAndAssertEventSummary(response, "Application");
}

TEST(EventsControllerTests, GivenSystemChannel_WhenGetEventSummaryCalled_ThenReturnsChannelSummary) {
    EventsController controller;
    EventSummaryResponseDto response = controller.GetEventSummary("System");
    LogAndAssertEventSummary(response, "System");
}

TEST(EventsControllerTests, GivenEmptyChannel_WhenGetEventSummaryCalled_ThenReturnsAggregatedSummary) {
    EventsController controller;
    EventSummaryResponseDto response = controller.GetEventSummary("");
    EXPECT_FALSE(response.Channel.IsEmpty());
    LogAndAssertEventSummary(response, response.Channel);
}

TEST(EventsControllerTests, GivenInvalidChannel_WhenGetEventSummaryCalled_ThenHandlesGracefully) {
    EventsController controller;
    EventSummaryResponseDto response = controller.GetEventSummary("NonExistentChannel_XYZ_12345");
    LogAndAssertEventSummary(response, "NonExistentChannel_XYZ_12345");
}

TEST(EventsControllerTests, GivenApplicationChannel_WhenQueryingCriticalLevel_ThenReturnsFilteredEvents) {
    EventsController controller;
    EventLogResponseDto response = controller.GetEvents("Application", 1, 10, "CRITICAL");
    LogAndAssertEventLog(response, "Application", 1, 10);
    for (int i = 0; i < response.Events.GetCount(); ++i) {
        LogAndAssertEventDto(response.Events[i], "Critical");
    }
}

TEST(EventsControllerTests, GivenApplicationChannel_WhenQueryingErrorLevel_ThenReturnsFilteredEvents) {
    EventsController controller;
    EventLogResponseDto response = controller.GetEvents("Application", 1, 10, "ERROR");
    LogAndAssertEventLog(response, "Application", 1, 10);
    for (int i = 0; i < response.Events.GetCount(); ++i) {
        LogAndAssertEventDto(response.Events[i], "Error");
    }
}

TEST(EventsControllerTests, GivenApplicationChannel_WhenQueryingWarningLevel_ThenReturnsFilteredEvents) {
    EventsController controller;
    EventLogResponseDto response = controller.GetEvents("Application", 1, 10, "WARNING");
    LogAndAssertEventLog(response, "Application", 1, 10);
    for (int i = 0; i < response.Events.GetCount(); ++i) {
        LogAndAssertEventDto(response.Events[i], "Warning");
    }
}

TEST(EventsControllerTests, GivenSystemChannel_WhenQueryingInfoLevel_ThenReturnsFilteredEvents) {
    EventsController controller;
    EventLogResponseDto response = controller.GetEvents("System", 1, 10, "INFO");
    LogAndAssertEventLog(response, "System", 1, 10);
    for (int i = 0; i < response.Events.GetCount(); ++i) {
        LogAndAssertEventDto(response.Events[i], "Information");
    }
}

TEST(EventsControllerTests, GivenSystemChannel_WhenQueryingVerboseLevel_ThenReturnsFilteredEvents) {
    EventsController controller;
    EventLogResponseDto response = controller.GetEvents("System", 1, 10, "VERBOSE");
    LogAndAssertEventLog(response, "System", 1, 10);
    for (int i = 0; i < response.Events.GetCount(); ++i) {
        LogAndAssertEventDto(response.Events[i], "Verbose");
    }
}

TEST(EventsControllerTests, GivenApplicationChannel_WhenQueryingAllEvents_ThenReturnsEventList) {
    EventsController controller;
    EventLogResponseDto response = controller.GetEvents("Application", 1, 20, "ALL");
    LogAndAssertEventLog(response, "Application", 1, 20);
    for (int i = 0; i < response.Events.GetCount(); ++i) {
        LogAndAssertEventDto(response.Events[i], "");
    }
}

TEST(EventsControllerTests, GivenPaginationParameters_WhenGetEventsCalled_ThenReturnsPaginatedSlice) {
    EventsController controller;
    EventLogResponseDto page1 = controller.GetEvents("Application", 1, 5, "ALL");
    EventLogResponseDto page2 = controller.GetEvents("Application", 2, 5, "ALL");
    LogAndAssertEventLog(page1, "Application", 1, 5);
    LogAndAssertEventLog(page2, "Application", 2, 5);
}

TEST(EventsControllerTests, GivenNonExistentChannel_WhenGetEventsCalled_ThenReturnsEmptyEventList) {
    EventsController controller;
    EventLogResponseDto response = controller.GetEvents("NonExistentChannel_XYZ_12345", 1, 10, "ALL");
    LogAndAssertEventLog(response, "NonExistentChannel_XYZ_12345", 1, 10);
    EXPECT_EQ(response.Events.GetCount(), 0);
}

TEST(EventsControllerTests, GivenController_WhenGetAnomaliesCalled_ThenReturnsCrossChannelAnomalies) {
    EventsController controller;
    MultiChannelAnomaliesDto response = controller.GetAnomalies(10);
    EXPECT_GE(response.SecurityEvents.GetCount(), 0);
    EXPECT_GE(response.SystemEvents.GetCount(), 0);
    EXPECT_GE(response.ApplicationEvents.GetCount(), 0);
    EXPECT_GE(response.SysmonEvents.GetCount(), 0);
}
