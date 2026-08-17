#include "pch.h"
#include "Core/EventService.h"
#include "Platform/WindowsEtwLogReader.h"
#include "Core/AnomalyEngine.h"
#include "System/Diagnostics/Stopwatch.h"
#include "System/Console.h"
#include "Logging/AppLoggerManager.h"

using Console = DotNetDupe::System::Console;
using Stopwatch = DotNetDupe::System::Diagnostics::Stopwatch;
using DotNetDupe::System::SmartPointer;

namespace SmartEventViewer {
    static String EventLevelToString(EventLevel lvl) {
        switch (lvl) {
            case EventLevel::Critical: return "Critical";
            case EventLevel::Error: return "Error";
            case EventLevel::Warning: return "Warning";
            case EventLevel::Informational: return "Information";
            case EventLevel::Verbose: return "Verbose";
            default: return "Information";
        }
    }

    static String RiskLevelToString(RiskLevel rsk) {
        switch (rsk) {
            case RiskLevel::Critical: return "Critical";
            case RiskLevel::High: return "High";
            case RiskLevel::Medium: return "Medium";
            case RiskLevel::Low: return "Low";
            default: return "Low";
        }
    }

    EventService::EventService()
        : m_spReader(SmartPointer<IEventLogReader>(SmartPointer<WindowsEtwLogReader>::NewShared())),
          m_spAnomalyEngine(SmartPointer<IAnomalyEngine>(SmartPointer<AnomalyEngine>::NewShared())) {
    }

    EventService::EventService(
        const DotNetDupe::System::SmartPointer<IEventLogReader>& spReader,
        const DotNetDupe::System::SmartPointer<IAnomalyEngine>& spEngine)
        : m_spReader(spReader.IsNull() ? SmartPointer<IEventLogReader>(SmartPointer<WindowsEtwLogReader>::NewShared()) : spReader),
          m_spAnomalyEngine(spEngine.IsNull() ? SmartPointer<IAnomalyEngine>(SmartPointer<AnomalyEngine>::NewShared()) : spEngine) {
    }

    void EventService::ClearCache() {
        LockCS lock(m_cacheCs);
        m_cache.Clear();
    }

    ChannelsResponseDto EventService::GetChannels() {
        ChannelsResponseDto dto;
        try {
            dto.Channels = m_spReader->GetEventChannels();
        } catch (const DotNetDupe::System::Exception& ex) {
            AppLoggerManager::Error("SERVER", String::Format("[EVENTS_SVC] GetChannels error: {0}", ex.What()));
        }
        return dto;
    }

    bool EventService::TryGetCachedSummary(const String& sChannel, unsigned long long uTotal, EventSummaryResponseDto& outSummary) {
        LockCS lock(m_cacheCs);
        EventChannelCacheEntry entry;
        if (m_cache.TryGet(sChannel, entry) && entry.LastEventCount == uTotal && uTotal > 0) {
            outSummary = entry.SummaryDto;
            return true;
        }
        return false;
    }

    void EventService::CalculateSummaryCounts(const String& sChannel, EventSummaryResponseDto& dto) {
        dto.Channel = sChannel;
        dto.TotalCount = m_spReader->GetChannelEventCount(sChannel);
        auto sample = m_spReader->ReadEvents(sChannel, 100, 0, true);
        for (int i = 0; i < sample.GetCount(); ++i) {
            auto lvl = sample[i].GetLevel();
            if (lvl == EventLevel::Critical) dto.CriticalCount++;
            else if (lvl == EventLevel::Error) dto.ErrorCount++;
            else if (lvl == EventLevel::Warning) dto.WarningCount++;
            else if (lvl == EventLevel::Informational) dto.InfoCount++;
            else if (lvl == EventLevel::Verbose) dto.VerboseCount++;
        }
    }

