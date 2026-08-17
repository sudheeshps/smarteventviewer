#pragma once

#include "ViewerCommon.h"
#include "Core/IEventService.h"
#include "Core/IEventLogReader.h"
#include "Core/IAnomalyEngine.h"
#include "Collections/LruCache.h"
#include "System/Threading/CriticalSection.h"
#include "System/Threading/Lock.h"
#include "System/SmartPointer.h"

namespace SmartEventViewer {
    using CriticalSection = DotNetDupe::System::Threading::CriticalSection;
    using LockCS = DotNetDupe::System::Threading::Lock<CriticalSection>;

    struct EventChannelCacheEntry {
        String ChannelName{};
        unsigned long long LastEventCount{ 0 };
        EventSummaryResponseDto SummaryDto{};
        DotNetDupe::System::Collections::Generic::Dictionary<String, EventLogResponseDto> CachedPages{};
    };

    class SMARTEVENTVIEWER_API EventService : public IEventService {
    private:
        DotNetDupe::System::SmartPointer<IEventLogReader> m_spReader{ nullptr };
        DotNetDupe::System::SmartPointer<IAnomalyEngine> m_spAnomalyEngine{ nullptr };
        LruCache<String, EventChannelCacheEntry> m_cache{ 10 };
        mutable CriticalSection m_cacheCs{};

        EventDto MapEventDto(const EventRecord& record, size_t nIdx);
        void CalculateSummaryCounts(const String& sChannel, EventSummaryResponseDto& dto);
        bool TryGetCachedSummary(const String& sChannel, unsigned long long uTotal, EventSummaryResponseDto& outSummary);
        bool TryGetCachedPage(const String& sChannel, const String& sPageKey, EventLogResponseDto& outPage);
        void StoreCachedPage(const String& sChannel, const String& sPageKey, const EventLogResponseDto& pageDto);

    public:
        EventService();
        explicit EventService(
            const DotNetDupe::System::SmartPointer<IEventLogReader>& spReader,
            const DotNetDupe::System::SmartPointer<IAnomalyEngine>& spEngine = nullptr);
        ~EventService() override = default;

        ChannelsResponseDto GetChannels() override;
        EventSummaryResponseDto GetEventSummary(const String& sChannelName) override;
        EventLogResponseDto GetEvents(
            const String& sChannelName,
            size_t nPage,
            size_t nPageSize,
            const String& sLevelFilter = "ALL") override;
        void ClearCache() override;
    };
}
