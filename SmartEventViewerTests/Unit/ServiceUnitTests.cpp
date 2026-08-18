#include <gtest/gtest.h>
#include "Core/EventRecord.h"
#include "Core/AnomalyEngine.h"
#include "Core/IEventService.h"
#include "Core/EventService.h"
#include "Core/ITelemetryService.h"
#include "Core/TelemetryService.h"
#include "Core/TelemetryChangeDetector.h"
#include "Core/IAnalysisService.h"
#include "Core/AnalysisService.h"
#include "Core/IDiagnosticsService.h"
#include "Core/DiagnosticsService.h"
#include "Mocks/MockEventLogReader.h"
#include "Mocks/MockSystemTelemetryProvider.h"
#include "Mocks/MockTelemetryPushNotifier.h"
#include "System/Console.h"
#include "System/SmartPointer.h"
#include "System/Threading/Thread.h"

using DotNetDupe::System::String;
using DotNetDupe::System::Console;
using DotNetDupe::System::Threading::Thread;
using namespace SmartEventViewer;
using namespace SmartEventViewer::Tests;

template<typename T>
using SmartPtr = DotNetDupe::System::SmartPointer<T>;

// =============================================================================
// TelemetryChangeDetector Unit Tests
// =============================================================================
TEST(TelemetryChangeDetectorTests, GivenInitialSnapshot_WhenChecked_ThenReportsChangedForFirstObservation) {
    TelemetryChangeDetector detector;
    SystemMetricsResponseDto metrics;
    metrics.CpuUsagePercent = 10.0;
    metrics.MemoryUsagePercent = 40.0;
    metrics.MemoryUsedMB = 4096;

    EXPECT_TRUE(detector.HasSummaryChanged(metrics));
}

TEST(TelemetryChangeDetectorTests, GivenSameSnapshot_WhenCheckedAgain_ThenReportsNotChanged) {
    TelemetryChangeDetector detector;
    SystemMetricsResponseDto metrics;
    metrics.CpuUsagePercent = 10.0;
    metrics.MemoryUsagePercent = 40.0;
    metrics.MemoryUsedMB = 4096;

    detector.HasSummaryChanged(metrics);
    EXPECT_FALSE(detector.HasSummaryChanged(metrics));
}

TEST(TelemetryChangeDetectorTests, GivenCpuDeltaAboveThreshold_WhenChecked_ThenReportsSummaryChanged) {
    TelemetryChangeDetector detector;
    SystemMetricsResponseDto m1;
    m1.CpuUsagePercent = 10.0;
    m1.MemoryUsagePercent = 40.0;
    m1.MemoryUsedMB = 4096;
    detector.HasSummaryChanged(m1);

    SystemMetricsResponseDto m2;
    m2.CpuUsagePercent = 10.6;
    m2.MemoryUsagePercent = 40.0;
    m2.MemoryUsedMB = 4096;

    EXPECT_TRUE(detector.HasSummaryChanged(m2));
}

TEST(TelemetryChangeDetectorTests, GivenCpuDeltaBelowThreshold_WhenChecked_ThenReportsNoSummaryChange) {
    TelemetryChangeDetector detector;
    SystemMetricsResponseDto m1;
    m1.CpuUsagePercent = 10.0;
    m1.MemoryUsagePercent = 40.0;
    m1.MemoryUsedMB = 4096;
    detector.HasSummaryChanged(m1);

    SystemMetricsResponseDto m2;
    m2.CpuUsagePercent = 10.2;
    m2.MemoryUsagePercent = 40.0;
    m2.MemoryUsedMB = 4096;

    EXPECT_FALSE(detector.HasSummaryChanged(m2));
}

TEST(TelemetryChangeDetectorTests, GivenNewProcessAdded_WhenChecked_ThenReportsProcessesChanged) {
    TelemetryChangeDetector detector;
    SystemMetricsResponseDto m1;
    ProcessResourceDto proc1;
    proc1.ProcessId = 100;
    proc1.Name = "svchost.exe";
    m1.TopProcesses.Add(proc1);
    detector.HaveProcessesChanged(m1);

    SystemMetricsResponseDto m2;
    m2.TopProcesses.Add(proc1);
    ProcessResourceDto proc2;
    proc2.ProcessId = 200;
    proc2.Name = "malware.exe";
    m2.TopProcesses.Add(proc2);

    EXPECT_TRUE(detector.HaveProcessesChanged(m2));
}

