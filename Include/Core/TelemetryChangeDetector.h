#pragma once

#include "ViewerCommon.h"
#include "System/SmartPointer.h"
#include "Dto/TelemetryDtos.h"
#include "System/Threading/CriticalSection.h"
#include "System/Threading/Lock.h"

namespace SmartEventViewer {
    using CriticalSection = DotNetDupe::System::Threading::CriticalSection;
    using LockCS = DotNetDupe::System::Threading::Lock<CriticalSection>;

    class SMARTEVENTVIEWER_API TelemetryChangeDetector {
    private:
        SystemMetricsResponseDto m_prevSummary{};
        SystemMetricsResponseDto m_prevProcesses{};
        SystemMetricsResponseDto m_prevSessions{};
        ServicesResponseDto m_prevServices{};
        bool m_bHasInitialSummary{ false };
        bool m_bHasInitialProcesses{ false };
        bool m_bHasInitialSessions{ false };
        bool m_bHasInitialServices{ false };
        mutable CriticalSection m_csLock{};

    public:
        TelemetryChangeDetector() = default;
        ~TelemetryChangeDetector() = default;

        bool HasSummaryChanged(const SystemMetricsResponseDto& current);
        bool HaveProcessesChanged(const SystemMetricsResponseDto& current);
        bool HaveSessionsChanged(const SystemMetricsResponseDto& current);
        bool HaveServicesChanged(const ServicesResponseDto& current);
        void Reset();
    };
}
