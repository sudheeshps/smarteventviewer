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
            if (m_cache.TryGet(String("summary"), entry) && (curTimeMs - entry.FetchTimeMs < 1000))
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
            m_cache.Put(String("summary"), entry);
        }

        sw.Stop();
        Console::WriteLine(String::Format("[TELEMETRY_CACHE] GetSummary Cache MISS (Queried SystemMetrics) (Elapsed: {0} ms)", static_cast<double>(sw.ElapsedMilliseconds())));
        return entry.MetricsDto;
    }

    SystemMetricsResponseDto TelemetryCacheManager::GetProcesses()
    {
        Stopwatch sw = Stopwatch::StartNew();
        ULONGLONG curTimeMs = GetTickCount64();
        TelemetryCacheEntry entry;

        {
            LockCS lock(m_cacheCs);
            if (m_cache.TryGet(String("processes"), entry) && (curTimeMs - entry.FetchTimeMs < 1000))
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
            m_cache.Put(String("processes"), entry);
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
            if (m_cache.TryGet(String("sessions"), entry) && (curTimeMs - entry.FetchTimeMs < 15000))
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
            m_cache.Put(String("sessions"), entry);
        }

        sw.Stop();
        Console::WriteLine(String::Format("[TELEMETRY_CACHE] GetSessions Cache MISS (Queried User/RDP Sessions) (Elapsed: {0} ms)", static_cast<double>(sw.ElapsedMilliseconds())));
        return entry.MetricsDto;
    }

    SystemMetricsResponseDto TelemetryCacheManager::GetFullMetrics()
    {
        Stopwatch sw = Stopwatch::StartNew();
        ULONGLONG curTimeMs = GetTickCount64();
        TelemetryCacheEntry entry;

        {
            LockCS lock(m_cacheCs);
            if (m_cache.TryGet(String("full"), entry) && (curTimeMs - entry.FetchTimeMs < 2000))
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
            m_cache.Put(String("full"), entry);
        }

        sw.Stop();
        Console::WriteLine(String::Format("[TELEMETRY_CACHE] GetFullMetrics Cache MISS (Queried Full Metrics) (Elapsed: {0} ms)", static_cast<double>(sw.ElapsedMilliseconds())));
        return entry.MetricsDto;
    }
}
