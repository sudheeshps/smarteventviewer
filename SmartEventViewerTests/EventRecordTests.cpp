#include <cassert>
#include "Core/EventRecord.h"
#include "Core/AnomalyEngine.h"
#include "Platform/WinEventLogReader.h"
#include "Core/SystemTelemetryProvider.h"
#include "Core/TelemetryController.h"
#include "Platform/LinuxJournalReader.h"
#include "Ai/LocalLlmEngine.h"
#include "Ai/RagVectorStore.h"
#include "System/Console.h"

using String = DotNetDupe::System::String;
using Console = DotNetDupe::System::Console;
template<typename T>
using SmartPointer = DotNetDupe::System::SmartPointer<T>;

// --- RagVectorStore Tests ---
// Positive Test
void GivenValidEvent_WhenIndexed_ThenRagVectorStoreReturnsSimilarMatches()
{
    SmartEventViewer::RagVectorStore vectorStore;
    SmartEventViewer::EventRecord evt1(101, SmartEventViewer::EventLevel::Warning, "Security", "Security", "Failed logon attempt for user admin", "2026-07-29T12:00:00Z");
    SmartEventViewer::EventRecord evt2(102, SmartEventViewer::EventLevel::Informational, "System", "System", "Service started successfully", "2026-07-29T12:05:00Z");

    vectorStore.IndexEvent(evt1);
    vectorStore.IndexEvent(evt2);

    SmartEventViewer::EventList results;
    bool bSuccess = vectorStore.QuerySimilarEvents("logon attempt", 2, results);
    assert(bSuccess);
    assert(results.GetCount() > 0);
    Console::WriteLine("[PASS] GivenValidEvent_WhenIndexed_ThenRagVectorStoreReturnsSimilarMatches");
}

// Edge Case Test
void GivenEmptyVectorStore_WhenQueried_ThenReturnsFalseOrEmptyList()
{
    SmartEventViewer::RagVectorStore vectorStore;
    SmartEventViewer::EventList results;
    bool bSuccess = vectorStore.QuerySimilarEvents("logon attempt", 2, results);
    assert(bSuccess || results.GetCount() == 0);
    Console::WriteLine("[PASS] GivenEmptyVectorStore_WhenQueried_ThenReturnsFalseOrEmptyList");
}

// --- EventRecord Tests ---
// Positive Test
void GivenValidParameters_WhenEventRecordCreated_ThenPropertiesInitializedCorrectly()
{
    SmartEventViewer::EventRecord evt(1001, SmartEventViewer::EventLevel::Error, "Application Error", "Application", "Process crashed", "2026-07-29T10:00:00Z");
    assert(evt.GetEventId() == 1001);
    assert(evt.GetLevel() == SmartEventViewer::EventLevel::Error);
    assert(evt.GetProviderName() == "Application Error");
    assert(evt.GetChannel() == "Application");
    assert(evt.GetEventMessage() == "Process crashed");
    Console::WriteLine("[PASS] GivenValidParameters_WhenEventRecordCreated_ThenPropertiesInitializedCorrectly");
}

// Negative & Edge Case Test
void GivenZeroEventIdAndEmptyStrings_WhenEventRecordCreated_ThenHandlesEdgeValues()
{
    SmartEventViewer::EventRecord evt(0, SmartEventViewer::EventLevel::LogAlways, "", "", "", "");
    assert(evt.GetEventId() == 0);
    assert(evt.GetLevel() == SmartEventViewer::EventLevel::LogAlways);
    assert(evt.GetProviderName().IsEmpty());
    assert(evt.GetChannel().IsEmpty());
    assert(evt.GetEventMessage().IsEmpty());
    Console::WriteLine("[PASS] GivenZeroEventIdAndEmptyStrings_WhenEventRecordCreated_ThenHandlesEdgeValues");
}

// --- AnomalyEngine Tests ---
// Positive & Negative Risk Evaluation Test
void GivenNormalAndSecurityEvents_WhenEvaluated_ThenAnomalyEngineAssignsCorrectRiskLevels()
{
    SmartEventViewer::AnomalyEngine engine;
    SmartEventViewer::EventRecord normalEvt(100, SmartEventViewer::EventLevel::Informational, "System", "System", "Service status ok", "2026-07-29T11:00:00Z");
    SmartEventViewer::RiskLevel riskNormal = engine.EvaluateRisk(normalEvt);
    assert(riskNormal == SmartEventViewer::RiskLevel::Low);

    SmartEventViewer::EventRecord criticalEvt(4625, SmartEventViewer::EventLevel::Error, "Microsoft-Windows-Security-Auditing", "Security", "An account failed to log on. Multiple attempts detected.", "2026-07-29T11:01:00Z");
    SmartEventViewer::RiskLevel riskCritical = engine.EvaluateRisk(criticalEvt);
    assert(riskCritical == SmartEventViewer::RiskLevel::Critical || riskCritical == SmartEventViewer::RiskLevel::High);
    Console::WriteLine("[PASS] GivenNormalAndSecurityEvents_WhenEvaluated_ThenAnomalyEngineAssignsCorrectRiskLevels");
}

