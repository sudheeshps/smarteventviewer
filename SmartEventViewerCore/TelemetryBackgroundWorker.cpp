#include "pch.h"
#include "Core/TelemetryBackgroundWorker.h"
#include "Core/TelemetryService.h"
#include "System/Console.h"
#include "System/Threading/Thread.h"

using Console = DotNetDupe::System::Console;
using Thread = DotNetDupe::System::Threading::Thread;
using ThreadStart = DotNetDupe::System::Threading::ThreadStart;

namespace SmartEventViewer {
    bool TelemetryBackgroundWorker::s_bRunning = false;
    DotNetDupe::System::SmartPointer<Thread> TelemetryBackgroundWorker::s_spWorkerThread = nullptr;
    DotNetDupe::System::SmartPointer<ITelemetryService> TelemetryBackgroundWorker::s_spTelemetryService = nullptr;

    void TelemetryBackgroundWorker::Start(const DotNetDupe::System::SmartPointer<ITelemetryService>& spService) {
        if (s_bRunning) return;
        s_bRunning = true;
        s_spTelemetryService = spService.IsNull() 
            ? TelemetryService::GetDefault() 
            : spService;

        s_spWorkerThread = DotNetDupe::System::SmartPointer<Thread>::NewShared(ThreadStart([]() {
            WorkerThreadProc();
        }));
        s_spWorkerThread->Start();
        Console::WriteLine("[TELEMETRY_WORKER] Telemetry worker thread started.");
    }

    void TelemetryBackgroundWorker::Stop() {
        if (!s_bRunning) return;
        s_bRunning = false;
        if (!s_spWorkerThread.IsNull()) {
            s_spWorkerThread->Join(2000);
            s_spWorkerThread = nullptr;
        }
        s_spTelemetryService = nullptr;
        Console::WriteLine("[TELEMETRY_WORKER] Telemetry worker thread stopped.");
    }

    bool TelemetryBackgroundWorker::IsRunning() {
        return s_bRunning;
    }

    void TelemetryBackgroundWorker::Tick() {
        if (!s_spTelemetryService.IsNull()) {
            try {
                s_spTelemetryService->SampleAndDetectChanges();
            } catch (const DotNetDupe::System::Exception& ex) {
                Console::WriteLine(String::Format("[TELEMETRY_WORKER_ERROR] Sampling error: {0}", ex.What()));
            } catch (...) {
                Console::WriteLine("[TELEMETRY_WORKER_ERROR] Unknown sampling error.");
            }
        }
    }

    void TelemetryBackgroundWorker::WorkerThreadProc() {
        while (s_bRunning) {
            Thread::Sleep(1000);
            if (!s_bRunning) break;
            Tick();
        }
    }
}
