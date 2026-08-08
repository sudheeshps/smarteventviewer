#include "pch.h"
#include "Core/TelemetryCacheManager.h"
#include "Core/SystemTelemetryProvider.h"
#include "System/Console.h"
#include "System/Diagnostics/Stopwatch.h"

using Console = DotNetDupe::System::Console;
using Stopwatch = DotNetDupe::System::Diagnostics::Stopwatch;

namespace SmartEventViewer
{
    TelemetryCacheManager& TelemetryCacheManager::GetInstance()
    {
        static TelemetryCacheManager instance;
        return instance;
    }

    void TelemetryCacheManager::Clear()
    {
        LockCS lock(m_cacheCs);
        m_cache.Clear();
    }

    SystemMetricsResponseDto TelemetryCacheManager::GetSummary()
    {
        Stopwatch sw = Stopwatch::StartNew();
        ULONGLONG curTimeMs = GetTickCount64();
        TelemetryCacheEntry entry;

        {
            LockCS lock(m_cacheCs);
            if (m_cache.TryGet("summary", entry) && (curTimeMs - entry.FetchTimeMs < 1000))
            {
                sw.Stop();
                Console::WriteLine(String::Format("[TELEMETRY_CACHE] GetSummary Cache HIT (Elapsed: {0} ms)", static_cast<double>(sw.ElapsedMilliseconds())));
                return entry.MetricsDto;
            }
        }

        entry.FetchTimeMs = curTimeMs;
        entry.TtlMs = 1000;
        entry.MetricsDto = SystemTelemetryProvider::QuerySummary();

        {
            LockCS lock(m_cacheCs);
            m_cache.Put("summary", entry);
        }

        sw.Stop();
        Console::WriteLine(String::Format("[TELEMETRY_CACHE] GetSummary Cache MISS (Queried SystemMetrics) (Elapsed: {0} ms)", static_cast<double>(sw.ElapsedMilliseconds())));
        return entry.MetricsDto;
    }

    SystemMetricsResponseDto TelemetryCacheManager::GetCpuUsage()
    {
        Stopwatch sw = Stopwatch::StartNew();
        ULONGLONG curTimeMs = GetTickCount64();
        TelemetryCacheEntry entry;

        {
            LockCS lock(m_cacheCs);
            if (m_cache.TryGet("cpu", entry) && (curTimeMs - entry.FetchTimeMs < 1000))
            {
                sw.Stop();
                return entry.MetricsDto;
            }
        }

        entry.FetchTimeMs = curTimeMs;
        entry.TtlMs = 1000;
        entry.MetricsDto = SystemTelemetryProvider::QueryCpuUsage();

        {
            LockCS lock(m_cacheCs);
            m_cache.Put("cpu", entry);
        }

        sw.Stop();
        return entry.MetricsDto;
    }

    SystemMetricsResponseDto TelemetryCacheManager::GetMemoryUsage()
    {
        Stopwatch sw = Stopwatch::StartNew();
        ULONGLONG curTimeMs = GetTickCount64();
        TelemetryCacheEntry entry;

        {
            LockCS lock(m_cacheCs);
            if (m_cache.TryGet("memory", entry) && (curTimeMs - entry.FetchTimeMs < 1000))
            {
                sw.Stop();
                return entry.MetricsDto;
            }
        }

        entry.FetchTimeMs = curTimeMs;
        entry.TtlMs = 1000;
        entry.MetricsDto = SystemTelemetryProvider::QueryMemoryUsage();

        {
            LockCS lock(m_cacheCs);
            m_cache.Put("memory", entry);
        }

        sw.Stop();
        return entry.MetricsDto;
    }

    SystemMetricsResponseDto TelemetryCacheManager::GetDiskUsage()
    {
        Stopwatch sw = Stopwatch::StartNew();
        ULONGLONG curTimeMs = GetTickCount64();
        TelemetryCacheEntry entry;

        {
            LockCS lock(m_cacheCs);
            if (m_cache.TryGet("disk", entry) && (curTimeMs - entry.FetchTimeMs < 1000))
            {
                sw.Stop();
                return entry.MetricsDto;
            }
        }

        entry.FetchTimeMs = curTimeMs;
        entry.TtlMs = 1000;
        entry.MetricsDto = SystemTelemetryProvider::QueryDiskUsage();

        {
            LockCS lock(m_cacheCs);
            m_cache.Put("disk", entry);
        }

        sw.Stop();
        return entry.MetricsDto;
    }

