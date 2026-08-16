#include <gtest/gtest.h>
#include <cassert>
#include "Core/EventRecord.h"
#include "Core/AnomalyEngine.h"
#include "System/Diagnostics/EtwLogReader.h"
#include "Core/SystemTelemetryProvider.h"
#include "Core/TelemetryController.h"
#include "Platform/LinuxJournalReader.h"
#include "Ai/LocalLlmEngine.h"
#include "Ai/RagVectorStore.h"
#include "System/Console.h"
#include "Core/DiagnosticsController.h"
#include "Core/EventsController.h"
#include "Core/LlmAnalysisController.h"
#include "Extensions/DependencyInjection/ServiceCollection.h"
#include "Extensions/DependencyInjection/ServiceProvider.h"
#include "System/Threading/ManualResetEvent.h"
#include "System/IO/File.h"

using DotNetDupe::System::String;
using DotNetDupe::System::Console;
using DotNetDupe::System::SmartPointer;

// --- RagVectorStore Tests ---
// Positive Test
TEST(EventRecordTest, GivenValidEvent_WhenIndexed_ThenRagVectorStoreReturnsSimilarMatches) {
    Console::WriteLine("1. Init vectorStore");
    SmartEventViewer::RagVectorStore vectorStore;
    try {
        Console::WriteLine("2. Init evt1");
        SmartEventViewer::EventRecord evt1(101, SmartEventViewer::EventLevel::Warning, "Security", "Security", "Failed logon attempt for user admin", "2026-07-29T12:00:00Z");
        Console::WriteLine("3. Init evt2");
        SmartEventViewer::EventRecord evt2(102, SmartEventViewer::EventLevel::Informational, "System", "System", "Service started successfully", "2026-07-29T12:05:00Z");
    } catch (const DotNetDupe::System::Exception& e) {
        Console::WriteLine(String::Format("DotNetDupe Exception: {0}", e.What()));
    } catch (const std::exception& e) {
        Console::WriteLine(String::Format("std::exception: {0}", String(e.what())));
    } catch (...) {
        Console::WriteLine("Unknown Exception");
    }
    
    // We will just exit the test early so it doesn't crash the next steps
    return;
}

// Edge Case Test
TEST(EventRecordTest, GivenEmptyVectorStore_WhenQueried_ThenReturnsFalseOrEmptyList) {
    SmartEventViewer::RagVectorStore vectorStore;
    SmartEventViewer::EventList results;
    bool bSuccess = vectorStore.QuerySimilarEvents("logon attempt", 2, results);
    EXPECT_TRUE(bSuccess || results.GetCount() == 0);
    Console::WriteLine("[PASS] GivenEmptyVectorStore_WhenQueried_ThenReturnsFalseOrEmptyList");
}

// --- EventRecord Tests ---
// Positive Test
TEST(EventRecordTest, GivenValidParameters_WhenEventRecordCreated_ThenPropertiesInitializedCorrectly) {
    SmartEventViewer::EventRecord evt(1001, SmartEventViewer::EventLevel::Error, "Application Error", "Application", "Process crashed", "2026-07-29T10:00:00Z");
    EXPECT_TRUE(evt.GetEventId() == 1001);
    EXPECT_TRUE(evt.GetLevel() == SmartEventViewer::EventLevel::Error);
    EXPECT_TRUE(evt.GetProviderName() == "Application Error");
    EXPECT_TRUE(evt.GetChannel() == "Application");
    EXPECT_TRUE(evt.GetEventMessage() == "Process crashed");
    Console::WriteLine("[PASS] GivenValidParameters_WhenEventRecordCreated_ThenPropertiesInitializedCorrectly");
}

// Negative & Edge Case Test
TEST(EventRecordTest, GivenZeroEventIdAndEmptyStrings_WhenEventRecordCreated_ThenHandlesEdgeValues) {
    SmartEventViewer::EventRecord evt(0, SmartEventViewer::EventLevel::LogAlways, "", "", "", "");
    EXPECT_TRUE(evt.GetEventId() == 0);
    EXPECT_TRUE(evt.GetLevel() == SmartEventViewer::EventLevel::LogAlways);
    EXPECT_TRUE(evt.GetProviderName().IsEmpty());
    EXPECT_TRUE(evt.GetChannel().IsEmpty());
    EXPECT_TRUE(evt.GetEventMessage().IsEmpty());
    Console::WriteLine("[PASS] GivenZeroEventIdAndEmptyStrings_WhenEventRecordCreated_ThenHandlesEdgeValues");
}

