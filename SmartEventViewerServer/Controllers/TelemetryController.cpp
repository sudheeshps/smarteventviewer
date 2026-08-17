#include "TelemetryController.h"
#include "Core/TelemetryService.h"

namespace SmartEventViewer {
    TelemetryController::TelemetryController()
        : m_spTelemetryService(DotNetDupe::System::SmartPointer<TelemetryService>::NewShared()) {
    }

    TelemetryController::TelemetryController(const DotNetDupe::System::SmartPointer<ITelemetryService>& spService)
        : m_spTelemetryService(spService.IsNull() ? DotNetDupe::System::SmartPointer<ITelemetryService>(DotNetDupe::System::SmartPointer<TelemetryService>::NewShared()) : spService) {
    }

    SystemMetricsResponseDto TelemetryController::GetSummary() {
        return m_spTelemetryService->GetSummary();
    }

    SystemMetricsResponseDto TelemetryController::GetCpuUsage() {
        return m_spTelemetryService->GetCpuUsage();
    }

    SystemMetricsResponseDto TelemetryController::GetMemoryUsage() {
        return m_spTelemetryService->GetMemoryUsage();
    }

    SystemMetricsResponseDto TelemetryController::GetDiskUsage() {
        return m_spTelemetryService->GetDiskUsage();
    }

    SystemMetricsResponseDto TelemetryController::GetNetworkUsage() {
        return m_spTelemetryService->GetNetworkUsage();
    }

    SystemMetricsResponseDto TelemetryController::GetProcesses() {
        return m_spTelemetryService->GetProcesses();
    }

    SystemMetricsResponseDto TelemetryController::GetSessions() {
        return m_spTelemetryService->GetSessions();
    }

    ServicesResponseDto TelemetryController::GetServices() {
        return m_spTelemetryService->GetServices();
    }

    SystemMetricsResponseDto TelemetryController::GetMetrics() {
        return m_spTelemetryService->GetFullMetrics();
    }
}
