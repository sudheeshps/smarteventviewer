#include "pch.h"
#include "Core/EventLruCacheManager.h"
#include "Core/EventsController.h"
#include "System/Console.h"
#include "System/Diagnostics/Stopwatch.h"
#include "System/Diagnostics/EtwLogReader.h"

using Console = DotNetDupe::System::Console;
using Stopwatch = DotNetDupe::System::Diagnostics::Stopwatch;

namespace SmartEventViewer
{
    EventLruCacheManager& EventLruCacheManager::GetInstance()
    {
        static EventLruCacheManager instance;
        return instance;
    }

    void EventLruCacheManager::Clear()
    {
        LockCS lock(m_cacheCs);
        m_cache.Clear();
    }

    EventSummaryResponseDto EventLruCacheManager::GetSummary(const String& sTargetChannel)
    {
        Stopwatch sw = Stopwatch::StartNew();

        unsigned long long uOsTotalCount = 0;
        try {
            uOsTotalCount = m_logReader.GetChannelEventCount(sTargetChannel);
        } catch (...) {
            uOsTotalCount = 0;
        }

        ChannelCacheEntry entry;
        try {
            LockCS lock(m_cacheCs);
            if (m_cache.TryGet(sTargetChannel, entry))
            {
                if (entry.LastEventCount == uOsTotalCount && uOsTotalCount > 0)
                {
                    sw.Stop();
                    Console::WriteLine(String::Format("[EVENTS_CACHE] GetSummary Cache HIT for '{0}' (Count: {1}) (Elapsed: {2:F2} ms)", sTargetChannel, uOsTotalCount, static_cast<double>(sw.ElapsedMilliseconds())));
                    return entry.SummaryDto;
                }
                entry.CachedPages.Clear();
            }
        } catch (...) {
        }

        DotNetDupe::System::Diagnostics::EtwEventLevelCounts levelCounts{};
        try {
            levelCounts = DotNetDupe::System::Diagnostics::EtwLogReader::GetChannelEventLevelCounts(sTargetChannel);
        } catch (...) {
        }

        EventSummaryResponseDto dto;
        dto.Channel = sTargetChannel;
        dto.TotalCount = uOsTotalCount;
        dto.CriticalCount = levelCounts.uCriticalCount;
        dto.ErrorCount = levelCounts.uErrorCount;
        dto.WarningCount = levelCounts.uWarningCount;
        dto.InfoCount = levelCounts.uInfoCount;
        dto.VerboseCount = levelCounts.uVerboseCount;

        entry.ChannelName = sTargetChannel;
        entry.LastEventCount = uOsTotalCount;
        entry.SummaryDto = dto;

        try {
            LockCS lock(m_cacheCs);
            m_cache.Put(sTargetChannel, entry);
        } catch (...) {
        }

        sw.Stop();
        try {
            Console::WriteLine(String::Format("[EVENTS_CACHE] GetSummary Cache MISS for '{0}' (Fetched from ETW) (Elapsed: {1:F2} ms)", sTargetChannel, static_cast<double>(sw.ElapsedMilliseconds())));
        } catch (...) {
        }
        return dto;
    }

    EventLogResponseDto EventLruCacheManager::GetEvents(const String& sTargetChannel, size_t page, size_t pageSize)
    {
        if (page < 1) page = 1;
        if (pageSize < 1) pageSize = 20;

        String sPageKey = String::Format("{0}_{1}", page, pageSize);

        unsigned long long uOsTotalCount = 0;
        try {
            uOsTotalCount = m_logReader.GetChannelEventCount(sTargetChannel);
        } catch (...) {
            uOsTotalCount = 0;
        }

        ChannelCacheEntry entry;
        {
            LockCS lock(m_cacheCs);
            if (m_cache.TryGet(sTargetChannel, entry))
            {
                if (entry.LastEventCount == uOsTotalCount && uOsTotalCount > 0)
                {
                    EventLogResponseDto cachedPage;
                    if (entry.CachedPages.TryGetValue(sPageKey, cachedPage))
                    {
                        return cachedPage;
                    }
                }
                else
                {
                    entry.CachedPages.Clear();
                }
            }
            else
            {
                entry.ChannelName = sTargetChannel;
            }
        }

        size_t totalPages = (uOsTotalCount == 0) ? 0 : static_cast<size_t>((uOsTotalCount + pageSize - 1) / pageSize);

        EventLogResponseDto responseDto;
        responseDto.Channel = sTargetChannel;
        responseDto.TotalCount = uOsTotalCount;
        responseDto.Page = page;
        responseDto.PageSize = pageSize;
        responseDto.TotalPages = totalPages;

        size_t startIndex = (page - 1) * pageSize;

        {
            LockCS lock(m_cacheCs);
            if (m_logReader.OpenLogPaged(sTargetChannel, static_cast<int>(pageSize), static_cast<int>(startIndex)))
            {
                EventRecord evt;
                size_t currentItemIdx = startIndex + 1;

                while (m_logReader.ReadNextEvent(evt))
                {
                    EventDto dto;
                    dto.Index = currentItemIdx++;
                    dto.Id = evt.GetEventId();
                    dto.Level = (evt.GetLevel() == EventLevel::Critical ? "Critical" : (evt.GetLevel() == EventLevel::Error ? "Error" : (evt.GetLevel() == EventLevel::Warning ? "Warning" : "Information")));
                    dto.Risk = (evt.GetRiskLevel() == RiskLevel::Critical ? "Critical" : (evt.GetRiskLevel() == RiskLevel::High ? "High" : (evt.GetRiskLevel() == RiskLevel::Medium ? "Medium" : "Low")));
                    dto.Provider = evt.GetProviderName();
                    dto.Time = evt.GetTimeCreated();
                    dto.Message = evt.GetEventMessage();
                    dto.RawXml = evt.GetRawXml();

                    responseDto.Events.Add(dto);
                }
                m_logReader.Close();
            }
        }

        entry.LastEventCount = uOsTotalCount;
        entry.CachedPages[sPageKey] = responseDto;

        {
            LockCS lock(m_cacheCs);
            m_cache.Put(sTargetChannel, entry);
        }

        return responseDto;
    }
}
