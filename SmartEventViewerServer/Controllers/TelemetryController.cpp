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
        try {
            return m_spTelemetryService->GetSummary();
        } catch (...) {
            return SystemMetricsResponseDto{};
        }
    }

    SystemMetricsResponseDto TelemetryController::GetCpuUsage() {
        try {
            return m_spTelemetryService->GetCpuUsage();
        } catch (...) {
            return SystemMetricsResponseDto{};
        }
    }

    SystemMetricsResponseDto TelemetryController::GetMemoryUsage() {
        try {
            return m_spTelemetryService->GetMemoryUsage();
        } catch (...) {
            return SystemMetricsResponseDto{};
        }
    }

    SystemMetricsResponseDto TelemetryController::GetDiskUsage() {
        try {
            return m_spTelemetryService->GetDiskUsage();
        } catch (...) {
            return SystemMetricsResponseDto{};
        }
    }

    SystemMetricsResponseDto TelemetryController::GetNetworkUsage() {
        try {
            return m_spTelemetryService->GetNetworkUsage();
        } catch (...) {
            return SystemMetricsResponseDto{};
        }
    }

    SystemMetricsResponseDto TelemetryController::GetProcesses() {
        try {
            return m_spTelemetryService->GetProcesses();
        } catch (...) {
            return SystemMetricsResponseDto{};
        }
    }

    SystemMetricsResponseDto TelemetryController::GetSessions() {
        try {
            return m_spTelemetryService->GetSessions();
        } catch (...) {
            return SystemMetricsResponseDto{};
        }
    }

    ServicesResponseDto TelemetryController::GetServices() {
        try {
            return m_spTelemetryService->GetServices();
        } catch (...) {
            return ServicesResponseDto{};
        }
    }

    SystemMetricsResponseDto TelemetryController::GetMetrics() {
        try {
            return m_spTelemetryService->GetFullMetrics();
        } catch (...) {
            return SystemMetricsResponseDto{};
        }
    }
}
