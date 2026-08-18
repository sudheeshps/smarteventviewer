#include "pch.h"
#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "Core/TelemetryService.h"
#include "Platform/WindowsSystemTelemetryProvider.h"
#include "System/DateTime.h"
#include "System/Console.h"

using Console = DotNetDupe::System::Console;

namespace SmartEventViewer {
    static unsigned long long GetCurrentTickMs() {
#if defined(_WIN32) || defined(_WIN64)
        return static_cast<unsigned long long>(GetTickCount64());
#else
        return static_cast<unsigned long long>(DotNetDupe::System::DateTime::UtcNow().GetTicks() / 10000);
#endif
    }

    TelemetryService::TelemetryService()
        : m_spProvider(DotNetDupe::System::SmartPointer<ISystemTelemetryProvider>(DotNetDupe::System::SmartPointer<WindowsSystemTelemetryProvider>::NewShared())),
          m_spNotifier(nullptr),
          m_spChangeDetector(DotNetDupe::System::SmartPointer<TelemetryChangeDetector>::NewShared()),
          m_uLastHeartbeatMs(GetCurrentTickMs()) {
    }

    TelemetryService::TelemetryService(
        const DotNetDupe::System::SmartPointer<ISystemTelemetryProvider>& spProvider,
        const DotNetDupe::System::SmartPointer<ITelemetryPushNotifier>& spNotifier,
        const DotNetDupe::System::SmartPointer<TelemetryChangeDetector>& spDetector)
        : m_spProvider(spProvider.IsNull() ? DotNetDupe::System::SmartPointer<ISystemTelemetryProvider>(DotNetDupe::System::SmartPointer<WindowsSystemTelemetryProvider>::NewShared()) : spProvider),
          m_spNotifier(spNotifier),
          m_spChangeDetector(spDetector.IsNull() ? DotNetDupe::System::SmartPointer<TelemetryChangeDetector>::NewShared() : spDetector),
          m_uLastHeartbeatMs(GetCurrentTickMs()) {
    }

    void TelemetryService::ClearCache() {
        LockCS lock(m_cacheCs);
        m_cache.Clear();
    }

    bool TelemetryService::TryGetCached(const String& sKey, unsigned long long uTtlMs, SystemMetricsResponseDto& outDto) {
        LockCS lock(m_cacheCs);
        TelemetryServiceCacheEntry entry;
        unsigned long long cur = GetCurrentTickMs();
        if (m_cache.TryGet(sKey, entry) && (cur - entry.FetchTimeMs < uTtlMs)) {
            outDto = entry.MetricsDto;
            return true;
        }
        return false;
    }

    void TelemetryService::PutCached(const String& sKey, unsigned long long uTtlMs, const SystemMetricsResponseDto& dto) {
        TelemetryServiceCacheEntry entry;
        entry.FetchTimeMs = GetCurrentTickMs();
        entry.TtlMs = uTtlMs;
        entry.MetricsDto = dto;
        LockCS lock(m_cacheCs);
        m_cache.Put(sKey, entry);
    }

    SystemMetricsResponseDto TelemetryService::GetSummary() {
        SystemMetricsResponseDto cached;
        if (TryGetCached("summary", 1000, cached)) return cached;
        auto fresh = m_spProvider->QuerySummary();
        PutCached("summary", 1000, fresh);
        return fresh;
    }

    SystemMetricsResponseDto TelemetryService::GetCpuUsage() {
        SystemMetricsResponseDto cached;
        if (TryGetCached("cpu", 1000, cached)) return cached;
        auto fresh = m_spProvider->QueryCpuUsage();
        PutCached("cpu", 1000, fresh);
        return fresh;
    }

    SystemMetricsResponseDto TelemetryService::GetMemoryUsage() {
        SystemMetricsResponseDto cached;
        if (TryGetCached("memory", 1000, cached)) return cached;
        auto fresh = m_spProvider->QueryMemoryUsage();
        PutCached("memory", 1000, fresh);
        return fresh;
    }

    SystemMetricsResponseDto TelemetryService::GetDiskUsage() {
        SystemMetricsResponseDto cached;
        if (TryGetCached("disk", 1000, cached)) return cached;
        auto fresh = m_spProvider->QueryDiskUsage();
        PutCached("disk", 1000, fresh);
        return fresh;
    }

    SystemMetricsResponseDto TelemetryService::GetNetworkUsage() {
        SystemMetricsResponseDto cached;
        if (TryGetCached("network", 1000, cached)) return cached;
        auto fresh = m_spProvider->QueryNetworkUsage();
        PutCached("network", 1000, fresh);
        return fresh;
    }

    SystemMetricsResponseDto TelemetryService::GetProcesses() {
        SystemMetricsResponseDto cached;
        if (TryGetCached("processes", 1000, cached)) return cached;
        auto fresh = m_spProvider->QueryProcesses();
        PutCached("processes", 1000, fresh);
        return fresh;
    }

    SystemMetricsResponseDto TelemetryService::GetSessions() {
        SystemMetricsResponseDto cached;
        if (TryGetCached("sessions", 15000, cached)) return cached;
        auto fresh = m_spProvider->QuerySessions();
        PutCached("sessions", 15000, fresh);
        return fresh;
    }

    ServicesResponseDto TelemetryService::GetServices() {
        return m_spProvider->QueryServices();
    }

    SystemMetricsResponseDto TelemetryService::GetFullMetrics() {
        SystemMetricsResponseDto cached;
        if (TryGetCached("full", 2000, cached)) return cached;
        auto fresh = m_spProvider->QuerySystemMetrics();
        PutCached("full", 2000, fresh);
        return fresh;
    }

    void TelemetryService::CheckHeartbeat(unsigned long long curTimeMs) {
        if (curTimeMs - m_uLastHeartbeatMs >= 2000) {
            m_uLastHeartbeatMs = curTimeMs;
            if (!m_spNotifier.IsNull()) {
                m_spNotifier->BroadcastCategoryUpdate("summary");
                m_spNotifier->BroadcastCategoryUpdate("processes");
            }
        }
    }

    void TelemetryService::SampleAndDetectChanges() {
        unsigned long long curTimeMs = GetCurrentTickMs();
        if (m_spNotifier.IsNull() || m_spChangeDetector.IsNull()) return;
        auto summary = m_spProvider->QuerySummary();
        if (m_spChangeDetector->HasSummaryChanged(summary)) {
            m_spNotifier->BroadcastCategoryUpdate("summary");
        }
        auto processes = m_spProvider->QueryProcesses();
        if (m_spChangeDetector->HaveProcessesChanged(processes)) {
            m_spNotifier->BroadcastCategoryUpdate("processes");
        }
        auto sessions = GetSessions();
        if (m_spChangeDetector->HaveSessionsChanged(sessions)) {
            m_spNotifier->BroadcastCategoryUpdate("sessions");
        }
        auto services = GetServices();
        if (m_spChangeDetector->HaveServicesChanged(services)) {
            m_spNotifier->BroadcastCategoryUpdate("services");
        }
        CheckHeartbeat(curTimeMs);
    }
}
