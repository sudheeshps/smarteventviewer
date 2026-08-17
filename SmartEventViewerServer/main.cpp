#include <csignal>
#include <cstdlib>
#include <thread>
#include <chrono>

#include "System/Console.h"
#include "System/Convert.h"
#include "System/IO/Path.h"
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

#include "Controllers/EventsController.h"
#include "Controllers/TelemetryController.h"
#include "Controllers/LlmAnalysisController.h"
#include "Controllers/DiagnosticsController.h"
#include "WebSockets/TelemetryWebSocketHandler.h"

#include "Core/IEventService.h"
#include "Core/EventService.h"
#include "Core/ITelemetryService.h"
#include "Core/TelemetryService.h"
#include "Core/IAnalysisService.h"
#include "Core/AnalysisService.h"
#include "Core/IDiagnosticsService.h"
#include "Core/DiagnosticsService.h"
#include "Core/IEventLogReader.h"
#include "Platform/WindowsEtwLogReader.h"
#include "Core/ISystemTelemetryProvider.h"
#include "Platform/WindowsSystemTelemetryProvider.h"
#include "Core/IAnomalyEngine.h"
#include "Core/AnomalyEngine.h"
#include "Core/TelemetryBackgroundWorker.h"
#include "Logging/AppLoggerManager.h"

using namespace DotNetDupe::System;
using namespace DotNetDupe::System::IO;
using namespace DotNetDupe::Extensions::Logging;
using namespace DotNetDupe::WebAppCore::Builder;
using namespace DotNetDupe::WebAppCore::Http;
using namespace DotNetDupe::WebAppCore::Controllers;
using namespace DotNetDupe::WebAppCore::Server;

SmartPointer<WebApplication> g_app = nullptr;
SmartPointer<WebAppServer> g_webServer = nullptr;

void SignalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        Console::WriteLine("\n[SERVER] Shutdown signal received. Stopping WebAppServer...");
        SmartEventViewer::TelemetryBackgroundWorker::Stop();
        if (!g_webServer.IsNull()) {
            g_webServer->Stop();
        }
        Console::WriteLine("[SERVER] Server gracefully stopped.");
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    try {
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

        // Register Providers, Engines and Push Notifier
        builder.GetServices().AddSingleton<SmartEventViewer::ITelemetryPushNotifier>(SmartEventViewer::TelemetryWebSocketHandler::GetInstance());
        builder.GetServices().AddSingleton<SmartEventViewer::IEventLogReader, SmartEventViewer::WindowsEtwLogReader>();
        builder.GetServices().AddSingleton<SmartEventViewer::ISystemTelemetryProvider, SmartEventViewer::WindowsSystemTelemetryProvider>();
        builder.GetServices().AddSingleton<SmartEventViewer::IAnomalyEngine, SmartEventViewer::AnomalyEngine>();
        builder.GetServices().AddSingleton<SmartEventViewer::LocalLlmEngine, SmartEventViewer::LocalLlmEngine>();

        // Register Domain Services
        builder.GetServices().AddSingleton<SmartEventViewer::IEventService, SmartEventViewer::EventService>();
        builder.GetServices().AddSingleton<SmartEventViewer::ITelemetryService, SmartEventViewer::TelemetryService>();
        builder.GetServices().AddSingleton<SmartEventViewer::IAnalysisService>(SmartEventViewer::AnalysisService::GetSharedInstance());
        builder.GetServices().AddSingleton<SmartEventViewer::IDiagnosticsService, SmartEventViewer::DiagnosticsService>();

        SmartEventViewer::AnalysisService::GetSharedInstance()->SetPushNotifier(SmartEventViewer::TelemetryWebSocketHandler::GetInstance());

        // Register Web API Controllers
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
            .MapGet("/analyze/status", static_cast<SmartEventViewer::AnalyzeResponseDto (SmartEventViewer::LlmAnalysisController::*)(const DotNetDupe::System::String&)>(&SmartEventViewer::LlmAnalysisController::GetAnalyzeStatus));

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
        if (!File::Exists(Path::Combine({ sWebRoot, "index.html" }))) {
            sWebRoot = "SmartEventViewerApp";
        }

        Console::WriteLine("[SERVER] Configuring WebAppServer with web root: {0}", sWebRoot);
        g_webServer = SmartPointer<WebAppServer>::New(g_app, sWebRoot);
        g_webServer->EnableStaticFiles("index.html");

        // 5. Start Telemetry Background Worker Thread
        auto spTelemetryService = SmartPointer<SmartEventViewer::ITelemetryService>(SmartPointer<SmartEventViewer::TelemetryService>::NewShared(
            SmartPointer<SmartEventViewer::ISystemTelemetryProvider>(SmartPointer<SmartEventViewer::WindowsSystemTelemetryProvider>::NewShared()),
            SmartPointer<SmartEventViewer::ITelemetryPushNotifier>(SmartEventViewer::TelemetryWebSocketHandler::GetInstance())
        ));
        SmartEventViewer::TelemetryBackgroundWorker::Start(spTelemetryService);

        // 6. Start the server via DotNetDupe WebAppServer framework
        Console::WriteLine("[SERVER] Starting WebAppServer static + WebAPI listener on port 8080");
        Console::WriteLine("[SERVER] Server is actively running at http://127.0.0.1:8080/");
        Console::WriteLine("[SERVER] Press Ctrl+C to stop.\n");

        g_webServer->Run("http://127.0.0.1:8080");

        Console::Read();
    }
    catch (const DotNetDupe::System::Exception& ex) {
        Console::WriteLine("\n[SERVER_CRASH_FATAL] DotNetDupe Exception Caught: {0}", ex.What());
        SmartEventViewer::AppLoggerManager::Error("SERVER", String::Format("[FATAL_EXCEPTION] {0}", ex.What()));
    }
    catch (const std::exception& ex) {
        Console::WriteLine("\n[SERVER_CRASH_FATAL] Standard C++ Exception Caught: {0}", ex.what());
        SmartEventViewer::AppLoggerManager::Error("SERVER", String::Format("[FATAL_EXCEPTION] {0}", ex.what()));
    }
    catch (...) {
        Console::WriteLine("\n[SERVER_CRASH_FATAL] Unknown Unhandled Exception Caught in main().");
        SmartEventViewer::AppLoggerManager::Error("SERVER", "[FATAL_EXCEPTION] Unknown Unhandled Exception in main()");
    }

    return 0;
}
