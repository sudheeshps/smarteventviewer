#include "pch.h"
#include "TelemetryController.h"
#include "SystemTelemetryProvider.h"

namespace SmartEventViewer
{
    SystemMetricsResponseDto TelemetryController::GetMetrics()
    {
        return SystemTelemetryProvider::QuerySystemMetrics();
    }
}