    SystemMetricsResponseDto TelemetryCacheManager::GetNetworkUsage()
    {
        Stopwatch sw = Stopwatch::StartNew();
        ULONGLONG curTimeMs = GetTickCount64();
        TelemetryCacheEntry entry;

        {
            LockCS lock(m_cacheCs);
            if (m_cache.TryGet("network", entry) && (curTimeMs - entry.FetchTimeMs < 1000))
            {
                sw.Stop();
                return entry.MetricsDto;
            }
        }

        entry.FetchTimeMs = curTimeMs;
        entry.TtlMs = 1000;
        entry.MetricsDto = SystemTelemetryProvider::QueryNetworkUsage();

        {
            LockCS lock(m_cacheCs);
            m_cache.Put("network", entry);
        }

        sw.Stop();
        return entry.MetricsDto;
    }

    SystemMetricsResponseDto TelemetryCacheManager::GetProcesses()
    {
        Stopwatch sw = Stopwatch::StartNew();
        ULONGLONG curTimeMs = GetTickCount64();
        TelemetryCacheEntry entry;

        {
            LockCS lock(m_cacheCs);
            if (m_cache.TryGet("processes", entry) && (curTimeMs - entry.FetchTimeMs < 1000))
            {
                sw.Stop();
                Console::WriteLine(String::Format("[TELEMETRY_CACHE] GetProcesses Cache HIT (Elapsed: {0} ms)", static_cast<double>(sw.ElapsedMilliseconds())));
                return entry.MetricsDto;
            }
        }

        entry.FetchTimeMs = curTimeMs;
        entry.TtlMs = 1000;
        entry.MetricsDto = SystemTelemetryProvider::QueryProcesses();

        {
            LockCS lock(m_cacheCs);
            m_cache.Put("processes", entry);
        }

        sw.Stop();
        Console::WriteLine(String::Format("[TELEMETRY_CACHE] GetProcesses Cache MISS (Queried Processes) (Elapsed: {0} ms)", static_cast<double>(sw.ElapsedMilliseconds())));
        return entry.MetricsDto;
    }

    SystemMetricsResponseDto TelemetryCacheManager::GetSessions()
    {
        Stopwatch sw = Stopwatch::StartNew();
        ULONGLONG curTimeMs = GetTickCount64();
        TelemetryCacheEntry entry;

        {
            LockCS lock(m_cacheCs);
            if (m_cache.TryGet("sessions", entry) && (curTimeMs - entry.FetchTimeMs < 15000))
            {
                sw.Stop();
                Console::WriteLine(String::Format("[TELEMETRY_CACHE] GetSessions Cache HIT (Elapsed: {0} ms)", static_cast<double>(sw.ElapsedMilliseconds())));
                return entry.MetricsDto;
            }
        }

        entry.FetchTimeMs = curTimeMs;
        entry.TtlMs = 15000;
        entry.MetricsDto = SystemTelemetryProvider::QuerySessions();

        {
            LockCS lock(m_cacheCs);
            m_cache.Put("sessions", entry);
        }

        sw.Stop();
        Console::WriteLine(String::Format("[TELEMETRY_CACHE] GetSessions Cache MISS (Queried User/RDP Sessions) (Elapsed: {0} ms)", static_cast<double>(sw.ElapsedMilliseconds())));
        return entry.MetricsDto;
    }

    ServicesResponseDto TelemetryCacheManager::GetServices()
    {
        return SystemTelemetryProvider::QueryServices();
    }

    SystemMetricsResponseDto TelemetryCacheManager::GetFullMetrics()
    {
        Stopwatch sw = Stopwatch::StartNew();
        ULONGLONG curTimeMs = GetTickCount64();
        TelemetryCacheEntry entry;

        {
            LockCS lock(m_cacheCs);
            if (m_cache.TryGet("full", entry) && (curTimeMs - entry.FetchTimeMs < 2000))
            {
                sw.Stop();
                Console::WriteLine(String::Format("[TELEMETRY_CACHE] GetFullMetrics Cache HIT (Elapsed: {0} ms)", static_cast<double>(sw.ElapsedMilliseconds())));
                return entry.MetricsDto;
            }
        }

        entry.FetchTimeMs = curTimeMs;
        entry.TtlMs = 2000;
        entry.MetricsDto = SystemTelemetryProvider::QuerySystemMetrics();

        {
            LockCS lock(m_cacheCs);
            m_cache.Put("full", entry);
        }

        sw.Stop();
        Console::WriteLine(String::Format("[TELEMETRY_CACHE] GetFullMetrics Cache MISS (Queried Full Metrics) (Elapsed: {0} ms)", static_cast<double>(sw.ElapsedMilliseconds())));
        return entry.MetricsDto;
    }
}