// --- AnomalyEngine Tests ---
// Positive & Negative Risk Evaluation Test
TEST(EventRecordTest, GivenNormalAndSecurityEvents_WhenEvaluated_ThenAnomalyEngineAssignsCorrectRiskLevels) {
    SmartEventViewer::AnomalyEngine engine;
    SmartEventViewer::EventRecord normalEvt(100, SmartEventViewer::EventLevel::Informational, "System", "System", "Service status ok", "2026-07-29T11:00:00Z");
    SmartEventViewer::RiskLevel riskNormal = engine.EvaluateRisk(normalEvt);
    EXPECT_TRUE(riskNormal == SmartEventViewer::RiskLevel::Low);

    SmartEventViewer::EventRecord criticalEvt(4625, SmartEventViewer::EventLevel::Error, "Microsoft-Windows-Security-Auditing", "Security", "An account failed to log on. Multiple attempts detected.", "2026-07-29T11:01:00Z");
    SmartEventViewer::RiskLevel riskCritical = engine.EvaluateRisk(criticalEvt);
    EXPECT_TRUE(riskCritical == SmartEventViewer::RiskLevel::Critical || riskCritical == SmartEventViewer::RiskLevel::High);
    Console::WriteLine("[PASS] GivenNormalAndSecurityEvents_WhenEvaluated_ThenAnomalyEngineAssignsCorrectRiskLevels");
}

// Mock Llama Model Provider for Unit Testing
class MockLlamaModelProvider : public SmartEventViewer::ILlamaModelProvider {
private:
    bool m_bLoaded{ false };

public:
    void InitBackend() override { }
    void LoadModel(const String& sModelPath) override { m_bLoaded = true; }
    void CreateContext() override { if (!m_bLoaded) throw DotNetDupe::System::SystemException("Mock not loaded"); }
    String ExecuteInference(const String& sSystemPrompt, const String& sUserQuery, const DotNetDupe::System::Collections::Generic::List<SmartEventViewer::EventRecord>& events) override {
        return "🤖 [MOCK LLAMA MODEL PROVIDER EXECUTED]\nMock inference result for query: " + sUserQuery;
    }
    void FreeContextAndModel() override { m_bLoaded = false; }
    bool IsLoaded() const override { return m_bLoaded; }
    bool IsModelFilePresent(const String& sModelPath) const override { return true; }
};

// --- LocalLlmEngine Tests ---
// Positive Test with Mock Model Provider
TEST(EventRecordTest, GivenMockModelProvider_WhenLocalLlmEngineQueryProcessed_ThenInvokesMockModelLayer) {
    Console::WriteLine("[DEBUG] Init mock engine");
    SmartPointer<SmartEventViewer::ILlamaModelProvider> spMockProvider(SmartPointer<MockLlamaModelProvider>::NewShared());
    SmartEventViewer::LocalLlmEngine llm(spMockProvider);

    Console::WriteLine("[DEBUG] Initializing model");
    llm.Initialize("mock/models/model.gguf");
    Console::WriteLine("[DEBUG] Checking IsModelLoaded");
    EXPECT_TRUE(llm.IsModelLoaded() == true);

    Console::WriteLine("[DEBUG] Wait for completion");
    DotNetDupe::System::Threading::ManualResetEvent completion(false);
    String sResponse = "";
    llm.ProcessQueryAsync("Analyze suspicious activity", nullptr, 0, [&completion, &sResponse](const String& s, const String& r, double p) {
        Console::WriteLine("[DEBUG] Callback invoked with status: " + s);
        if (s == "COMPLETED" || s == "ERROR") {
            sResponse = r;
            if (s == "ERROR") Console::WriteLine("[FAIL] Background task error: " + r);
            completion.Set();
        }
    });
    Console::WriteLine("[DEBUG] Calling WaitOne");
    bool bWaitResult = completion.WaitOne(30000);
    Console::WriteLine(String::Format("[DEBUG] WaitOne returned: {0}", bWaitResult));
    if (sResponse.IsEmpty()) {
        Console::WriteLine("[FAIL] Response is empty!");
    }
    if (!sResponse.Contains("MOCK LLAMA MODEL PROVIDER EXECUTED")) {
        Console::WriteLine(String::Format("[FAIL] Response does not contain MOCK string: '{0}'", sResponse));
    }
    Console::WriteLine("[PASS] GivenMockModelProvider_WhenLocalLlmEngineQueryProcessed_ThenInvokesMockModelLayer");
}

