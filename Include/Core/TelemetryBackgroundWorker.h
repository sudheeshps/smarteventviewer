#pragma once

#include "ViewerCommon.h"
#include "System/SmartPointer.h"
#include "System/Threading/Thread.h"
#include "Core/ITelemetryService.h"

namespace SmartEventViewer {
    class SMARTEVENTVIEWER_API TelemetryBackgroundWorker {
    private:
        static bool s_bRunning;
        static DotNetDupe::System::SmartPointer<DotNetDupe::System::Threading::Thread> s_spWorkerThread;
        static DotNetDupe::System::SmartPointer<ITelemetryService> s_spTelemetryService;

        static void WorkerThreadProc();

    public:
        static void Start(const DotNetDupe::System::SmartPointer<ITelemetryService>& spService = nullptr);
        static void Stop();
        static bool IsRunning();
        static void Tick();
    };
}
