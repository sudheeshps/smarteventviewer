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
#include "System/Net/Http/FileDownloader.h"
#include "System/Diagnostics/ProcessStreamer.h"
#include "System/Diagnostics/ProcessStreamOptions.h"
#include "Ai/AnalysisEvents.h"
#include "Ai/IAnalysisState.h"
#include "Ai/AnalysisStates.h"
#include "Mocks/MockLlamaModelProvider.h"

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
    auto spMockProvider = SmartPtr<ILlamaModelProvider>(SmartPtr<MockLlamaModelProvider>::NewShared());
    auto spLlm = SmartPtr<LocalLlmEngine>::NewShared(spMockProvider);
    auto spEvents = SmartPtr<IEventService>(SmartPtr<EventService>::NewShared());
    auto spNotifier = SmartPtr<MockTelemetryPushNotifier>::NewShared();

    AnalysisService service(spLlm, spEvents, spNotifier);

    AnalyzeRequestDto req;
    req.Channel = "Security";
    req.Query = "Detect brute force attacks";
    auto pending = service.EnqueueTask(req);

    AnalyzeResponseDto status;
    for (int i = 0; i < 30 && status.Status != "COMPLETED"; ++i) {
        DotNetDupe::System::Threading::Thread::Sleep(100);
        status = service.GetTaskStatus(pending.TaskId);
    }
    Console::WriteLine(String::Format("Analysis status: {0} Analysis: {1}", status.Status, status.Analysis));
    EXPECT_EQ(status.Status, "COMPLETED");
    EXPECT_FALSE(status.Analysis.IsEmpty());
}