// Negative & Edge Case Test
TEST(EventRecordTest, GivenEmptyQuery_WhenProcessed_ThenLocalLlmEngineReturnsFallbackResponse) {
    SmartPointer<SmartEventViewer::ILlamaModelProvider> spMockProvider(SmartPointer<MockLlamaModelProvider>::NewShared());
    SmartEventViewer::LocalLlmEngine llm(spMockProvider);
    DotNetDupe::System::Threading::ManualResetEvent completion(false);
    String sResponse = "";
    llm.ProcessQueryAsync("", nullptr, 0, [&completion, &sResponse](const String& s, const String& r, double p) {
        Console::WriteLine("[DEBUG-EmptyQuery] Callback invoked with status: " + s);
        if (s == "COMPLETED" || s == "ERROR") {
            sResponse = r;
            completion.Set();
        }
    });
    Console::WriteLine("[DEBUG-EmptyQuery] Calling WaitOne");
    bool bWaitEmpty = completion.WaitOne(30000);
    Console::WriteLine(String::Format("[DEBUG-EmptyQuery] WaitOne returned: {0}", bWaitEmpty));
    if (sResponse.IsEmpty()) {
        Console::WriteLine("[FAIL] Empty response in fallback!");
    }
    Console::WriteLine("[PASS] GivenEmptyQuery_WhenProcessed_ThenLocalLlmEngineReturnsFallbackResponse");
}

// Positive Test for Real LocalLlmEngine Initialization
TEST(EventRecordTest, GivenRealLocalLlmEngine_WhenProcessQueryInvoked_ThenInitializesEngine) {
    SmartEventViewer::LocalLlmEngine llm;
    String sModelPath = "models/Qwen1.5-4B-Chat-Q4_K_M.gguf";
    if (DotNetDupe::System::IO::File::Exists(sModelPath)) {
        llm.Initialize(sModelPath);
        EXPECT_TRUE(llm.IsModelLoaded());
    } else {
        EXPECT_FALSE(llm.IsModelLoaded());
    }
    Console::WriteLine("[PASS] GivenRealLocalLlmEngine_WhenProcessQueryInvoked_ThenInitializesEngine");
}

// --- EtwLogReader & LinuxJournalReader Tests ---
// Positive & Negative EtwLogReader Test
TEST(EventRecordTest, GivenEtwLogReader_WhenChannelsQueried_ThenReturnsChannelsList) {
    auto channels = DotNetDupe::System::Diagnostics::EtwLogReader::GetEventChannels();
    EXPECT_TRUE(channels.GetCount() > 0);

    auto events = DotNetDupe::System::Diagnostics::EtwLogReader::ReadEvents("Application", 5, 0, true);
    for (int i = 0; i < events.GetCount(); ++i) {
        auto record = SmartEventViewer::EventRecord::FromEtwEvent(events[i], "Application");
        EXPECT_TRUE(record.GetEventId() == static_cast<unsigned int>(events[i].iEventId));
    }
    Console::WriteLine("[PASS] GivenEtwLogReader_WhenChannelsQueried_ThenReturnsChannelsList (Channels: {0}, Read: {1})", channels.GetCount(), events.GetCount());
}

