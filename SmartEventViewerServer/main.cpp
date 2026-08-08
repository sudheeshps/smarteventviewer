#include <csignal>
#include <cstdlib>
#include <thread>
#include <chrono>

#include "System/Console.h"
#include "System/Convert.h"
#include "System/Path.h"
#include "System/IO/File.h"
#include "System/IO/TextWriter.h"
#include "System/IO/StringWriter.h"
#include "WebAppCore/Builder/WebApplication.h"
#include "WebAppCore/Builder/WebApplicationBuilder.h"
#include "WebAppCore/Controllers/ControllerBase.h"
#include "WebAppCore/Controllers/ControllerRouteBuilder.h"
#include "WebAppCore/Server/WebAppServer.h"
#include "Extensions/Logging/LogManager.h"
#include "Extensions/Logging/FileLoggerProvider.h"
#include "Extensions/Logging/ConsoleLoggerProvider.h"
#include "Extensions/Logging/LoggerConfiguration.h"
#include "Extensions/Logging/LoggerTextWriter.h"
#include "Core/EventsController.h"
#include "Core/TelemetryController.h"
#include "Core/LlmAnalysisController.h"
#include "Core/DiagnosticsController.h"

#include "Core/TelemetryWebSocketHandler.h"
#include "Core/TelemetryBackgroundWorker.h"

using namespace DotNetDupe::System;
using namespace DotNetDupe::System::IO;
using namespace DotNetDupe::Extensions::Logging;
using namespace DotNetDupe::WebAppCore::Builder;
using namespace DotNetDupe::WebAppCore::Http;
using namespace DotNetDupe::WebAppCore::Controllers;
using namespace DotNetDupe::WebAppCore::Server;

SmartPointer<WebApplication> g_app = nullptr;
SmartPointer<WebAppServer> g_webServer = nullptr;

void SignalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        Console::WriteLine("\n[SERVER] Shutdown signal received. Stopping WebAppServer...");
        SmartEventViewer::TelemetryBackgroundWorker::Stop();
        if (!g_webServer.IsNull())
        {
            g_webServer->Stop();
        }
        Console::WriteLine("[SERVER] Server gracefully stopped.");
        exit(0);
    }
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    try
    {
        Console::WriteLine("=================================================");
        Console::WriteLine("   SmartEventViewer Web Server Native Host (x64) ");
        Console::WriteLine("=================================================");
        Console::WriteLine("[SERVER] Initializing DotNetDupe WebApplication pipeline...");

        WebApplicationBuilder builder;

        LoggerConfiguration logConfig;
        logConfig.FilePath = "logs/SmartEventViewerServer.log";
        logConfig.MinLevel = LogLevel::Information;
        logConfig.IsJsonFormat = true;
        logConfig.Rollover.EnableRollover = true;
        logConfig.Rollover.MaxFileSizeInBytes = 5 * 1024 * 1024;
        logConfig.Rollover.MaxBackupFiles = 5;

        LogManager::Configure(logConfig);
        
        Console::SetOut(SmartPointer<LoggerTextWriter>::NewShared("Console"));

        Console::WriteLine("[SERVER] LogManager configured with JSON file logging & console redirection.");

        builder.GetServices().AddTransient<SmartEventViewer::EventsController, SmartEventViewer::EventsController>();
        builder.GetServices().AddTransient<SmartEventViewer::TelemetryController, SmartEventViewer::TelemetryController>();
        builder.GetServices().AddTransient<SmartEventViewer::LlmAnalysisController, SmartEventViewer::LlmAnalysisController>();
        builder.GetServices().AddTransient<SmartEventViewer::DiagnosticsController, SmartEventViewer::DiagnosticsController>();

        Console::WriteLine("[SERVER] Registering Domain Controllers: EventsController, TelemetryController, LlmAnalysisController, DiagnosticsController");
        
        builder.AddController<SmartEventViewer::EventsController>("/api")
            .MapGet("/channels", &SmartEventViewer::EventsController::GetChannels)
            .MapGet("/events/summary", static_cast<SmartEventViewer::EventSummaryResponseDto (SmartEventViewer::EventsController::*)()>(&SmartEventViewer::EventsController::GetEventSummary))
            .MapGet("/events", static_cast<SmartEventViewer::EventLogResponseDto (SmartEventViewer::EventsController::*)(const String&, size_t, size_t)>(&SmartEventViewer::EventsController::GetEvents));

        builder.AddController<SmartEventViewer::TelemetryController>("/api")
            .MapGet("/metrics/summary", &SmartEventViewer::TelemetryController::GetSummary)
            .MapGet("/metrics/cpu", &SmartEventViewer::TelemetryController::GetCpuUsage)
            .MapGet("/metrics/memory", &SmartEventViewer::TelemetryController::GetMemoryUsage)
            .MapGet("/metrics/disk", &SmartEventViewer::TelemetryController::GetDiskUsage)
            .MapGet("/metrics/network", &SmartEventViewer::TelemetryController::GetNetworkUsage)
            .MapGet("/metrics/processes", &SmartEventViewer::TelemetryController::GetProcesses)
            .MapGet("/metrics/sessions", &SmartEventViewer::TelemetryController::GetSessions)
            .MapGet("/metrics/services", &SmartEventViewer::TelemetryController::GetServices);

        builder.AddController<SmartEventViewer::DiagnosticsController>("/api")
            .MapGet("/logs/format", &SmartEventViewer::DiagnosticsController::GetLogFormat)
            .MapGet("/logs", &SmartEventViewer::DiagnosticsController::GetServerLogs);

        builder.AddController<SmartEventViewer::LlmAnalysisController>("/api")
            .MapPost("/analyze", &SmartEventViewer::LlmAnalysisController::AnalyzeEvents)
            .MapGet("/analyze/status", &SmartEventViewer::LlmAnalysisController::GetAnalyzeStatus);

        // 2. Build the WebApplication
        Console::WriteLine("[SERVER] Building WebApplication pipeline...");
        g_app = builder.Build();

        // 3. Map WebSocket for Telemetry Push
        g_app->MapWebSocket("/ws/telemetry", SmartEventViewer::TelemetryWebSocketHandler::GetInstance());
        Console::WriteLine("[SERVER] Mapped WebSocket endpoint: /ws/telemetry");

        // 3c. Setup Web API Controllers
        Console::WriteLine("[SERVER] Finalizing Web API Controllers mapping...");
        g_app->MapControllers();

        // 4. Configure WebAppServer for static file serving
        String sWebRoot = Path::Combine({ Path::GetFullPath("."), "UI" });
        if (!File::Exists(Path::Combine({ sWebRoot, "index.html" })))
        {
            sWebRoot = "SmartEventViewerApp";
        }

        Console::WriteLine("[SERVER] Configuring WebAppServer with web root: {0}", sWebRoot);
        g_webServer = SmartPointer<WebAppServer>::New(g_app, sWebRoot);
        g_webServer->EnableStaticFiles("index.html");

        // 5. Start Telemetry Background Worker Thread
        SmartEventViewer::TelemetryBackgroundWorker::Start();

        // 6. Start the server via DotNetDupe WebAppServer framework
        Console::WriteLine("[SERVER] Starting WebAppServer static + WebAPI listener on port 8080");
        Console::WriteLine("[SERVER] Server is actively running at http://127.0.0.1:8080/");
        Console::WriteLine("[SERVER] Press Ctrl+C to stop.\n");

        g_webServer->Run("http://127.0.0.1:8080");

        Console::Read();
    }
    catch (const BasicException<char>& ex)
    {
        Console::WriteLine("\n[SERVER_CRASH_FATAL] DotNetDupe Exception Caught: {0}", ex.What());
        SmartEventViewer::EventsController::Log(String::Format("[FATAL_EXCEPTION] {0}", ex.What()));
    }
    catch (const std::exception& ex)
    {
        Console::WriteLine("\n[SERVER_CRASH_FATAL] Standard C++ Exception Caught: {0}", ex.what());
        SmartEventViewer::EventsController::Log(String::Format("[FATAL_EXCEPTION] {0}", ex.what()));
    }
    catch (...)
    {
        Console::WriteLine("\n[SERVER_CRASH_FATAL] Unknown Unhandled Exception Caught in main().");
        SmartEventViewer::EventsController::Log(String("[FATAL_EXCEPTION] Unknown Unhandled Exception in main()"));
    }

    return 0;
}
