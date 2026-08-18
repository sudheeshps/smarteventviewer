#pragma once

#include "ViewerCommon.h"
#include "System/Object.h"
#include "Dto/TelemetryDtos.h"

namespace SmartEventViewer {
    class ITelemetryService : public virtual DotNetDupe::System::Object {
    public:
        virtual ~ITelemetryService() = default;

        virtual SystemMetricsResponseDto GetSummary() = 0;
        virtual SystemMetricsResponseDto GetCpuUsage() = 0;
        virtual SystemMetricsResponseDto GetMemoryUsage() = 0;
        virtual SystemMetricsResponseDto GetDiskUsage() = 0;
        virtual SystemMetricsResponseDto GetNetworkUsage() = 0;
        virtual SystemMetricsResponseDto GetProcesses() = 0;
        virtual SystemMetricsResponseDto GetSessions() = 0;
        virtual ServicesResponseDto GetServices() = 0;
        virtual SystemMetricsResponseDto GetFullMetrics() = 0;
        virtual TelemetryPostureReportDto GetPostureReport() = 0;
        virtual void SampleAndDetectChanges() = 0;
        virtual void ClearCache() = 0;
    };
}
