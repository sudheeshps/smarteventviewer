#include <iostream>
#include <csignal>
#include <cstdlib>
#include <thread>
#include <chrono>
#include "Core/NativeRestServer.h"

static SmartEventViewer::WebApplication g_app;

void SignalHandler(int signal)
{
    (void)signal;
    std::cout << "\n[INFO] Shutdown signal received. Stopping SmartEventViewer WebApplication...\n";
    g_app.Stop();
    exit(0);
}

int main(int argc, char* argv[])
{
    unsigned short uPort = 8080;
    if (argc > 1)
    {
        uPort = static_cast<unsigned short>(std::atoi(argv[1]));
    }

    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    std::cout << "===================================================\n";
    std::cout << "  SmartEventViewer DotNetDupe WebApplication Server\n";
    std::cout << "===================================================\n";
    std::cout << "[INFO] Initializing DotNetDupe WebApplicationBuilder...\n";

    auto builder = SmartEventViewer::WebApplication::CreateBuilder(argc, argv);
    builder.UseUrls("http://localhost:8080");
    builder.UseStaticFiles("UI");
    g_app = builder.Build();

    std::cout << "[INFO] Registering Controller Routes & Static React UI Files:\n";
    std::cout << "  -> GET /               (Serves React SPA Frontend index.html)\n";
    std::cout << "  -> GET /api/channels   (Enumerates Win32 EvtQuery Event Sources)\n";
    std::cout << "  -> GET /api/events     (Streams live kernel EventRecord payloads)\n";

    if (g_app.Run(uPort))
    {
        std::cout << "[SUCCESS] DotNetDupe WebApplication is listening at http://localhost:" << uPort << "/\n";
        std::cout << "[INFO] Press Ctrl+C to stop the WebApplication.\n\n";

        while (g_app.IsRunning())
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    else
    {
        std::cerr << "[ERROR] Failed to run DotNetDupe WebApplication on port " << uPort << "\n";
        return 1;
    }

    return 0;
}
