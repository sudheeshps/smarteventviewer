#pragma once

#include "WebAppCore/Controllers/ControllerBase.h"
#include "System/SmartPointer.h"
#include "Dto/TelemetryDtos.h"
#include "Core/ITelemetryService.h"

namespace SmartEventViewer {
    class TelemetryController : public DotNetDupe::WebAppCore::Controllers::ControllerBase {
    private:
        DotNetDupe::System::SmartPointer<ITelemetryService> m_spTelemetryService{ nullptr };

    public:
        TelemetryController();
        explicit TelemetryController(const DotNetDupe::System::SmartPointer<ITelemetryService>& spService);
        ~TelemetryController() override = default;

        SystemMetricsResponseDto GetSummary();
        SystemMetricsResponseDto GetCpuUsage();
        SystemMetricsResponseDto GetMemoryUsage();
        SystemMetricsResponseDto GetDiskUsage();
        SystemMetricsResponseDto GetNetworkUsage();
        SystemMetricsResponseDto GetProcesses();
        SystemMetricsResponseDto GetSessions();
        ServicesResponseDto GetServices();
        SystemMetricsResponseDto GetMetrics();
    };
}