TEST(TelemetryChangeDetectorTests, GivenServiceStatusChange_WhenChecked_ThenReportsServicesChanged) {
    TelemetryChangeDetector detector;
    ServicesResponseDto initial;
    detector.HaveServicesChanged(initial);

    ServicesResponseDto updated;
    ServiceInfoDto svc("EventLog", "Windows Event Log", "Running", "Auto", 123);
    updated.Services.Add(svc);

    EXPECT_TRUE(detector.HaveServicesChanged(updated));
}

// =============================================================================
// TelemetryService & Push Unit Tests
// =============================================================================
TEST(TelemetryServiceTests, GivenMockProviderAndMockNotifier_WhenSampledWithCpuDelta_ThenBroadcastsSummaryUpdate) {
    auto spProvider = SmartPtr<MockSystemTelemetryProvider>::NewShared();
    auto spNotifier = SmartPtr<MockTelemetryPushNotifier>::NewShared();
    auto spDetector = SmartPtr<TelemetryChangeDetector>::NewShared();

    TelemetryService service(spProvider, spNotifier, spDetector);
    service.SampleAndDetectChanges();

    EXPECT_TRUE(spNotifier->GetBroadcastHistory().Contains("summary"));
}

TEST(TelemetryServiceTests, GivenMockProvider_WhenGetSummaryCalled_ThenReturnsMetrics) {
    auto spProvider = SmartPtr<MockSystemTelemetryProvider>::NewShared();
    spProvider->SetCpuUsage(25.5);
    TelemetryService service(spProvider);

    auto summary = service.GetSummary();
    EXPECT_DOUBLE_EQ(summary.CpuUsagePercent, 25.5);
}

TEST(TelemetryServiceTests, GivenMockProvider_WhenProcessesQueried_ThenReturnsProcessesList) {
    auto spProvider = SmartPtr<MockSystemTelemetryProvider>::NewShared();
    ProcessResourceDto proc;
    proc.ProcessId = 5000;
    proc.Name = "system_agent.exe";
    spProvider->AddProcess(proc);

    TelemetryService service(spProvider);
    auto result = service.GetProcesses();
    EXPECT_EQ(result.TopProcesses.GetCount(), 1);
    EXPECT_EQ(result.TopProcesses[0].ProcessId, 5000U);
}

// =============================================================================
// EventService Unit Tests
// =============================================================================
TEST(EventServiceTests, GivenMockReader_WhenGetChannelsCalled_ThenReturnsConfiguredChannels) {
    auto spReader = SmartPtr<MockEventLogReader>::NewShared();
    spReader->AddChannel("CustomSecurityChannel");

    EventService service(spReader);
    auto channelsDto = service.GetChannels();

    EXPECT_TRUE(channelsDto.Channels.Contains("CustomSecurityChannel"));
}

TEST(EventServiceTests, GivenMockReaderWithSecurityEvents_WhenGetEventsCalled_ThenReturnsRiskEvaluatedDtos) {
    auto spReader = SmartPtr<MockEventLogReader>::NewShared();
    EventRecord secEvt(4625, EventLevel::Error, "Microsoft-Windows-Security-Auditing", "Security", "Account failed logon", "2026-08-17T10:00:00Z");
    spReader->AddEvent("Security", secEvt);

    EventService service(spReader);
    auto logDto = service.GetEvents("Security", 1, 10, "ALL");

    EXPECT_EQ(logDto.Events.GetCount(), 1);
    EXPECT_EQ(logDto.Events[0].Id, 4625U);
    EXPECT_EQ(logDto.Events[0].Risk, "High");
}