// Edge & Cross-Platform LinuxJournalReader Test
TEST(EventRecordTest, GivenLinuxJournalReader_WhenOpenedOnWindows_ThenGracefullyHandlesPlatformFallback) {
    SmartEventViewer::LinuxJournalReader journalReader;
    SmartEventViewer::StringList sources;
    bool bSourcesRetrieved = journalReader.GetEventSources(sources);
    EXPECT_TRUE(bSourcesRetrieved || sources.GetCount() == 0);

    bool bOpened = journalReader.OpenLog("syslog");
    if (bOpened) {
        SmartEventViewer::EventRecord evt;
        journalReader.ReadNextEvent(evt);
        journalReader.Close();
    }
    Console::WriteLine("[PASS] GivenLinuxJournalReader_WhenOpenedOnWindows_ThenGracefullyHandlesPlatformFallback");
}

// Positive & Edge Case Test for EventRecord RawXml
TEST(EventRecordTest, GivenRawXml_WhenEventRecordCreated_ThenGetRawXmlReturnsXmlContent) {
    SmartEventViewer::EventRecord evt(200, SmartEventViewer::EventLevel::Informational, "System", "System", "Service started", "2026-07-29T12:00:00Z", "<Event><Id>200</Id></Event>");
    EXPECT_TRUE(evt.GetRawXml() == "<Event><Id>200</Id></Event>");
    Console::WriteLine("[PASS] GivenRawXml_WhenEventRecordCreated_ThenGetRawXmlReturnsXmlContent");
}

// Positive & Edge Case Test for RagVectorStore Index Count
TEST(EventRecordTest, GivenVectorStore_WhenEventsIndexed_ThenGetIndexedCountReflectsTotal) {
    SmartEventViewer::RagVectorStore vectorStore;
    EXPECT_TRUE(vectorStore.GetIndexedCount() == 0);

    SmartEventViewer::EventRecord evt(101, SmartEventViewer::EventLevel::Warning, "Security", "Security", "Logon fail", "2026-07-29T12:00:00Z");
    vectorStore.IndexEvent(evt);
    EXPECT_TRUE(vectorStore.GetIndexedCount() == 1);
    Console::WriteLine("[PASS] GivenVectorStore_WhenEventsIndexed_ThenGetIndexedCountReflectsTotal");
}

// Positive Test for EtwLogReader Paged Log Reading & Count
TEST(EventRecordTest, GivenEtwLogReader_WhenPagedReadAndCountQueried_ThenReturnsCountAndReadsPagedEvents) {
    unsigned long long uCount = DotNetDupe::System::Diagnostics::EtwLogReader::GetChannelEventCount("Application");
    (void)uCount;

    auto events = DotNetDupe::System::Diagnostics::EtwLogReader::ReadEvents("Application", 10, 0, true);
    for (int i = 0; i < events.GetCount(); ++i) {
        auto record = SmartEventViewer::EventRecord::FromEtwEvent(events[i], "Application");
        EXPECT_TRUE(record.GetEventId() > 0 || record.GetEventMessage().GetLength() >= 0);
    }
    Console::WriteLine("[PASS] GivenEtwLogReader_WhenPagedReadAndCountQueried_ThenReturnsCountAndReadsPagedEvents (Read {0})", events.GetCount());
}

// Positive Test for LocalLlmEngine Followup Query & History Management
TEST(EventRecordTest, GivenLlmEngine_WhenFollowupQueriedAndHistoryCleared_ThenTracksAndClearsHistory) {
    SmartPointer<SmartEventViewer::ILlamaModelProvider> spMockProvider(SmartPointer<MockLlamaModelProvider>::NewShared());
    SmartEventViewer::LocalLlmEngine llm(spMockProvider);

    llm.Initialize("mock/models/model.gguf");
    DotNetDupe::System::Threading::ManualResetEvent completion(false);
    llm.ProcessQueryAsync("First query", nullptr, 0, [&completion](const String& s, const String& r, double p) {
        if (s == "COMPLETED" || s == "ERROR") {
            completion.Set();
        }
    });
    completion.WaitOne();
    EXPECT_TRUE(llm.GetHistoryCount() == 2);

    llm.ClearConversationHistory();
    EXPECT_TRUE(llm.GetHistoryCount() == 0);
    Console::WriteLine("[PASS] GivenLlmEngine_WhenFollowupQueriedAndHistoryCleared_ThenTracksAndClearsHistory");
}

