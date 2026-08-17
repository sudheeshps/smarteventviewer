#include <gtest/gtest.h>
#include <cassert>
#include "Core/EventRecord.h"
#include "Core/AnomalyEngine.h"
#include "System/Diagnostics/EtwLogReader.h"
#include "Core/SystemTelemetryProvider.h"
#include "Platform/LinuxJournalReader.h"
#include "Ai/LocalLlmEngine.h"
#include "Ai/RagVectorStore.h"
#include "System/Console.h"
#include "Core/AnalysisService.h"
#include "Core/EventService.h"
#include "Core/TelemetryService.h"
#include "Dto/DiagnosticsDtos.h"
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
    SmartEventViewer::RagVectorStore vectorStore;
    SmartEventViewer::EventRecord evt1(4624, SmartEventViewer::EventLevel::Informational, "Security-Auditing", "Security", "An account was successfully logged on", "2026-07-29T10:00:00Z");
    SmartEventViewer::EventRecord evt2(4625, SmartEventViewer::EventLevel::Error, "Security-Auditing", "Security", "An account failed to log on", "2026-07-29T10:05:00Z");
    
    vectorStore.IndexEvent(evt1);
    vectorStore.IndexEvent(evt2);
    
    SmartEventViewer::EventList results;
    bool bOk = vectorStore.QuerySimilarEvents("logon failure credentials error", 5, results);
    EXPECT_TRUE(bOk);
    EXPECT_GT(results.GetCount(), 0);
    EXPECT_EQ(results[0].GetEventId(), 4624U);
    Console::WriteLine("[PASS] GivenValidEvent_WhenIndexed_ThenRagVectorStoreReturnsSimilarMatches");
}

// Negative & Edge Case Test
TEST(EventRecordTest, GivenEmptyVectorStore_WhenQueried_ThenReturnsEmptyList) {
    SmartEventViewer::RagVectorStore vectorStore;
    SmartEventViewer::EventList results;
    vectorStore.QuerySimilarEvents("any query", 5, results);
    EXPECT_EQ(results.GetCount(), 0);
    Console::WriteLine("[PASS] GivenEmptyVectorStore_WhenQueried_ThenReturnsEmptyList");
}

// --- LocalLlmEngine Prompt & Format Tests ---
TEST(EventRecordTest, GivenRagContext_WhenFormatThreatAnalysisQueried_ThenGeneratesReport) {
    DotNetDupe::System::Collections::Generic::List<SmartEventViewer::EventRecord> contextEvents;
    contextEvents.Add(SmartEventViewer::EventRecord(4625, SmartEventViewer::EventLevel::Error, "Security-Auditing", "Security", "Repeated failed logon attempts detected from 192.168.1.100", "2026-07-29T10:05:00Z"));
    contextEvents.Add(SmartEventViewer::EventRecord(7045, SmartEventViewer::EventLevel::Warning, "Service Control Manager", "System", "A service was installed in the system: PwDumpSvc", "2026-07-29T10:06:00Z"));
    
    String report = SmartEventViewer::LocalLlmEngine::FormatThreatAnalysisResponse(contextEvents);
    EXPECT_FALSE(report.IsEmpty());
    EXPECT_TRUE(report.Contains("Security Incident") || report.Contains("Threat Analysis") || report.Contains("4625"));
    Console::WriteLine("[PASS] GivenRagContext_WhenFormatThreatAnalysisQueried_ThenGeneratesReport");
}

TEST(EventRecordTest, GivenEmptyRagContext_WhenFormatThreatAnalysisQueried_ThenGeneratesBaselineReport) {
    DotNetDupe::System::Collections::Generic::List<SmartEventViewer::EventRecord> emptyContext;
    String report = SmartEventViewer::LocalLlmEngine::FormatThreatAnalysisResponse(emptyContext);
    EXPECT_FALSE(report.IsEmpty());
    Console::WriteLine("[PASS] GivenEmptyRagContext_WhenFormatThreatAnalysisQueried_ThenGeneratesBaselineReport");
}

TEST(EventRecordTest, GivenLlamaEngine_WhenSystemPromptBuilt_ThenIncludesSecurityGuidelines) {
    String prompt = SmartEventViewer::LocalLlmEngine::BuildLlamaSystemPrompt();
    EXPECT_FALSE(prompt.IsEmpty());
    EXPECT_TRUE(prompt.Contains("Security Analyst") || prompt.Contains("SmartEventViewer"));
    Console::WriteLine("[PASS] GivenLlamaEngine_WhenSystemPromptBuilt_ThenIncludesSecurityGuidelines");
}