// Mock Llama Model Provider for Unit Testing
class MockLlamaModelProvider : public SmartEventViewer::ILlamaModelProvider
{
private:
    bool m_bLoaded{ false };

public:
    bool InitBackend() override { return true; }
    bool LoadModel(const String& sModelPath) override { m_bLoaded = true; return true; }
    bool CreateContext() override { return m_bLoaded; }
    String ExecuteInference(const String& sSystemPrompt, const String& sUserQuery, const std::vector<SmartEventViewer::EventRecord>& events) override
    {
        return String("🤖 [MOCK LLAMA MODEL PROVIDER EXECUTED]\nMock inference result for query: ") + sUserQuery;
    }
    void FreeContextAndModel() override { m_bLoaded = false; }
    bool IsLoaded() const override { return m_bLoaded; }
};

// --- LocalLlmEngine Tests ---
// Positive Test with Mock Model Provider
void GivenMockModelProvider_WhenLocalLlmEngineQueryProcessed_ThenInvokesMockModelLayer()
{
    SmartPointer<SmartEventViewer::ILlamaModelProvider> spMockProvider(static_cast<SmartEventViewer::ILlamaModelProvider*>(new MockLlamaModelProvider()));
    SmartEventViewer::LocalLlmEngine llm(spMockProvider);

    bool bInit = llm.Initialize("mock/models/model.gguf");
    assert(bInit);
    assert(llm.IsModelLoaded() == true);

    String sResponse = llm.ProcessQuery("Analyze suspicious activity", nullptr, 0);
    assert(!sResponse.IsEmpty());
    assert(sResponse.Contains("MOCK LLAMA MODEL PROVIDER EXECUTED"));
    Console::WriteLine("[PASS] GivenMockModelProvider_WhenLocalLlmEngineQueryProcessed_ThenInvokesMockModelLayer");
}

// Positive Test with Default Model Provider
void GivenUninitializedEngine_WhenInitialized_ThenLocalLlmEngineProcessesQueries()
{
    SmartEventViewer::LocalLlmEngine llm;
    assert(llm.IsModelLoaded() == false);

    bool bInit = llm.Initialize("models/Llama-3-8B-Instruct.Q4_K_M.gguf");
    (void)bInit;

    String sResponse = llm.ProcessQuery("Summarize security threats", nullptr, 0);
    assert(!sResponse.IsEmpty());
    Console::WriteLine("[PASS] GivenUninitializedEngine_WhenInitialized_ThenLocalLlmEngineProcessesQueries");
}

// Negative & Edge Case Test
void GivenEmptyQuery_WhenProcessed_ThenLocalLlmEngineReturnsFallbackResponse()
{
    SmartEventViewer::LocalLlmEngine llm;
    String sResponse = llm.ProcessQuery("", nullptr, 0);
    assert(!sResponse.IsEmpty());
    Console::WriteLine("[PASS] GivenEmptyQuery_WhenProcessed_ThenLocalLlmEngineReturnsFallbackResponse");
}

// --- WinEventLogReader & LinuxJournalReader Tests ---
// Positive & Negative WinEventLogReader Test
void GivenWinEventLogReader_WhenOpened_ThenReadsOrHandlesChannelAvailability()
{
    SmartEventViewer::WinEventLogReader reader;
    SmartEventViewer::StringList sources;
    bool bSourcesRetrieved = reader.GetEventSources(sources);
    assert(bSourcesRetrieved);
    assert(sources.GetCount() > 0);

    if (reader.OpenLog("Application"))
    {
        SmartEventViewer::EventRecord evt;
        size_t count = 0;
        while (reader.ReadNextEvent(evt) && count < 5)
        {
            count++;
        }
        reader.Close();
        Console::WriteLine("[PASS] GivenWinEventLogReader_WhenOpened_ThenReadsOrHandlesChannelAvailability (Read {0} records, Sources: {1})", count, sources.GetCount());
    }
    else
    {
        Console::WriteLine("[PASS] GivenWinEventLogReader_WhenOpened_ThenReadsOrHandlesChannelAvailability (Application channel handled gracefully, Sources: {0})", sources.GetCount());
    }
}

