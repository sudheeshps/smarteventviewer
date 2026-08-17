#pragma once

#include "ViewerCommon.h"
#include "Core/ISystemTelemetryProvider.h"

namespace SmartEventViewer {
    class SMARTEVENTVIEWER_API WindowsSystemTelemetryProvider : public ISystemTelemetryProvider {
    public:
        WindowsSystemTelemetryProvider() = default;
        ~WindowsSystemTelemetryProvider() override = default;

        SystemMetricsResponseDto QuerySummary() override;
        SystemMetricsResponseDto QueryCpuUsage() override;
        SystemMetricsResponseDto QueryMemoryUsage() override;
        SystemMetricsResponseDto QueryDiskUsage() override;
        SystemMetricsResponseDto QueryNetworkUsage() override;
        SystemMetricsResponseDto QueryProcesses() override;
        SystemMetricsResponseDto QuerySessions() override;
        ServicesResponseDto QueryServices() override;
        SystemMetricsResponseDto QuerySystemMetrics() override;
    };
}
