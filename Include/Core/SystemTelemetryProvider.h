#pragma once

#include "ViewerCommon.h"
#include "System/Diagnostics/SystemMetrics.h"
#include "System/String.h"
#include "Dto/TelemetryDtos.h"

namespace DotNetDupe {
    namespace System {
        namespace Diagnostics {
            struct RealTimeSystemInfo;
            struct ProcessInfo;
        }
    }
}

namespace SmartEventViewer {
    class SystemTelemetryProvider {
    public:
        SystemTelemetryProvider() = default;
        ~SystemTelemetryProvider() = default;

        SMARTEVENTVIEWER_API static SystemMetricsResponseDto QuerySystemMetrics();
        SMARTEVENTVIEWER_API static SystemMetricsResponseDto QuerySummary();
        SMARTEVENTVIEWER_API static SystemMetricsResponseDto QueryCpuUsage();
        SMARTEVENTVIEWER_API static SystemMetricsResponseDto QueryMemoryUsage();
        SMARTEVENTVIEWER_API static SystemMetricsResponseDto QueryDiskUsage();
        SMARTEVENTVIEWER_API static SystemMetricsResponseDto QueryNetworkUsage();
        SMARTEVENTVIEWER_API static SystemMetricsResponseDto QueryProcesses();
        SMARTEVENTVIEWER_API static SystemMetricsResponseDto QuerySessions();
        SMARTEVENTVIEWER_API static ServicesResponseDto QueryServices();

        // Helper methods for telemetry and process resource mapping
        SMARTEVENTVIEWER_API static void CalculateDiskRates(const DotNetDupe::System::Diagnostics::DiskInfo& diskInfo, double& outReadMb, double& outWriteMb);
        SMARTEVENTVIEWER_API static DotNetDupe::System::String FormatCommandLine(const DotNetDupe::System::String& sPath, const DotNetDupe::System::String& sCmd);
        SMARTEVENTVIEWER_API static ProcessResourceDto MapProcessResourceDto(const DotNetDupe::System::Diagnostics::ProcessInfo& proc);
        SMARTEVENTVIEWER_API static void PopulateUserSessions(SystemMetricsResponseDto& metrics);
    };
}