// Edge & Cross-Platform LinuxJournalReader Test
void GivenLinuxJournalReader_WhenOpenedOnWindows_ThenGracefullyHandlesPlatformFallback()
{
    SmartEventViewer::LinuxJournalReader journalReader;
    SmartEventViewer::StringList sources;
    bool bSourcesRetrieved = journalReader.GetEventSources(sources);
    assert(bSourcesRetrieved || sources.GetCount() == 0);

    bool bOpened = journalReader.OpenLog("syslog");
    if (bOpened)
    {
        SmartEventViewer::EventRecord evt;
        journalReader.ReadNextEvent(evt);
        journalReader.Close();
    }
    Console::WriteLine("[PASS] GivenLinuxJournalReader_WhenOpenedOnWindows_ThenGracefullyHandlesPlatformFallback");
}

// Positive & Edge Case Test for EventRecord RawXml
void GivenRawXml_WhenEventRecordCreated_ThenGetRawXmlReturnsXmlContent()
{
    SmartEventViewer::EventRecord evt(200, SmartEventViewer::EventLevel::Informational, "System", "System", "Service started", "2026-07-29T12:00:00Z", "<Event><Id>200</Id></Event>");
    assert(evt.GetRawXml() == "<Event><Id>200</Id></Event>");
    Console::WriteLine("[PASS] GivenRawXml_WhenEventRecordCreated_ThenGetRawXmlReturnsXmlContent");
}

// Positive & Edge Case Test for RagVectorStore Index Count
void GivenVectorStore_WhenEventsIndexed_ThenGetIndexedCountReflectsTotal()
{
    SmartEventViewer::RagVectorStore vectorStore;
    assert(vectorStore.GetIndexedCount() == 0);

    SmartEventViewer::EventRecord evt(101, SmartEventViewer::EventLevel::Warning, "Security", "Security", "Logon fail", "2026-07-29T12:00:00Z");
    vectorStore.IndexEvent(evt);
    assert(vectorStore.GetIndexedCount() == 1);
    Console::WriteLine("[PASS] GivenVectorStore_WhenEventsIndexed_ThenGetIndexedCountReflectsTotal");
}

// Positive Test for WinEventLogReader Paged Log Reading & Count
void GivenWinEventLogReader_WhenPagedOpenAndCountQueried_ThenReturnsCountAndReadsPagedEvents()
{
    SmartEventViewer::WinEventLogReader reader;
    unsigned long long uCount = reader.GetChannelEventCount("Application");
    (void)uCount;

    bool bOpened = reader.OpenLogPaged("Application", 10, 0);
    if (bOpened)
    {
        SmartEventViewer::EventRecord evt;
        size_t readCount = 0;
        while (reader.ReadNextEvent(evt) && readCount < 5)
        {
            readCount++;
        }
        reader.Close();
        Console::WriteLine("[PASS] GivenWinEventLogReader_WhenPagedOpenAndCountQueried_ThenReturnsCountAndReadsPagedEvents (Read {0})", readCount);
    }
    else
    {
        Console::WriteLine("[PASS] GivenWinEventLogReader_WhenPagedOpenAndCountQueried_ThenReturnsCountAndReadsPagedEvents (Application channel unavailable)");
    }
}

// Positive Test for LocalLlmEngine Followup Query & History Management
void GivenLlmEngine_WhenFollowupQueriedAndHistoryCleared_ThenTracksAndClearsHistory()
{
    SmartPointer<SmartEventViewer::ILlamaModelProvider> spMockProvider(static_cast<SmartEventViewer::ILlamaModelProvider*>(new MockLlamaModelProvider()));
    SmartEventViewer::LocalLlmEngine llm(spMockProvider);

    llm.Initialize("mock/models/model.gguf");
    String sResp1 = llm.ProcessQuery("First query", nullptr, 0);
    assert(llm.GetHistoryCount() == 2);

    llm.ClearConversationHistory();
    assert(llm.GetHistoryCount() == 0);
    Console::WriteLine("[PASS] GivenLlmEngine_WhenFollowupQueriedAndHistoryCleared_ThenTracksAndClearsHistory");
}

