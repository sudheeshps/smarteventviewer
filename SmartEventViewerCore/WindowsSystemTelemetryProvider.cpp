#include "pch.h"
#include "Platform/WindowsSystemTelemetryProvider.h"
#include "Core/SystemTelemetryProvider.h"

namespace SmartEventViewer {
    SystemMetricsResponseDto WindowsSystemTelemetryProvider::QuerySummary() {
        return SystemTelemetryProvider::QuerySummary();
    }

    SystemMetricsResponseDto WindowsSystemTelemetryProvider::QueryCpuUsage() {
        return SystemTelemetryProvider::QueryCpuUsage();
    }

    SystemMetricsResponseDto WindowsSystemTelemetryProvider::QueryMemoryUsage() {
        return SystemTelemetryProvider::QueryMemoryUsage();
    }

    SystemMetricsResponseDto WindowsSystemTelemetryProvider::QueryDiskUsage() {
        return SystemTelemetryProvider::QueryDiskUsage();
    }

    SystemMetricsResponseDto WindowsSystemTelemetryProvider::QueryNetworkUsage() {
        return SystemTelemetryProvider::QueryNetworkUsage();
    }

    SystemMetricsResponseDto WindowsSystemTelemetryProvider::QueryProcesses() {
        return SystemTelemetryProvider::QueryProcesses();
    }

    SystemMetricsResponseDto WindowsSystemTelemetryProvider::QuerySessions() {
        return SystemTelemetryProvider::QuerySessions();
    }

    ServicesResponseDto WindowsSystemTelemetryProvider::QueryServices() {
        return SystemTelemetryProvider::QueryServices();
    }

    SystemMetricsResponseDto WindowsSystemTelemetryProvider::QuerySystemMetrics() {
        return SystemTelemetryProvider::QuerySystemMetrics();
    }
}