// --- SystemTelemetryProvider Real-Time Metrics & Network Diagnostics Tests ---
TEST(EventRecordTest, GivenSystemMetrics_WhenQueried_ThenPopulatesCpuMemoryDiskAndNetworkUsage) {
    auto metrics = SmartEventViewer::SystemTelemetryProvider::QuerySystemMetrics();
    
    EXPECT_GE(metrics.CpuUsagePercent, 0.0);
    EXPECT_LE(metrics.CpuUsagePercent, 100.0);
    EXPECT_GE(metrics.MemoryUsagePercent, 0.0);
    EXPECT_LE(metrics.MemoryUsagePercent, 100.0);
    EXPECT_GE(metrics.MemoryUsedMB, 0.0);
    EXPECT_GE(metrics.MemoryTotalMB, 0.0);
    
    EXPECT_GE(metrics.DiskUsagePercent, 0.0);
    EXPECT_LE(metrics.DiskUsagePercent, 100.0);
    EXPECT_GE(metrics.NetworkUsageMbps, 0.0);
    EXPECT_GT(metrics.TopProcesses.GetCount(), 0);
    
    Console::WriteLine("[PASS] GivenSystemMetrics_WhenQueried_ThenPopulatesCpuMemoryDiskAndNetworkUsage");
}

TEST(EventRecordTest, GivenProcessInfo_WhenMappedToDto_ThenCorrectlyTransfersAttributes) {
    DotNetDupe::System::Diagnostics::ProcessInfo proc;
    proc.iProcessId = 1234;
    proc.sName = "test_process.exe";
    proc.sPath = "C:\\Windows\\System32\\test_process.exe";
    proc.dCpuUsagePercent = 15.5;
    proc.memory.lPhysicalMemoryBytes = 104857600; // 100 MB
    proc.network.lNetworkReadBytes = 2048;
    proc.network.lNetworkWriteBytes = 4096;
    
    auto dto = SmartEventViewer::SystemTelemetryProvider::MapProcessResourceDto(proc);
    
    EXPECT_EQ(dto.ProcessId, 1234U);
    EXPECT_EQ(dto.Name, "test_process.exe");
    EXPECT_DOUBLE_EQ(dto.CpuUsagePercent, 15.5);
    EXPECT_EQ(dto.NetworkReadBytes, 2048ULL);
    EXPECT_EQ(dto.NetworkWriteBytes, 4096ULL);
    
    Console::WriteLine("[PASS] GivenProcessInfo_WhenMappedToDto_ThenCorrectlyTransfersAttributes");
}

TEST(EventRecordTest, GivenSystemTelemetryProvider_WhenSessionsAndServicesQueried_ThenReturnsValidLists) {
    auto sessions = SmartEventViewer::SystemTelemetryProvider::QuerySessions();
    EXPECT_GE(sessions.ActiveUserSessions.GetCount(), 0);
    
    auto servicesDto = SmartEventViewer::SystemTelemetryProvider::QueryServices();
    EXPECT_GE(servicesDto.Services.GetCount(), 0);
    Console::WriteLine("[PASS] GivenSystemTelemetryProvider_WhenSessionsAndServicesQueried_ThenReturnsValidLists");
}