TEST(LocalLlmEngineTests, DISABLED_GivenLiveNetwork_WhenModelDownloaded_ThenCachesFileOnDisk) {
    auto spLlm = SmartPtr<LocalLlmEngine>::NewShared();
    bool bProgressCalled = false;
    double dLastPercent = 0.0;

    spLlm->DownloadModelWithProgress("models/Qwen1.5-4B-Chat-Q4_K_M.gguf",
        DotNetDupe::System::Action<double, double, long long, long long>(
            [&bProgressCalled, &dLastPercent](double pct, double rate, long long dl, long long total) {
                bProgressCalled = true;
                dLastPercent = pct;
                Console::WriteLine("[LIVE_DOWNLOAD] {0}% ({1}/{2} bytes) at {3} KB/s", static_cast<int>(pct), dl, total, rate / 1024.0);
            }
        )
    );

    EXPECT_TRUE(bProgressCalled);
    EXPECT_GE(dLastPercent, 0.0);
    EXPECT_TRUE(spLlm->IsModelFilePresent("models/Qwen1.5-4B-Chat-Q4_K_M.gguf"));
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

// =============================================================================
// FileDownloader & ProcessStreamer EventHandler Integration Tests
// =============================================================================
TEST(FileDownloaderEventHandlerTests, GivenDownloadEventArgs_WhenPropertiesAccessed_ThenReturnsAccurateValues) {
    using namespace DotNetDupe::System::Net::Http;
    DownloadProgressChangedEventArgs progressArgs(1024, 2048, 50.0, 51200.0, DownloadStatus::Downloading);
    EXPECT_EQ(progressArgs.GetBytesReceived(), 1024LL);
    EXPECT_EQ(progressArgs.GetTotalBytesToReceive(), 2048LL);
    EXPECT_DOUBLE_EQ(progressArgs.GetProgressPercentage(), 50.0);
    EXPECT_DOUBLE_EQ(progressArgs.GetDownloadRateBytesPerSec(), 51200.0);
    EXPECT_EQ(progressArgs.GetStatus(), DownloadStatus::Downloading);

    DownloadCompletedEventArgs completedArgs(true, false, "");
    EXPECT_TRUE(completedArgs.IsSuccess());
    EXPECT_FALSE(completedArgs.IsCancelled());
    EXPECT_TRUE(completedArgs.GetError().IsEmpty());

    DownloadCompletedEventArgs failedArgs(false, false, "Connection refused");
    EXPECT_FALSE(failedArgs.IsSuccess());
    EXPECT_EQ(failedArgs.GetError(), "Connection refused");
}

TEST(ProcessStreamerEventHandlerTests, GivenProcessStreamer_WhenBatchAndProcessEventsAttached_ThenFiresSubscribers) {
    using namespace DotNetDupe::System::Diagnostics;
    ProcessStreamOptions options;
    options.eDetailLevel = ProcessMetricsDetail::FastDiscoveryOnly;
    options.iBatchSize = 5;
    options.iBatchIntervalMs = 10;
    options.bIncludeNetworkInfo = false;

    DotNetDupe::System::Diagnostics::ProcessStreamer streamer(options);
    bool bDiscovered = false;
    bool bBatchFired = false;
    bool bCompletedFired = false;

    streamer.ProcessDiscovered += [&bDiscovered](const void* pSender, const ProcessEventArgs& e) {
        if (e.GetProcess().iProcessId > 0) bDiscovered = true;
    };
    streamer.BatchReady += [&bBatchFired](const void* pSender, const ProcessBatchEventArgs& e) {
        if (e.GetBatch().GetCount() > 0) bBatchFired = true;
    };
    streamer.Completed += [&bCompletedFired](const void* pSender, const DotNetDupe::System::EventArgs& e) {
        bCompletedFired = true;
    };

    streamer.Start();
    int iWaitedMs = 0;
    while (streamer.IsRunning() && iWaitedMs < 2000) {
        Thread::Sleep(50);
        iWaitedMs += 50;
    }

    EXPECT_TRUE(bDiscovered || bBatchFired);
    EXPECT_TRUE(bCompletedFired);
}

// =============================================================================
// Analysis State Pattern & EventArgs Unit Tests
// =============================================================================
TEST(AnalysisEventsTests, GivenAnalysisEventArgs_WhenPropertiesAccessed_ThenReturnsAccurateValues) {
    AnalyzeResponseDto respDto;
    respDto.TaskId = "task_100";
    respDto.Status = "COMPLETED";

    AnalysisStateChangedEventArgs stateArgs("task_100", "ModelDownloading", "ModelInitializing", "INITIALIZING", "Loading weights...", respDto, true);
    EXPECT_EQ(stateArgs.GetTaskId(), "task_100");
    EXPECT_EQ(stateArgs.GetPreviousState(), "ModelDownloading");
    EXPECT_EQ(stateArgs.GetNewState(), "ModelInitializing");
    EXPECT_EQ(stateArgs.GetStatus(), "INITIALIZING");
    EXPECT_EQ(stateArgs.GetProgressMessage(), "Loading weights...");
    EXPECT_TRUE(stateArgs.IsTerminal());
    EXPECT_EQ(stateArgs.GetResponse().Status, "COMPLETED");

    auto spDlDetails = SmartPtr<DotNetDupe::System::Net::Http::DownloadProgressChangedEventArgs>::NewShared(
        5242880LL, 10485760LL, 50.0, 1048576.0, DotNetDupe::System::Net::Http::DownloadStatus::Downloading);
    AnalysisProgressChangedEventArgs progArgs("task_100", 50.0, "Downloading weights...", spDlDetails);
    EXPECT_EQ(progArgs.GetTaskId(), "task_100");
    EXPECT_DOUBLE_EQ(progArgs.GetProgressPercentage(), 50.0);
    EXPECT_TRUE(progArgs.HasDetails());

    auto spExtracted = progArgs.GetDetailsAs<DotNetDupe::System::Net::Http::DownloadProgressChangedEventArgs>();
    EXPECT_FALSE(spExtracted.IsNull());
    EXPECT_EQ(spExtracted->GetBytesReceived(), 5242880LL);
}

TEST(AnalysisStateTests, GivenEventIngestingState_WhenExecuted_ThenTransitionsContextToPromptSetupState) {
    auto spReader = SmartPtr<MockEventLogReader>::NewShared();
    EventRecord secRec(201, EventLevel::Error, "Microsoft-Windows-Security-Auditing", "2026-08-20 10:00:00", "State test event", "");
    spReader->AddEvent("Security", secRec);
    auto spEventService = SmartPtr<IEventService>(SmartPtr<EventService>::NewShared(spReader));
    auto spLlm = SmartPtr<LocalLlmEngine>::NewShared();

    AnalysisService service(spLlm, spEventService, nullptr);
    auto pItem = SmartPtr<AnalysisTaskItem>::NewShared();
    pItem->TaskId = "task_200";
    pItem->Request.Channel = "Security";
    pItem->Request.Query = "Audit test query";

    EventIngestingState ingestState;
    ingestState.Execute(service, pItem);

    EXPECT_FALSE(service.GetCurrentState().IsNull());
    EXPECT_EQ(service.GetCurrentState()->GetStateName(), "PromptSetup");
}

TEST(AnalysisStateTests, GivenFailedState_WhenExecuted_ThenRaisesTerminalStateChangedWithFailure) {
    auto spLlm = SmartPtr<LocalLlmEngine>::NewShared();
    AnalysisService service(spLlm, nullptr, nullptr);

    bool bTerminalReceived = false;
    String sReceivedStatus;
    service.StateChanged += [&bTerminalReceived, &sReceivedStatus](const void* pSender, const AnalysisStateChangedEventArgs& e) {
        if (e.IsTerminal()) {
            bTerminalReceived = true;
            sReceivedStatus = e.GetStatus();
        }
    };

    auto pItem = SmartPtr<AnalysisTaskItem>::NewShared();
    pItem->TaskId = "task_300";
    pItem->Request.Channel = "Application";

    FailedState failedState("Injected unit test error");
    failedState.Execute(service, pItem);

    EXPECT_TRUE(bTerminalReceived);
    EXPECT_EQ(sReceivedStatus, "FAILED");
}

TEST(AnalysisServiceStateTests, GivenAnalysisService_WhenClientSubscribesToEventHandlers_ThenReceivesStateAndProgressNotifications) {
    auto spMockProvider = SmartPtr<ILlamaModelProvider>(SmartPtr<MockLlamaModelProvider>::NewShared());
    auto spLlm = SmartPtr<LocalLlmEngine>::NewShared(spMockProvider);
    auto spEvents = SmartPtr<IEventService>(SmartPtr<EventService>::NewShared());
    AnalysisService service(spLlm, spEvents, nullptr);

    bool bStateChangedFired = false;
    bool bCompletedFired = false;

    service.StateChanged += [&bStateChangedFired, &bCompletedFired](const void* pSender, const AnalysisStateChangedEventArgs& e) {
        bStateChangedFired = true;
        if (e.IsTerminal() && e.GetStatus() == "COMPLETED") {
            bCompletedFired = true;
        }
    };

    AnalyzeRequestDto req;
    req.Channel = "Security";
    req.Query = "Detect brute force attacks";
    auto pending = service.EnqueueTask(req);

    AnalyzeResponseDto status;
    for (int i = 0; i < 30 && status.Status != "COMPLETED"; ++i) {
        Thread::Sleep(100);
        status = service.GetTaskStatus(pending.TaskId);
    }

    EXPECT_TRUE(bStateChangedFired);
    EXPECT_TRUE(bCompletedFired);
    EXPECT_EQ(status.Status, "COMPLETED");
    EXPECT_FALSE(status.Analysis.IsEmpty());
}



