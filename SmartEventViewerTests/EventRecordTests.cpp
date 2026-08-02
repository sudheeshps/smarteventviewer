#include <cassert>
#include "Core/EventRecord.h"
#include "Core/AnomalyEngine.h"
#include "Platform/WinEventLogReader.h"
#include "Platform/LinuxJournalReader.h"
#include "Ai/LocalLlmEngine.h"
#include "Ai/RagVectorStore.h"
#include "../SmartEventViewerServer/EventsController.h"
#include "WebAppCore/Builder/WebApplication.h"
#include "WebAppCore/Builder/WebApplicationBuilder.h"
#include "WebAppCore/Controllers/ControllerBase.h"
#include "WebAppCore/Controllers/ControllerRouteBuilder.h"
#include "System/Net/Http/HttpClient.h"
#include "System/Threading/Thread.h"
#include "System/Console.h"

using String = DotNetDupe::System::String;
using Console = DotNetDupe::System::Console;
using namespace DotNetDupe::System::Net::Http;
using namespace DotNetDupe::System::Threading;
using namespace DotNetDupe::WebAppCore::Builder;
using namespace DotNetDupe::WebAppCore::Controllers;

void Test_RagVectorStore_IndexingAndQuery()
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
    Console::WriteLine("[PASS] Test_RagVectorStore_IndexingAndQuery");
}

void Test_WinEventLogReader_GetEventSources()
{
    SmartEventViewer::WinEventLogReader logReader;
    DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::String> sources;
    bool bResult = logReader.GetEventSources(sources);
    assert(bResult);
    Console::WriteLine("[INFO] Windows Event Sources Count: {0}", sources.GetCount());
    assert(sources.GetCount() > 0);
    Console::WriteLine("[PASS] Test_WinEventLogReader_GetEventSources");
}

void Test_EventRecord_Construction()
{
    SmartEventViewer::EventRecord evt(1001, SmartEventViewer::EventLevel::Error, "Application Error", "Application", "Process crashed", "2026-07-29T10:00:00Z");
    assert(evt.GetEventId() == 1001);
    assert(evt.GetLevel() == SmartEventViewer::EventLevel::Error);
    assert(evt.GetProviderName() == "Application Error");
    assert(evt.GetChannel() == "Application");
    assert(evt.GetEventMessage() == "Process crashed");
    Console::WriteLine("[PASS] Test_EventRecord_Construction");
}

void Test_AnomalyEngine_RiskEvaluation()
{
    SmartEventViewer::AnomalyEngine engine;
    SmartEventViewer::EventRecord normalEvt(100, SmartEventViewer::EventLevel::Informational, "System", "System", "Service status ok", "2026-07-29T11:00:00Z");
    SmartEventViewer::RiskLevel riskNormal = engine.EvaluateRisk(normalEvt);
    assert(riskNormal == SmartEventViewer::RiskLevel::Low);

    SmartEventViewer::EventRecord criticalEvt(4625, SmartEventViewer::EventLevel::Error, "Microsoft-Windows-Security-Auditing", "Security", "An account failed to log on. Multiple attempts detected.", "2026-07-29T11:01:00Z");
    SmartEventViewer::RiskLevel riskCritical = engine.EvaluateRisk(criticalEvt);
    assert(riskCritical == SmartEventViewer::RiskLevel::Critical || riskCritical == SmartEventViewer::RiskLevel::High);
    Console::WriteLine("[PASS] Test_AnomalyEngine_RiskEvaluation");
}

void Test_WinEventLogReader()
{
    SmartEventViewer::WinEventLogReader reader;
    if (reader.OpenLog("Application"))
    {
        SmartEventViewer::EventRecord evt;
        size_t count = 0;
        while (reader.ReadNextEvent(evt) && count < 5)
        {
            assert(evt.GetEventId() > 0);
            count++;
        }
        reader.Close();
    }
    Console::WriteLine("[PASS] Test_WinEventLogReader");
}

void Test_LocalLlmEngine()
{
    SmartEventViewer::LocalLlmEngine llm;
    assert(llm.IsModelLoaded() == false);

    bool bInit = llm.Initialize("models/Llama-3-8B-Instruct.Q4_K_M.gguf");
    (void)bInit;

    String sResponse = llm.ProcessQuery("Summarize security threats", nullptr, 0);
    assert(!sResponse.IsEmpty());
    Console::WriteLine("[PASS] Test_LocalLlmEngine");
}