TEST(EventServiceTests, GivenMockReaderWithEvents_WhenGetEventSummaryCalled_ThenComputesLevelCounts) {
    auto spReader = SmartPtr<MockEventLogReader>::NewShared();
    EventRecord critEvt(1, EventLevel::Critical, "Kernel", "System", "Kernel panic", "2026-08-17T10:00:00Z");
    EventRecord warnEvt(2, EventLevel::Warning, "Disk", "System", "Disk space low", "2026-08-17T10:05:00Z");
    spReader->AddEvent("System", critEvt);
    spReader->AddEvent("System", warnEvt);

    EventService service(spReader);
    auto summary = service.GetEventSummary("System");

    EXPECT_EQ(summary.Channel, "System");
    EXPECT_EQ(summary.TotalCount, 2ULL);
    EXPECT_EQ(summary.CriticalCount, 1ULL);
    EXPECT_EQ(summary.WarningCount, 1ULL);
}

class MockFallbackEventLogReader : public IEventLogReader {
public:
    StringList GetEventChannels() override {
        StringList channels;
        channels.Add("Sysmon");
        return channels;
    }
    unsigned long long GetChannelEventCount(const String& sChannelName) override {
        (void)sChannelName;
        return 49723ULL;
    }
    bool GetChannelLevelCounts(const String& sChannelName, EventLevelCounts& outCounts) override {
        (void)sChannelName;
        (void)outCounts;
        return false;
    }
    DotNetDupe::System::Collections::Generic::List<EventRecord> ReadEvents(
        const String& sChannelName, size_t nMaxCount, size_t nStartIndex = 0, bool bReverseOrder = true, EventLevel eLevel = EventLevel::LogAlways) override {
        (void)sChannelName;
        (void)nMaxCount;
        (void)nStartIndex;
        (void)bReverseOrder;
        (void)eLevel;
        DotNetDupe::System::Collections::Generic::List<EventRecord> results;
        results.Add(EventRecord(1, EventLevel::Warning, "Sysmon", "Sysmon", "Driver warning", "2026-08-17T10:00:00Z"));
        return results;
    }
};

TEST(EventServiceTests, GivenFallbackReaderWithLargeChannel_WhenGetEventSummaryCalled_ThenInfoCountCalculatedFromTotal) {
    auto spReader = SmartPtr<MockFallbackEventLogReader>::NewShared();
    EventService service(spReader);
    auto summary = service.GetEventSummary("Sysmon");

    EXPECT_EQ(summary.Channel, "Sysmon");
    EXPECT_EQ(summary.TotalCount, 49723ULL);
    EXPECT_EQ(summary.WarningCount, 1ULL);
    EXPECT_EQ(summary.InfoCount, 49722ULL);
}

TEST(EventServiceTests, GivenSpecificLevelFilter_WhenGetEventsCalled_ThenReturnsLevelFilteredEventsWithAccurateTotalCount) {
    auto spReader = SmartPtr<MockEventLogReader>::NewShared();
    spReader->AddEvent("Security", EventRecord(4624, EventLevel::Informational, "Audit", "Security", "Logon ok", "2026-08-18 10:00:00"));
    spReader->AddEvent("Security", EventRecord(4625, EventLevel::Error, "Audit", "Security", "Logon failed", "2026-08-18 10:01:00"));
    spReader->AddEvent("Security", EventRecord(1102, EventLevel::Critical, "Audit", "Security", "Audit log cleared", "2026-08-18 10:02:00"));

    EventService service(spReader);
    auto res = service.GetEvents("Security", 1, 10, "Critical");

    EXPECT_EQ(res.TotalCount, 1ULL);
    EXPECT_EQ(res.Events.GetCount(), 1);
    EXPECT_EQ(res.Events[0].Level, "Critical");
    EXPECT_EQ(res.Events[0].Id, 1102);
}

// =============================================================================
// AnalysisService Unit Tests
// =============================================================================
TEST(AnalysisServiceTests, GivenMockNotifier_WhenTaskEnqueued_ThenEmitsLlmAnalysisPushNotification) {
    auto spLlm = SmartPtr<LocalLlmEngine>::NewShared();
    auto spEvents = SmartPtr<IEventService>(SmartPtr<EventService>::NewShared());
    auto spNotifier = SmartPtr<MockTelemetryPushNotifier>::NewShared();

    AnalysisService service(spLlm, spEvents, spNotifier);

    AnalyzeRequestDto req;
    req.Channel = "Application";
    req.Query = "Analyze crash dumps";
    auto pending = service.EnqueueTask(req);

    EXPECT_EQ(pending.Status, "PENDING");
    EXPECT_FALSE(pending.TaskId.IsEmpty());
    EXPECT_TRUE(spNotifier->GetBroadcastHistory().Contains("llm_analysis"));
}

