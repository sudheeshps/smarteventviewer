#include <iostream>
#include <cassert>
#include "Core/EventRecord.h"
#include "Core/AnomalyEngine.h"
#include "Platform/WinEventLogReader.h"
#include "Platform/LinuxJournalReader.h"
#include "Ai/LocalLlmEngine.h"
#include "Ai/RagVectorStore.h"
#include "Core/NativeRestServer.h"

using String = DotNetDupe::System::String;

void Test_RagVectorStore_IndexingAndQuery()
{
    SmartEventViewer::RagVectorStore vectorStore;
    SmartEventViewer::EventRecord evt1(4625, SmartEventViewer::EventLevel::Warning, String("Security"), String("Security"), String("Failed logon"), String("2026-07-29T12:00:00Z"));
    SmartEventViewer::EventRecord evt2(7045, SmartEventViewer::EventLevel::Informational, String("Service Control Manager"), String("System"), String("Service installed"), String("2026-07-29T12:01:00Z"));

    vectorStore.IndexEvent(evt1);
    vectorStore.IndexEvent(evt2);

    assert(vectorStore.GetIndexedCount() == 2);

    auto matches = vectorStore.QuerySimilarEvents(String("privilege escalation or failed logon"), 1);
    assert(matches.GetCount() == 1);
    assert(matches[0].GetEventId() == 4625);

    std::cout << "[PASS] Test_RagVectorStore_IndexingAndQuery\n";
}

void Test_WinEventLogReader_EnumerateSources()
{
    SmartEventViewer::WinEventLogReader reader;
    auto sources = reader.EnumerateEventSources();
    std::cout << "[INFO] Windows Event Sources Count: " << sources.GetCount() << "\n";
    assert(sources.GetCount() > 0);
    std::cout << "[PASS] Test_WinEventLogReader_EnumerateSources\n";
}

void Test_EventRecord_Construction()
{
    SmartEventViewer::EventRecord record(4625, SmartEventViewer::EventLevel::Warning, String("Microsoft-Windows-Security-Auditing"), String("Security"), String("Failed logon"), String("2026-07-29T11:00:00Z"));
    assert(record.GetEventId() == 4625);
    assert(record.GetLevel() == SmartEventViewer::EventLevel::Warning);
    std::cout << "[PASS] Test_EventRecord_Construction\n";
}

void Test_AnomalyEngine_RiskEvaluation()
{
    SmartEventViewer::EventRecord failedLogonEvent(4625, SmartEventViewer::EventLevel::Warning, String("Security"), String("Security"), String("Logon failed"), String("2026"));
    SmartEventViewer::RiskLevel risk = SmartEventViewer::AnomalyEngine::EvaluateRisk(failedLogonEvent);
    assert(risk == SmartEventViewer::RiskLevel::High);
    std::cout << "[PASS] Test_AnomalyEngine_RiskEvaluation\n";
}

void Test_WinEventLogReader()
{
    SmartEventViewer::WinEventLogReader reader;
    bool bOpened = reader.OpenLog(String("Security"));
    SmartEventViewer::EventRecord record;
    bool bRead = reader.ReadNextEvent(record);
    assert(bRead == true);
    assert(record.GetEventId() == 4625);
    std::cout << "[PASS] Test_WinEventLogReader\n";
}

void Test_LocalLlmEngine()
{
    SmartEventViewer::LocalLlmEngine llm;
    llm.Initialize(String("models/Llama-3-8B-Instruct.Q4_K_M.gguf"));
    SmartEventViewer::EventRecord events[1];
    String sResult = llm.ProcessQuery(String("Analyze high risk events"), events, 1);
    assert(!sResult.IsEmpty());
    std::cout << "[PASS] Test_LocalLlmEngine\n";
}

void Test_WebApplication_Lifecycle()
{
    SmartEventViewer::WebApplication app = SmartEventViewer::WebApplication::CreateBuilder(0, nullptr).Build();
    bool bStarted = app.Run(8080);
    assert(bStarted == true);
    assert(app.IsRunning() == true);
    assert(app.GetPort() == 8080);

    String sJsonChannels = app.GetController().GetChannels();
    assert(!sJsonChannels.IsEmpty());

    String sJsonEvents = app.GetController().GetEvents(String("Application"));
    assert(!sJsonEvents.IsEmpty());

    app.Stop();
    assert(app.IsRunning() == false);
    std::cout << "[PASS] Test_WebApplication_Lifecycle\n";
}

int main()
{
    std::cout << "--- Running SmartEventViewer Test Suite ---\n";
    Test_RagVectorStore_IndexingAndQuery();
    Test_WinEventLogReader_EnumerateSources();
    Test_EventRecord_Construction();
    Test_AnomalyEngine_RiskEvaluation();
    Test_WinEventLogReader();
    Test_LocalLlmEngine();
    Test_WebApplication_Lifecycle();
    std::cout << "--- All Tests Passed Successfully ---\n";
    return 0;
}