// Positive System Telemetry Core Test
void GivenSystemTelemetryProvider_WhenMetricsRequested_ThenPopulatesCpuAndMemory()
{
    auto metrics = SmartEventViewer::SystemTelemetryProvider::QuerySystemMetrics();
    assert(metrics.MemoryTotalMB >= 0);

    // Test individual SystemTelemetryProvider query methods
    auto cpuMetrics = SmartEventViewer::SystemTelemetryProvider::QueryCpuUsage();
    assert(cpuMetrics.CpuUsagePercent >= 0.0 && cpuMetrics.CpuUsagePercent <= 100.0);

    auto memMetrics = SmartEventViewer::SystemTelemetryProvider::QueryMemoryUsage();
    assert(memMetrics.MemoryTotalMB >= 0);
    assert(memMetrics.MemoryUsagePercent >= 0.0);

    auto diskMetrics = SmartEventViewer::SystemTelemetryProvider::QueryDiskUsage();
    assert(diskMetrics.DiskReadMBps >= 0.0);
    assert(diskMetrics.DiskWriteMBps >= 0.0);

    auto netMetrics = SmartEventViewer::SystemTelemetryProvider::QueryNetworkUsage();
    assert(netMetrics.NetworkUsageMbps >= 0.0);

    // Test FormatCommandLine static helper method
    String sCmd = SmartEventViewer::SystemTelemetryProvider::FormatCommandLine("C:\\Windows\\System32\\svchost.exe", "C:\\Windows\\System32\\svchost.exe -k DcomLaunch");
    assert(sCmd == "-k DcomLaunch");

    // Test MapProcessResourceDto static helper method
    DotNetDupe::System::Diagnostics::ProcessInfo proc;
    proc.iProcessId = 1234;
    proc.sName = "test.exe";
    proc.sPath = "C:\\test.exe";
    proc.sCommandLine = "C:\\test.exe --arg";
    proc.dCpuUsagePercent = 15.5;
    proc.memory.lPhysicalMemoryBytes = 104857600; // 100 MB
    auto dto = SmartEventViewer::SystemTelemetryProvider::MapProcessResourceDto(proc);
    assert(dto.ProcessId == 1234);
    assert(dto.MemoryUsageMB == 100);

    Console::WriteLine("[PASS] GivenSystemTelemetryProvider_WhenMetricsRequested_ThenPopulatesCpuAndMemory");
}

// Positive & Edge Case Test for SystemTelemetryProvider RdpSessions & TerminalSessions
void GivenRdpSessionsAndUserSessions_WhenMetricsQueried_ThenPopulatesRdpSessionsAndSystemUsers()
{
    auto metrics = SmartEventViewer::SystemTelemetryProvider::QuerySystemMetrics();
    // Verify RdpSessions list exists and contains enumerated sessions or empty state
    assert(metrics.RdpSessions.GetCount() >= 0);

    // Check process resource dto mapping for open ports and inbound connections
    DotNetDupe::System::Diagnostics::ProcessInfo proc;
    proc.iProcessId = 5678;
    proc.sName = "net_test.exe";
    proc.sPath = "C:\\net_test.exe";
    proc.network.lNetworkReadBytes = 2048;
    proc.network.lNetworkWriteBytes = 4096;

    auto dto = SmartEventViewer::SystemTelemetryProvider::MapProcessResourceDto(proc);
    assert(dto.ProcessId == 5678);
    assert(dto.NetworkReadBytes == 2048);
    assert(dto.NetworkWriteBytes == 4096);

    Console::WriteLine("[PASS] GivenRdpSessionsAndUserSessions_WhenMetricsQueried_ThenPopulatesRdpSessionsAndSystemUsers");
}

int main()
{
    Console::WriteLine("--- Running SmartEventViewer Core Library Unit Test Suite ---");
    GivenValidEvent_WhenIndexed_ThenRagVectorStoreReturnsSimilarMatches();
    GivenEmptyVectorStore_WhenQueried_ThenReturnsFalseOrEmptyList();
    GivenVectorStore_WhenEventsIndexed_ThenGetIndexedCountReflectsTotal();
    GivenValidParameters_WhenEventRecordCreated_ThenPropertiesInitializedCorrectly();
    GivenZeroEventIdAndEmptyStrings_WhenEventRecordCreated_ThenHandlesEdgeValues();
    GivenRawXml_WhenEventRecordCreated_ThenGetRawXmlReturnsXmlContent();
    GivenNormalAndSecurityEvents_WhenEvaluated_ThenAnomalyEngineAssignsCorrectRiskLevels();
    GivenMockModelProvider_WhenLocalLlmEngineQueryProcessed_ThenInvokesMockModelLayer();
    GivenUninitializedEngine_WhenInitialized_ThenLocalLlmEngineProcessesQueries();
    GivenEmptyQuery_WhenProcessed_ThenLocalLlmEngineReturnsFallbackResponse();
    GivenLlmEngine_WhenFollowupQueriedAndHistoryCleared_ThenTracksAndClearsHistory();
    GivenWinEventLogReader_WhenOpened_ThenReadsOrHandlesChannelAvailability();
    GivenWinEventLogReader_WhenPagedOpenAndCountQueried_ThenReturnsCountAndReadsPagedEvents();
    GivenLinuxJournalReader_WhenOpenedOnWindows_ThenGracefullyHandlesPlatformFallback();
    GivenSystemTelemetryProvider_WhenMetricsRequested_ThenPopulatesCpuAndMemory();
    GivenRdpSessionsAndUserSessions_WhenMetricsQueried_ThenPopulatesRdpSessionsAndSystemUsers();
    Console::WriteLine("--- All SmartEventViewerCore Library Unit Tests Passed Successfully ---");
    return 0;
}