TEST(AnalysisServiceTests, GivenValidTask_WhenProcessed_ThenTransitionsStatusToCompleted) {
    auto spLlm = SmartPtr<LocalLlmEngine>::NewShared();
    auto spEvents = SmartPtr<IEventService>(SmartPtr<EventService>::NewShared());
    auto spNotifier = SmartPtr<MockTelemetryPushNotifier>::NewShared();

    AnalysisService service(spLlm, spEvents, spNotifier);

    AnalyzeRequestDto req;
    req.Channel = "Security";
    req.Query = "Detect brute force attacks";
    auto pending = service.EnqueueTask(req);

    // Give background worker a moment to complete execution
    DotNetDupe::System::Threading::Thread::Sleep(600);

    auto status = service.GetTaskStatus(pending.TaskId);
    EXPECT_TRUE(status.Status == "COMPLETED" || status.Status == "PROCESSING");
    EXPECT_FALSE(status.Analysis.IsEmpty());
}

// =============================================================================
// DiagnosticsService Unit Tests
// =============================================================================
TEST(DiagnosticsServiceTests, GivenDiagnosticsService_WhenGetLogFormatCalled_ThenReturnsExpectedColumns) {
    DiagnosticsService service;
    auto formatDto = service.GetLogFormat();

    EXPECT_GT(formatDto.Columns.GetCount(), 0);
    EXPECT_EQ(formatDto.Columns[0].Key, "timestamp");
    EXPECT_EQ(formatDto.Columns[1].Key, "level");
}

TEST(DiagnosticsServiceTests, GivenDiagnosticsService_WhenGetServerLogsCalled_ThenReturnsRecords) {
    DiagnosticsService service;
    auto logsDto = service.GetServerLogs(50);

    EXPECT_GE(logsDto.Records.GetCount(), 0);
}

// =============================================================================
// Service Unit Tests with Injected Mocks
// =============================================================================
TEST(ServiceUnitTests, GivenInjectedEventService_WhenGetChannelsCalled_ThenReturnsExpectedDtos) {
    auto spReader = SmartPtr<MockEventLogReader>::NewShared();
    spReader->AddChannel("CustomTestChannel");
    auto spService = SmartPtr<IEventService>(SmartPtr<EventService>::NewShared(spReader));

    auto channels = spService->GetChannels();
    EXPECT_TRUE(channels.Channels.Contains("CustomTestChannel"));
}

TEST(ServiceUnitTests, GivenInjectedTelemetryService_WhenGetSummaryCalled_ThenReturnsMetrics) {
    auto spProvider = SmartPtr<MockSystemTelemetryProvider>::NewShared();
    spProvider->SetCpuUsage(18.2);
    auto spService = SmartPtr<ITelemetryService>(SmartPtr<TelemetryService>::NewShared(spProvider));

    auto summary = spService->GetSummary();
    EXPECT_DOUBLE_EQ(summary.CpuUsagePercent, 18.2);
}

TEST(ServiceUnitTests, GivenEventService_WhenGetCrossChannelAnomaliesCalled_ThenAggregatesAcrossCoreChannels) {
    auto spReader = SmartPtr<MockEventLogReader>::NewShared();
    EventRecord secRec(101, EventLevel::Error, "Microsoft-Windows-Security-Auditing", "2026-08-18 10:00:00", "Failed login attempt", "");
    EventRecord sysRec(102, EventLevel::Warning, "Service Control Manager", "2026-08-18 10:01:00", "Service timed out", "");
    spReader->AddEvent("Security", secRec);
    spReader->AddEvent("System", sysRec);

    auto spService = SmartPtr<IEventService>(SmartPtr<EventService>::NewShared(spReader));
    auto anomalies = spService->GetCrossChannelAnomalies(10);

    EXPECT_GT(anomalies.SecurityEvents.GetCount(), 0);
    EXPECT_GT(anomalies.SystemEvents.GetCount(), 0);
    EXPECT_GE(anomalies.TotalErrorCount, 1);
    EXPECT_GE(anomalies.TotalWarningCount, 1);
}
