#pragma once

#include "ViewerCommon.h"
#include "System/SmartPointer.h"
#include "System/Threading/Thread.h"

namespace SmartEventViewer
{
    class SMARTEVENTVIEWER_API TelemetryBackgroundWorker
    {
    private:
        static bool s_bRunning;
        static DotNetDupe::System::SmartPointer<DotNetDupe::System::Threading::Thread> s_spWorkerThread;

        static void WorkerThreadProc();

        TelemetryBackgroundWorker() = default;
        ~TelemetryBackgroundWorker() = default;

    public:
        static void Start();
        static void Stop();
        static bool IsRunning();
    };
}
