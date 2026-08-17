#pragma once

#include "ViewerCommon.h"
#include "Core/ITelemetryService.h"
#include "Core/ISystemTelemetryProvider.h"
#include "Core/ITelemetryPushNotifier.h"
#include "Core/TelemetryChangeDetector.h"
#include "Collections/LruCache.h"
#include "System/Threading/CriticalSection.h"
#include "System/Threading/Lock.h"
#include "System/SmartPointer.h"

namespace SmartEventViewer {
    using CriticalSection = DotNetDupe::System::Threading::CriticalSection;
    using LockCS = DotNetDupe::System::Threading::Lock<CriticalSection>;

    struct TelemetryServiceCacheEntry {
        unsigned long long FetchTimeMs{ 0 };
        unsigned long long TtlMs{ 1000 };
        SystemMetricsResponseDto MetricsDto{};
    };

    class SMARTEVENTVIEWER_API TelemetryService : public ITelemetryService {
    private:
        DotNetDupe::System::SmartPointer<ISystemTelemetryProvider> m_spProvider{ nullptr };
        DotNetDupe::System::SmartPointer<ITelemetryPushNotifier> m_spNotifier{ nullptr };
        DotNetDupe::System::SmartPointer<TelemetryChangeDetector> m_spChangeDetector{ nullptr };
        LruCache<String, TelemetryServiceCacheEntry> m_cache{ 10 };
        mutable CriticalSection m_cacheCs{};
        unsigned long long m_uLastHeartbeatMs{ 0 };

        bool TryGetCached(const String& sKey, unsigned long long uTtlMs, SystemMetricsResponseDto& outDto);
        void PutCached(const String& sKey, unsigned long long uTtlMs, const SystemMetricsResponseDto& dto);
        void CheckHeartbeat(unsigned long long curTimeMs);

    public:
        TelemetryService();
        explicit TelemetryService(
            const DotNetDupe::System::SmartPointer<ISystemTelemetryProvider>& spProvider,
            const DotNetDupe::System::SmartPointer<ITelemetryPushNotifier>& spNotifier = nullptr,
            const DotNetDupe::System::SmartPointer<TelemetryChangeDetector>& spDetector = nullptr);
        ~TelemetryService() override = default;

        SystemMetricsResponseDto GetSummary() override;
        SystemMetricsResponseDto GetCpuUsage() override;
        SystemMetricsResponseDto GetMemoryUsage() override;
        SystemMetricsResponseDto GetDiskUsage() override;
        SystemMetricsResponseDto GetNetworkUsage() override;
        SystemMetricsResponseDto GetProcesses() override;
        SystemMetricsResponseDto GetSessions() override;
        ServicesResponseDto GetServices() override;
        SystemMetricsResponseDto GetFullMetrics() override;
        void SampleAndDetectChanges() override;
        void ClearCache() override;
    };
}
