#pragma once

#include "Common.h"
#include "Core/EventsController.h"
#include "System/Diagnostics/EtwLogReader.h"
#include "Collections/LruCache.h"

#include "System/Threading/CriticalSection.h"
#include "System/Threading/Lock.h"

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;
    using CriticalSection = DotNetDupe::System::Threading::CriticalSection;
    using LockCS = DotNetDupe::System::Threading::Lock<CriticalSection>;

    struct ChannelCacheEntry {
        String ChannelName;
        unsigned long long LastEventCount{ 0 };
        EventSummaryResponseDto SummaryDto{};
        
        // Keyed by "page_pageSize" (e.g. "1_20")
        DotNetDupe::System::Collections::Generic::Dictionary<String, EventLogResponseDto> CachedPages{};
    };

    class SMARTEVENTVIEWER_API EventLruCacheManager {
    private:
        LruCache<String, ChannelCacheEntry> m_cache{ 10 };
        CriticalSection m_cacheCs;

        EventLruCacheManager() = default;
        ~EventLruCacheManager() = default;

        EventLruCacheManager(const EventLruCacheManager&) = delete;
        EventLruCacheManager& operator=(const EventLruCacheManager&) = delete;

    public:
        static EventLruCacheManager& GetInstance();

        EventSummaryResponseDto GetSummary(const String& channelName);
        EventLogResponseDto GetEvents(const String& channelName, size_t page, size_t pageSize, const String& sLevelFilter = "ALL");
        void Clear();
    };
}