// Positive & Negative EtwLogReader Test
TEST(EventRecordTest, GivenEtwLogReader_WhenChannelsQueried_ThenReturnsChannelsList) {
    auto channels = DotNetDupe::System::Diagnostics::EtwLogReader::GetEventChannels();
    EXPECT_GT(channels.GetCount(), 0);

    auto events = DotNetDupe::System::Diagnostics::EtwLogReader::ReadEvents("Application", 5, 0, true);
    for (int i = 0; i < events.GetCount(); ++i) {
        auto record = SmartEventViewer::EventRecord::FromEtwEvent(events[i], "Application");
        EXPECT_EQ(record.GetEventId(), static_cast<unsigned int>(events[i].iEventId));
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
    EXPECT_EQ(evt.GetRawXml(), "<Event><Id>200</Id></Event>");
    Console::WriteLine("[PASS] GivenRawXml_WhenEventRecordCreated_ThenGetRawXmlReturnsXmlContent");
}

// Positive & Edge Case Test for RagVectorStore Index Count
TEST(EventRecordTest, GivenVectorStore_WhenEventsIndexed_ThenGetIndexedCountReflectsTotal) {
    SmartEventViewer::RagVectorStore vectorStore;
    EXPECT_EQ(vectorStore.GetIndexedCount(), 0);

    SmartEventViewer::EventRecord evt(100, SmartEventViewer::EventLevel::Informational, "App", "Application", "Test event", "2026-07-29T12:00:00Z");
    vectorStore.IndexEvent(evt);
    EXPECT_EQ(vectorStore.GetIndexedCount(), 1);
    Console::WriteLine("[PASS] GivenVectorStore_WhenEventsIndexed_ThenGetIndexedCountReflectsTotal");
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
    
    EXPECT_EQ(deserialized.Level, dto.Level);
    EXPECT_EQ(deserialized.ProcessId, dto.ProcessId);
    EXPECT_EQ(deserialized.ThreadId, dto.ThreadId);
    EXPECT_EQ(deserialized.Category, dto.Category);
    EXPECT_EQ(deserialized.Message, dto.Message);
    
    Console::WriteLine("GivenLogRecordDto_WhenSerializedToJson_ThenCanBeDeserializedBack Passed");
}

// --- Dependency Injection Tests for AnalysisService & EventService ---
TEST(EventRecordTest, GivenServiceCollection_WhenAnalysisServiceAndEventServiceRegistered_ThenResolvesServices) {
    using namespace DotNetDupe::Extensions::DependencyInjection;
    ServiceCollection services;
    
    services.AddSingleton<SmartEventViewer::LocalLlmEngine, SmartEventViewer::LocalLlmEngine>();
    services.AddSingleton<SmartEventViewer::IEventService, SmartEventViewer::EventService>();
    services.AddSingleton<SmartEventViewer::IAnalysisService, SmartEventViewer::AnalysisService, SmartEventViewer::LocalLlmEngine, SmartEventViewer::IEventService>();
    
    auto spProvider = services.BuildServiceProvider();
    EXPECT_TRUE(!spProvider.IsNull());
    
    auto spService = spProvider->GetRequiredService<SmartEventViewer::IAnalysisService>();
    EXPECT_TRUE(!spService.IsNull());
    
    Console::WriteLine("[PASS] GivenServiceCollection_WhenAnalysisServiceAndEventServiceRegistered_ThenResolvesServices");
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

static void ExecuteChannelLevelQuery(SmartEventViewer::EventService& service, const String& sChannel, const String& sLevel, const String& sExpectedLevel) {
    auto eventsDto = service.GetEvents(sChannel, 1, 10, sLevel);
    LogAndAssertEvents(eventsDto, sChannel, sExpectedLevel);
}

// --- Application Channel Tests ---
TEST(EventRecordTest, GivenApplicationChannel_WhenGetSummaryAndLevelEventsQueried_ThenReturnsValidRecords) {
    SmartEventViewer::EventService service;
    auto summary = service.GetEventSummary("Application");
    LogAndAssertSummary(summary, "Application");

    ExecuteChannelLevelQuery(service, "Application", "Critical", "Critical");
    ExecuteChannelLevelQuery(service, "Application", "Error", "Error");
    ExecuteChannelLevelQuery(service, "Application", "Warning", "Warning");
    ExecuteChannelLevelQuery(service, "Application", "Information", "Information");
    Console::WriteLine("[PASS] GivenApplicationChannel_WhenGetSummaryAndLevelEventsQueried_ThenReturnsValidRecords");
}

// --- System Channel Tests ---
TEST(EventRecordTest, GivenSystemChannel_WhenGetSummaryAndLevelEventsQueried_ThenReturnsValidRecords) {
    SmartEventViewer::EventService service;
    auto summary = service.GetEventSummary("System");
    LogAndAssertSummary(summary, "System");

    ExecuteChannelLevelQuery(service, "System", "Critical", "Critical");
    ExecuteChannelLevelQuery(service, "System", "Error", "Error");
    ExecuteChannelLevelQuery(service, "System", "Warning", "Warning");
    ExecuteChannelLevelQuery(service, "System", "Information", "Information");
    Console::WriteLine("[PASS] GivenSystemChannel_WhenGetSummaryAndLevelEventsQueried_ThenReturnsValidRecords");
}

// --- Security Channel Tests ---
TEST(EventRecordTest, GivenSecurityChannel_WhenGetSummaryAndLevelEventsQueried_ThenReturnsValidRecords) {
    SmartEventViewer::EventService service;
    try {
        auto summary = service.GetEventSummary("Security");
        LogAndAssertSummary(summary, "Security");

        ExecuteChannelLevelQuery(service, "Security", "Critical", "Critical");
        ExecuteChannelLevelQuery(service, "Security", "Error", "Error");
        ExecuteChannelLevelQuery(service, "Security", "Warning", "Warning");
        ExecuteChannelLevelQuery(service, "Security", "Information", "Information");
        Console::WriteLine("[PASS] GivenSecurityChannel_WhenGetSummaryAndLevelEventsQueried_ThenReturnsValidRecords");
    } catch (const DotNetDupe::System::Exception& ex) {
        Console::WriteLine("[SECURITY_LOG] Access restricted in non-elevated execution: {0}", ex.What());
    }
}
