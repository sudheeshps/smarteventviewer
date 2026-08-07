#pragma once
#include "ViewerCommon.h"
#include "System/Diagnostics/SystemMetrics.h"
#include "System/String.h"

namespace DotNetDupe {
    namespace System {
        namespace Diagnostics {
            struct RealTimeSystemInfo;
            struct ProcessResourceInfo;
        }
    }
}

namespace SmartEventViewer
{
    struct SystemMetricsResponseDto;
    struct ProcessResourceDto;

    class SystemTelemetryProvider
    {
    public:
        SystemTelemetryProvider() = default;
        ~SystemTelemetryProvider() = default;

        SMARTEVENTVIEWER_API static SystemMetricsResponseDto QuerySystemMetrics();
        SMARTEVENTVIEWER_API static SystemMetricsResponseDto QuerySummary();
        SMARTEVENTVIEWER_API static SystemMetricsResponseDto QueryProcesses();
        SMARTEVENTVIEWER_API static SystemMetricsResponseDto QuerySessions();

        // Helper methods for telemetry and process resource mapping
        SMARTEVENTVIEWER_API static void CalculateDiskRates(const DotNetDupe::System::Diagnostics::RealTimeSystemInfo& realInfo, double& outReadMb, double& outWriteMb);
        SMARTEVENTVIEWER_API static DotNetDupe::System::String FormatCommandLine(const DotNetDupe::System::String& sPath, const DotNetDupe::System::String& sCmd);
        SMARTEVENTVIEWER_API static ProcessResourceDto MapProcessResourceDto(const DotNetDupe::System::Diagnostics::ProcessResourceInfo& proc);
        SMARTEVENTVIEWER_API static void PopulateUserSessions(SystemMetricsResponseDto& metrics);
    };
}