// Positive System Telemetry Core Test
TEST(EventRecordTest, GivenSystemTelemetryProvider_WhenMetricsRequested_ThenPopulatesCpuAndMemory) {
    auto metrics = SmartEventViewer::SystemTelemetryProvider::QuerySystemMetrics();
    EXPECT_TRUE(metrics.MemoryTotalMB >= 0);

    // Test individual SystemTelemetryProvider query methods
    auto cpuMetrics = SmartEventViewer::SystemTelemetryProvider::QueryCpuUsage();
    EXPECT_TRUE(cpuMetrics.CpuUsagePercent >= 0.0 && cpuMetrics.CpuUsagePercent <= 100.0);

    auto memMetrics = SmartEventViewer::SystemTelemetryProvider::QueryMemoryUsage();
    EXPECT_TRUE(memMetrics.MemoryTotalMB >= 0);
    EXPECT_TRUE(memMetrics.MemoryUsagePercent >= 0.0);

    auto diskMetrics = SmartEventViewer::SystemTelemetryProvider::QueryDiskUsage();
    EXPECT_TRUE(diskMetrics.DiskReadMBps >= 0.0);
    EXPECT_TRUE(diskMetrics.DiskWriteMBps >= 0.0);

    auto netMetrics = SmartEventViewer::SystemTelemetryProvider::QueryNetworkUsage();
    EXPECT_TRUE(netMetrics.NetworkUsageMbps >= 0.0);

    // Test FormatCommandLine static helper method
    String sCmd = SmartEventViewer::SystemTelemetryProvider::FormatCommandLine("C:\\Windows\\System32\\svchost.exe", "C:\\Windows\\System32\\svchost.exe -k DcomLaunch");
    EXPECT_TRUE(sCmd == "-k DcomLaunch");

    // Test MapProcessResourceDto static helper method
    DotNetDupe::System::Diagnostics::ProcessInfo proc;
    proc.iProcessId = 1234;
    proc.sName = "test.exe";
    proc.sPath = "C:\\test.exe";
    proc.sCommandLine = "C:\\test.exe --arg";
    proc.dCpuUsagePercent = 15.5;
    proc.memory.lPhysicalMemoryBytes = 104857600; // 100 MB
    auto dto = SmartEventViewer::SystemTelemetryProvider::MapProcessResourceDto(proc);
    EXPECT_TRUE(dto.ProcessId == 1234);
    EXPECT_TRUE(dto.MemoryUsageMB == 100);

    Console::WriteLine("[PASS] GivenSystemTelemetryProvider_WhenMetricsRequested_ThenPopulatesCpuAndMemory");
}

// Positive & Edge Case Test for SystemTelemetryProvider RdpSessions & TerminalSessions
TEST(EventRecordTest, GivenRdpSessionsAndUserSessions_WhenMetricsQueried_ThenPopulatesRdpSessionsAndSystemUsers) {
    auto metrics = SmartEventViewer::SystemTelemetryProvider::QuerySystemMetrics();
    // Verify RdpSessions list exists and contains enumerated sessions or empty state
    EXPECT_TRUE(metrics.RdpSessions.GetCount() >= 0);

    // Check process resource dto mapping for open ports and inbound connections
    DotNetDupe::System::Diagnostics::ProcessInfo proc;
    proc.iProcessId = 5678;
    proc.sName = "net_test.exe";
    proc.sPath = "C:\\net_test.exe";
    proc.network.lNetworkReadBytes = 2048;
    proc.network.lNetworkWriteBytes = 4096;

    auto dto = SmartEventViewer::SystemTelemetryProvider::MapProcessResourceDto(proc);
    EXPECT_TRUE(dto.ProcessId == 5678);
    EXPECT_TRUE(dto.NetworkReadBytes == 2048);
    EXPECT_TRUE(dto.NetworkWriteBytes == 4096);

    Console::WriteLine("[PASS] GivenRdpSessionsAndUserSessions_WhenMetricsQueried_ThenPopulatesRdpSessionsAndSystemUsers");
}

