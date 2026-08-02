#include <csignal>
#include <cstdlib>
#include <thread>
#include <chrono>

#include "System/Console.h"
#include "System/Convert.h"
#include "System/Path.h"
#include "System/IO/File.h"
#include "WebAppCore/Builder/WebApplication.h"
#include "WebAppCore/Builder/WebApplicationBuilder.h"
#include "WebAppCore/Controllers/ControllerBase.h"
#include "WebAppCore/Controllers/ControllerRouteBuilder.h"
#include "WebAppCore/Server/WebAppServer.h"
#include "EventsController.h"

using namespace DotNetDupe::System;
using namespace DotNetDupe::System::IO;
using namespace DotNetDupe::WebAppCore::Builder;
using namespace DotNetDupe::WebAppCore::Http;
using namespace DotNetDupe::WebAppCore::Controllers;
using namespace DotNetDupe::WebAppCore::Server;

static SmartPointer<WebApplication> g_app = nullptr;
static SmartPointer<WebAppServer> g_webServer = nullptr;

void SignalHandler(int signal)
{
    (void)signal;
    Console::WriteLine("\n[SERVER] Shutdown signal (SIGINT/SIGTERM) received. Stopping WebApplication...");
    if (!g_webServer.IsNull())
    {
        g_webServer->Stop();
    }
    else if (!g_app.IsNull())
    {
        g_app->Stop();
    }
    Console::WriteLine("[SERVER] Server process stopped cleanly.");
    exit(0);
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    unsigned short uPort = 8080;

    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    Console::WriteLine("==========================================================");
    Console::WriteLine("  SmartEventViewer SIEM Server (DotNetDupe Framework)");
    Console::WriteLine("==========================================================");
    Console::WriteLine("[SERVER] Initializing DotNetDupe WebApplicationBuilder...");

    // 1. Create Builder & Register Controllers
    WebApplicationBuilder builder;

    Console::WriteLine("[SERVER] Registering Controller Routes: /api/channels, /api/events, /api/metrics, /api/analyze, /api/analyze/status");
    builder.AddController<SmartEventViewer::EventsController>("/api")
        .MapGet("/channels", &SmartEventViewer::EventsController::GetChannels)
        .MapGet("/events", static_cast<SmartEventViewer::EventLogResponseDto (SmartEventViewer::EventsController::*)(const String&, size_t, size_t)>(&SmartEventViewer::EventsController::GetEvents))
        .MapGet("/metrics", &SmartEventViewer::EventsController::GetMetrics)
        .MapPost("/analyze", &SmartEventViewer::EventsController::AnalyzeEvents)
        .MapGet("/analyze/status", &SmartEventViewer::EventsController::GetAnalyzeStatus);

    // 2. Build the WebApplication
    Console::WriteLine("[SERVER] Building WebApplication pipeline...");
    g_app = builder.Build();

    // 3. Setup Web API Controllers & Minimal API Endpoints
    Console::WriteLine("[SERVER] Mapping Minimal API Endpoints...");

    // Dynamic Route Parameter Minimal API Endpoint
    g_app->MapGet("/api/events/{channelName}", [](SmartPointer<HttpContext> context) -> String {
        String channelName;
        if (context->GetRequest()->GetRouteValues().TryGetValue("channelName", channelName)) {
            Console::WriteLine("[SERVER] HTTP GET /api/events/{0} -> Target Channel: {0}", channelName);
            SmartEventViewer::EventsController controller;
            auto eventsDto = controller.GetEvents(channelName);
            return String::Format("{{\"channel\":\"{0}\",\"totalCount\":{1}}}", eventsDto.Channel, eventsDto.TotalCount);
        }
        return "{\"error\":\"Channel parameter missing in route path\"}";
    });

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

    // 5. Start the server via DotNetDupe WebAppServer framework
    Console::WriteLine("[SERVER] Starting WebAppServer static + WebAPI listener on port 8080");
    Console::WriteLine("[SERVER] Server is actively running at http://127.0.0.1:8080/");
    Console::WriteLine("[SERVER] Press Ctrl+C to stop.\n");

    g_webServer->Run("http://127.0.0.1:8080");

    Console::Read();
    return 0;
}
