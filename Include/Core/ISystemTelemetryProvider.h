#pragma once

#include "ViewerCommon.h"
#include "System/Object.h"
#include "Dto/TelemetryDtos.h"

namespace SmartEventViewer {
    class ISystemTelemetryProvider : public virtual DotNetDupe::System::Object {
    public:
        virtual ~ISystemTelemetryProvider() = default;

        virtual SystemMetricsResponseDto QuerySummary() = 0;
        virtual SystemMetricsResponseDto QueryCpuUsage() = 0;
        virtual SystemMetricsResponseDto QueryMemoryUsage() = 0;
        virtual SystemMetricsResponseDto QueryDiskUsage() = 0;
        virtual SystemMetricsResponseDto QueryNetworkUsage() = 0;
        virtual SystemMetricsResponseDto QueryProcesses() = 0;
        virtual SystemMetricsResponseDto QuerySessions() = 0;
        virtual ServicesResponseDto QueryServices() = 0;
        virtual SystemMetricsResponseDto QuerySystemMetrics() = 0;
    };
}