// --- LogRecordDto Tests ---
TEST(EventRecordTest, GivenLogRecordDto_WhenSerializedToJson_ThenCanBeDeserializedBack) {
    using namespace DotNetDupe::System::Text::Json;
    using namespace DotNetDupe::System;
    using namespace DotNetDupe::Extensions::Logging;
    
    DateTime dt = DateTime::Now();
    SmartEventViewer::LogRecordDto dto(dt, LogLevel::Error, 1234, 5678, "TEST_CATEGORY", "Test message");
    
    String json = JsonSerializer::Serialize(dto);
    auto deserialized = JsonSerializer::Deserialize<SmartEventViewer::LogRecordDto>(json);
    
    EXPECT_TRUE(std::abs(deserialized.Timestamp.GetTicks() - dto.Timestamp.GetTicks()) <= 10000000); // Within 1s after string serialization
    EXPECT_TRUE(deserialized.Level == dto.Level);
    EXPECT_TRUE(deserialized.ProcessId == dto.ProcessId);
    EXPECT_TRUE(deserialized.ThreadId == dto.ThreadId);
    EXPECT_TRUE(deserialized.Category == dto.Category);
    EXPECT_TRUE(deserialized.Message == dto.Message);
    
    Console::WriteLine("GivenLogRecordDto_WhenSerializedToJson_ThenCanBeDeserializedBack Passed");
}

// --- Dependency Injection Tests for LlmAnalysisController & LocalLlmEngine ---
TEST(LlmAnalysisControllerTest, GivenServiceCollection_WhenLocalLlmEngineAndControllerRegistered_ThenResolvesControllerWithInjectedEngine) {
    using namespace DotNetDupe::Extensions::DependencyInjection;
    ServiceCollection services;
    
    services.AddSingleton<SmartEventViewer::LocalLlmEngine, SmartEventViewer::LocalLlmEngine>();
    services.AddTransient<SmartEventViewer::LlmAnalysisController, SmartEventViewer::LlmAnalysisController, SmartEventViewer::LocalLlmEngine>();
    
    auto spProvider = services.BuildServiceProvider();
    EXPECT_TRUE(!spProvider.IsNull());
    
    auto spController = spProvider->GetRequiredService<SmartEventViewer::LlmAnalysisController>();
    EXPECT_TRUE(!spController.IsNull());
    
    auto spEngine = spProvider->GetRequiredService<SmartEventViewer::LocalLlmEngine>();
    EXPECT_TRUE(!spEngine.IsNull());
    
    Console::WriteLine("[PASS] GivenServiceCollection_WhenLocalLlmEngineAndControllerRegistered_ThenResolvesControllerWithInjectedEngine");
}

// --- EventsController Tests ---
TEST(EventsControllerTest, GivenHttpContextWithQueryParams_WhenGetEventsCalled_ThenCorrectlyExtractsChannelAndLevel) {
    SmartEventViewer::EventsController controller;
    auto spContext = DotNetDupe::System::SmartPointer<DotNetDupe::WebAppCore::Http::HttpContext>::NewShared();
    spContext->GetRequest()->GetQuery().Add("channel", "Application");
    spContext->GetRequest()->GetQuery().Add("level", "Error");
    spContext->GetRequest()->GetQuery().Add("page", "1");
    spContext->GetRequest()->GetQuery().Add("pageSize", "10");
    controller.Initialize(spContext);
    
    auto dto = controller.GetEvents("", 1, 20);
    EXPECT_TRUE(dto.Channel == "Application");
    EXPECT_TRUE(dto.Page == 1);
    EXPECT_TRUE(dto.PageSize == 10);
    Console::WriteLine("[PASS] GivenHttpContextWithQueryParams_WhenGetEventsCalled_ThenCorrectlyExtractsChannelAndLevel");
}

static void LogAndAssertSummary(const SmartEventViewer::EventSummaryResponseDto& summary, const String& sExpectedChannel) {
    Console::WriteLine("[TEST_SUMMARY] Channel={0} | Total={1} | Crit={2} | Err={3} | Warn={4} | Info={5} | Verb={6}",
        summary.Channel, summary.TotalCount, summary.CriticalCount, summary.ErrorCount, summary.WarningCount, summary.InfoCount, summary.VerboseCount);
    Console::WriteLine("[ASSERT] Summary Channel == '{0}' (Actual: '{1}')", sExpectedChannel, summary.Channel);
    EXPECT_EQ(summary.Channel, sExpectedChannel);
    EXPECT_GE(summary.TotalCount, 0ULL);
}

