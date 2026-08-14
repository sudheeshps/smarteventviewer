#include "pch.h"
#include "Core/TelemetryController.h"
#include "Core/TelemetryCacheManager.h"
#include "System/Console.h"
#include "System/Exception.h"
#include "System/SystemException.h"

using Console = DotNetDupe::System::Console;
using BasicCharException = DotNetDupe::System::Exception;
using BasicCharSystemException = DotNetDupe::System::SystemException;

namespace SmartEventViewer {
    SystemMetricsResponseDto TelemetryController::GetSummary() {
        try {
            return TelemetryCacheManager::GetInstance().GetSummary();
        } catch (const BasicCharSystemException& sysEx) {
            Console::WriteLine(String::Format("[TELEMETRY_CTRL_ERROR] GetSummary DotNetDupe SystemException: {0}", sysEx.What()));
            return SystemMetricsResponseDto{};
        } catch (const BasicCharException& ex) {
            Console::WriteLine(String::Format("[TELEMETRY_CTRL_ERROR] GetSummary DotNetDupe Exception: {0}", ex.What()));
            return SystemMetricsResponseDto{};
        } catch (...) {
            Console::WriteLine("[TELEMETRY_CTRL_ERROR] GetSummary unknown exception.");
            return SystemMetricsResponseDto{};
        }
    }

    SystemMetricsResponseDto TelemetryController::GetCpuUsage() {
        try {
            return TelemetryCacheManager::GetInstance().GetCpuUsage();
        } catch (...) {
            return SystemMetricsResponseDto{};
        }
    }

    SystemMetricsResponseDto TelemetryController::GetMemoryUsage() {
        try {
            return TelemetryCacheManager::GetInstance().GetMemoryUsage();
        } catch (...) {
            return SystemMetricsResponseDto{};
        }
    }

    SystemMetricsResponseDto TelemetryController::GetDiskUsage() {
        try {
            return TelemetryCacheManager::GetInstance().GetDiskUsage();
        } catch (...) {
            return SystemMetricsResponseDto{};
        }
    }

    SystemMetricsResponseDto TelemetryController::GetNetworkUsage() {
        try {
            return TelemetryCacheManager::GetInstance().GetNetworkUsage();
        } catch (...) {
            return SystemMetricsResponseDto{};
        }
    }

    SystemMetricsResponseDto TelemetryController::GetProcesses() {
        try {
            return TelemetryCacheManager::GetInstance().GetProcesses();
        } catch (const BasicCharSystemException& sysEx) {
            Console::WriteLine(String::Format("[TELEMETRY_CTRL_ERROR] GetProcesses DotNetDupe SystemException: {0}", sysEx.What()));
            return SystemMetricsResponseDto{};
        } catch (const BasicCharException& ex) {
            Console::WriteLine(String::Format("[TELEMETRY_CTRL_ERROR] GetProcesses DotNetDupe Exception: {0}", ex.What()));
            return SystemMetricsResponseDto{};
        } catch (...) {
            Console::WriteLine("[TELEMETRY_CTRL_ERROR] GetProcesses unknown exception.");
            return SystemMetricsResponseDto{};
        }
    }

    SystemMetricsResponseDto TelemetryController::GetSessions() {
        try {
            return TelemetryCacheManager::GetInstance().GetSessions();
        } catch (const BasicCharSystemException& sysEx) {
            Console::WriteLine(String::Format("[TELEMETRY_CTRL_ERROR] GetSessions DotNetDupe SystemException: {0}", sysEx.What()));
            return SystemMetricsResponseDto{};
        } catch (const BasicCharException& ex) {
            Console::WriteLine(String::Format("[TELEMETRY_CTRL_ERROR] GetSessions DotNetDupe Exception: {0}", ex.What()));
            return SystemMetricsResponseDto{};
        } catch (...) {
            Console::WriteLine("[TELEMETRY_CTRL_ERROR] GetSessions unknown exception.");
            return SystemMetricsResponseDto{};
        }
    }

    ServicesResponseDto TelemetryController::GetServices() {
        try {
            return TelemetryCacheManager::GetInstance().GetServices();
        } catch (...) {
            return ServicesResponseDto{};
        }
    }

    SystemMetricsResponseDto TelemetryController::GetMetrics() {
        try {
            return TelemetryCacheManager::GetInstance().GetFullMetrics();
        } catch (const BasicCharSystemException& sysEx) {
            Console::WriteLine(String::Format("[TELEMETRY_CTRL_ERROR] GetMetrics DotNetDupe SystemException: {0}", sysEx.What()));
            return SystemMetricsResponseDto{};
        } catch (const BasicCharException& ex) {
            Console::WriteLine(String::Format("[TELEMETRY_CTRL_ERROR] GetMetrics DotNetDupe Exception: {0}", ex.What()));
            return SystemMetricsResponseDto{};
        } catch (...) {
            Console::WriteLine("[TELEMETRY_CTRL_ERROR] GetMetrics unknown exception.");
            return SystemMetricsResponseDto{};
        }
    }
}
