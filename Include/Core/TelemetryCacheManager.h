#pragma once

#include "Common.h"
#include "Core/TelemetryController.h"
#include "Collections/LruCache.h"

#include "System/Threading/CriticalSection.h"
#include "System/Threading/Lock.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;
    using CriticalSection = DotNetDupe::System::Threading::CriticalSection;
    using LockCS = DotNetDupe::System::Threading::Lock<CriticalSection>;

    struct TelemetryCacheEntry
    {
        ULONGLONG FetchTimeMs{ 0 };
        ULONGLONG TtlMs{ 2000 };
        SystemMetricsResponseDto MetricsDto{};
    };

    class SMARTEVENTVIEWER_API TelemetryCacheManager
    {
    private:
        LruCache<String, TelemetryCacheEntry> m_cache{ 5 };
        CriticalSection m_cacheCs;

        TelemetryCacheManager() = default;
        ~TelemetryCacheManager() = default;

        TelemetryCacheManager(const TelemetryCacheManager&) = delete;
        TelemetryCacheManager& operator=(const TelemetryCacheManager&) = delete;

    public:
        static TelemetryCacheManager& GetInstance();

        SystemMetricsResponseDto GetSummary();
        SystemMetricsResponseDto GetCpuUsage();
        SystemMetricsResponseDto GetMemoryUsage();
        SystemMetricsResponseDto GetDiskUsage();
        SystemMetricsResponseDto GetNetworkUsage();
        SystemMetricsResponseDto GetProcesses();
        SystemMetricsResponseDto GetSessions();
        ServicesResponseDto GetServices();
        SystemMetricsResponseDto GetFullMetrics();

        void Clear();
    };
}