    EventSummaryResponseDto EventService::GetEventSummary(const String& sChannelName) {
        String sTarget = sChannelName.IsEmpty() ? String("Application") : sChannelName;
        unsigned long long uTotal = m_spReader->GetChannelEventCount(sTarget);
        EventSummaryResponseDto dto;
        if (TryGetCachedSummary(sTarget, uTotal, dto)) return dto;

        CalculateSummaryCounts(sTarget, dto);
        EventChannelCacheEntry entry;
        entry.ChannelName = sTarget;
        entry.LastEventCount = uTotal;
        entry.SummaryDto = dto;

        LockCS lock(m_cacheCs);
        m_cache.Put(sTarget, entry);
        return dto;
    }

    EventDto EventService::MapEventDto(const EventRecord& record, size_t nIdx) {
        EventDto dto;
        dto.Index = nIdx;
        dto.Id = record.GetEventId();
        dto.Level = EventLevelToString(record.GetLevel());
        dto.Provider = record.GetProviderName();
        dto.Time = record.GetTimeCreated();
        dto.Message = record.GetEventMessage();
        dto.RawXml = record.GetRawXml();
        dto.Risk = RiskLevelToString(m_spAnomalyEngine->EvaluateRisk(record));
        return dto;
    }

    bool EventService::TryGetCachedPage(const String& sChannel, const String& sPageKey, EventLogResponseDto& outPage) {
        LockCS lock(m_cacheCs);
        EventChannelCacheEntry entry;
        if (m_cache.TryGet(sChannel, entry) && entry.CachedPages.TryGetValue(sPageKey, outPage)) {
            return true;
        }
        return false;
    }

    void EventService::StoreCachedPage(const String& sChannel, const String& sPageKey, const EventLogResponseDto& pageDto) {
        LockCS lock(m_cacheCs);
        EventChannelCacheEntry entry;
        if (m_cache.TryGet(sChannel, entry)) {
            entry.CachedPages[sPageKey] = pageDto;
            m_cache.Put(sChannel, entry);
        }
    }

    static bool CaseInsensitiveEqual(const String& s1, const String& s2) {
        if (s1.GetLength() != s2.GetLength()) return false;
        int nLen = s1.GetLength();
        if (nLen == 0) return true;
        return String::Compare(s1, 0, s2, 0, nLen, true) == 0;
    }

    static bool MatchesLevelFilter(const EventRecord& record, const String& sFilter) {
        if (sFilter == "ALL" || sFilter.IsEmpty()) return true;
        String sRecLvl = EventLevelToString(record.GetLevel());
        return CaseInsensitiveEqual(sRecLvl, sFilter);
    }

    EventLogResponseDto EventService::GetEvents(
        const String& sChannelName, size_t nPage, size_t nPageSize, const String& sLevelFilter) {
        String sTarget = sChannelName.IsEmpty() ? String("Application") : sChannelName;
        String sPageKey = String::Format("{0}_{1}_{2}", static_cast<double>(nPage), static_cast<double>(nPageSize), sLevelFilter);
        EventLogResponseDto cachedPage;
        if (TryGetCachedPage(sTarget, sPageKey, cachedPage)) return cachedPage;

        EventLogResponseDto result;
        result.Channel = sTarget;
        result.Page = nPage > 0 ? nPage : 1;
        result.PageSize = nPageSize > 0 ? nPageSize : 20;
        result.TotalCount = m_spReader->GetChannelEventCount(sTarget);
        result.TotalPages = (result.TotalCount + result.PageSize - 1) / (result.PageSize > 0 ? result.PageSize : 1);

        size_t nStartIndex = (result.Page - 1) * result.PageSize;
        auto rawEvents = m_spReader->ReadEvents(sTarget, result.PageSize * 2, nStartIndex, true);
        size_t count = 0;
        for (int i = 0; i < rawEvents.GetCount() && count < result.PageSize; ++i) {
            if (MatchesLevelFilter(rawEvents[i], sLevelFilter)) {
                result.Events.Add(MapEventDto(rawEvents[i], nStartIndex + count + 1));
                count++;
            }
        }
        StoreCachedPage(sTarget, sPageKey, result);
        return result;
    }
}
