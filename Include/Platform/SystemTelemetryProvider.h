#pragma once

#include "Common.h"
#include "Core/EventDtos.h"

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wtsapi32.h>
#pragma comment(lib, "wtsapi32.lib")
#endif

namespace SmartEventViewer
{
    class SMARTEVENTVIEWER_API SystemTelemetryProvider
    {
    public:
        SystemTelemetryProvider() = default;
        ~SystemTelemetryProvider() = default;

        static SystemMetricsResponseDto QuerySystemMetrics();
    };
}