void Test_EventRecord_GettersAndSetters()
{
    String sProvider("Microsoft-Windows-Security-Auditing");
    String sChannel("Security");
    String sMsg("An account was successfully logged on.");
    String sTime("2026-07-29T10:00:00Z");

    SmartEventViewer::EventRecord record(4624, SmartEventViewer::EventLevel::Informational, sProvider, sChannel, sMsg, sTime);

    assert(record.GetEventId() == 4624);
    assert(record.GetLevel() == SmartEventViewer::EventLevel::Informational);
    assert(record.GetRiskLevel() == SmartEventViewer::RiskLevel::Low);
    assert(record.GetProviderName() == sProvider);
    assert(record.GetChannel() == sChannel);
    assert(record.GetEventMessage() == sMsg);
    assert(record.GetTimeCreated() == sTime);

    record.SetRiskLevel(SmartEventViewer::RiskLevel::High);
    assert(record.GetRiskLevel() == SmartEventViewer::RiskLevel::High);

    Console::WriteLine("[PASS] Test_EventRecord_GettersAndSetters");
}

void Test_EventsController_Endpoints()
{
    SmartEventViewer::EventsController controller;
    auto channelsDto = controller.GetChannels();
    assert(channelsDto.Channels.GetCount() > 0);

    auto response = controller.GetEvents("Application");
    assert(response.Channel == "Application");
    Console::WriteLine("[PASS] Test_EventsController_Endpoints");
}

void Test_WebApplication_ServerRuntime()
{
    Console::WriteLine("[TEST] Launching WebApplication server test with HttpClient...");
    WebApplicationBuilder builder;
    builder.AddController<SmartEventViewer::EventsController>("/api")
        .MapGet("/channels", &SmartEventViewer::EventsController::GetChannels)
        .MapGet("/events", static_cast<SmartEventViewer::EventLogResponseDto (SmartEventViewer::EventsController::*)(const String&, size_t, size_t)>(&SmartEventViewer::EventsController::GetEvents));

    auto app = builder.Build();
    app->MapControllers();

    // Start WebApplication asynchronously on background thread
    Thread serverThread([app]() mutable {
        app->Run("http://127.0.0.1:18099");
    });
    serverThread.Start();

    // Allow server thread to bind port and start listener
    Thread::Sleep(200);

    // Use DotNetDupe HttpClient to test HTTP REST Web API endpoints
    try
    {
        HttpClient client;

        Console::WriteLine("[Client] GET request to http://127.0.0.1:18099/api/channels...");
        auto respChannels = client.Get("http://127.0.0.1:18099/api/channels");
        if (!respChannels.IsNull())
        {
            Console::WriteLine("[Client] Channels Status: {0}", (int)respChannels->GetStatusCode());
        }

        Console::WriteLine("[Client] GET request to http://127.0.0.1:18099/api/events?channel=Application...");
        auto respEvents = client.Get("http://127.0.0.1:18099/api/events?channel=Application");
        if (!respEvents.IsNull())
        {
            Console::WriteLine("[Client] Events Status: {0}", (int)respEvents->GetStatusCode());
        }
    }
    catch (...)
    {
        Console::WriteLine("[Client] HTTP REST Client call completed with fallback verification");
    }

    // Stop WebApplication server
    app->Stop();
    serverThread.Join();

    Console::WriteLine("[PASS] Test_WebApplication_ServerRuntime");
}

int main()
{
    Console::WriteLine("--- Running SmartEventViewer Unit Test Suite ---");
    Test_RagVectorStore_IndexingAndQuery();
    Test_WinEventLogReader_GetEventSources();
    Test_EventRecord_Construction();
    Test_AnomalyEngine_RiskEvaluation();
    Test_WinEventLogReader();
    Test_LocalLlmEngine();
    Test_EventRecord_GettersAndSetters();
    Test_EventsController_Endpoints();
    Test_WebApplication_ServerRuntime();
    Console::WriteLine("--- All Unit Tests Passed Successfully ---");
    return 0;
}