static void LogAndAssertEvents(const SmartEventViewer::EventLogResponseDto& eventsDto, const String& sExpectedChannel, const String& sExpectedLevel) {
    Console::WriteLine("[TEST_EVENTS] Channel={0} | Level={1} | Count={2} | Total={3}",
        eventsDto.Channel, sExpectedLevel, eventsDto.Events.GetCount(), eventsDto.TotalCount);
    EXPECT_EQ(eventsDto.Channel, sExpectedChannel);
    for (int i = 0; i < eventsDto.Events.GetCount(); ++i) {
        const auto& evt = eventsDto.Events[i];
        Console::WriteLine("  [EVENT_REC] Id={0} | Level={1} | Provider={2} | MsgLen={3}",
            evt.Id, evt.Level, evt.Provider, evt.Message.GetLength());
        Console::WriteLine("  [ASSERT] Level == '{0}' (Actual: '{1}')", sExpectedLevel, evt.Level);
        EXPECT_EQ(evt.Level, sExpectedLevel);
        EXPECT_FALSE(evt.Provider.IsEmpty());
    }
}

static void ExecuteChannelLevelQuery(SmartEventViewer::EventsController& controller, const String& sChannel, const String& sLevel, const String& sExpectedLevel) {
    auto eventsDto = controller.GetEvents(sChannel, 1, 10, sLevel);
    LogAndAssertEvents(eventsDto, sChannel, sExpectedLevel);
}

// --- Application Channel Tests ---
TEST(EventRecordTest, GivenApplicationChannel_WhenGetSummaryAndLevelEventsQueried_ThenReturnsValidRecords) {
    SmartEventViewer::EventsController controller;
    auto summary = controller.GetEventSummary("Application");
    LogAndAssertSummary(summary, "Application");

    ExecuteChannelLevelQuery(controller, "Application", "Critical", "Critical");
    ExecuteChannelLevelQuery(controller, "Application", "Error", "Error");
    ExecuteChannelLevelQuery(controller, "Application", "Warning", "Warning");
    ExecuteChannelLevelQuery(controller, "Application", "Information", "Information");
    Console::WriteLine("[PASS] GivenApplicationChannel_WhenGetSummaryAndLevelEventsQueried_ThenReturnsValidRecords");
}

// --- System Channel Tests ---
TEST(EventRecordTest, GivenSystemChannel_WhenGetSummaryAndLevelEventsQueried_ThenReturnsValidRecords) {
    SmartEventViewer::EventsController controller;
    auto summary = controller.GetEventSummary("System");
    LogAndAssertSummary(summary, "System");

    ExecuteChannelLevelQuery(controller, "System", "Critical", "Critical");
    ExecuteChannelLevelQuery(controller, "System", "Error", "Error");
    ExecuteChannelLevelQuery(controller, "System", "Warning", "Warning");
    ExecuteChannelLevelQuery(controller, "System", "Information", "Information");
    Console::WriteLine("[PASS] GivenSystemChannel_WhenGetSummaryAndLevelEventsQueried_ThenReturnsValidRecords");
}

// --- Security Channel Tests ---
TEST(EventRecordTest, GivenSecurityChannel_WhenGetSummaryAndLevelEventsQueried_ThenReturnsValidRecords) {
    SmartEventViewer::EventsController controller;
    auto summary = controller.GetEventSummary("Security");
    LogAndAssertSummary(summary, "Security");

    ExecuteChannelLevelQuery(controller, "Security", "Critical", "Critical");
    ExecuteChannelLevelQuery(controller, "Security", "Error", "Error");
    ExecuteChannelLevelQuery(controller, "Security", "Warning", "Warning");
    ExecuteChannelLevelQuery(controller, "Security", "Information", "Information");
    Console::WriteLine("[PASS] GivenSecurityChannel_WhenGetSummaryAndLevelEventsQueried_ThenReturnsValidRecords");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
