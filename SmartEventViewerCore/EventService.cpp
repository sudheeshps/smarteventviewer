#include "pch.h"
#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "Core/EventService.h"
#include "Platform/WindowsEtwLogReader.h"
#include "Core/AnomalyEngine.h"
#include "System/DateTime.h"
#include "System/Console.h"
#include "Logging/AppLoggerManager.h"

using Console = DotNetDupe::System::Console;
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

    static unsigned long long GetCurrentTickMs() {
#if defined(_WIN32) || defined(_WIN64)
        return static_cast<unsigned long long>(GetTickCount64());
#else
        return static_cast<unsigned long long>(DotNetDupe::System::DateTime::UtcNow().GetTicks() / 10000);
#endif
    }

    static DotNetDupe::System::SmartPointer<IEventService> s_spDefaultEventService = nullptr;
    static CriticalSection s_defaultEventServiceCs;

    DotNetDupe::System::SmartPointer<IEventService> EventService::GetDefault() {
        LockCS lock(s_defaultEventServiceCs);
        if (s_spDefaultEventService.IsNull()) {
            s_spDefaultEventService = DotNetDupe::System::SmartPointer<EventService>::NewShared();
        }
        return s_spDefaultEventService;
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
        static ChannelsResponseDto s_cachedChannels;
        static unsigned long long s_lastChannelsFetchMs = 0;
        unsigned long long cur = GetCurrentTickMs();
        if (cur - s_lastChannelsFetchMs < 30000 && s_cachedChannels.Channels.GetCount() > 0) return s_cachedChannels;
        ChannelsResponseDto dto;
        try {
            dto.Channels = m_spReader->GetEventChannels();
            s_cachedChannels = dto;
            s_lastChannelsFetchMs = cur;
        } catch (const DotNetDupe::System::Exception& ex) {
            AppLoggerManager::Error("SERVER", String::Format("[EVENTS_SVC] GetChannels error: {0}", ex.What()));
        }
        return dto;
    }

    bool EventService::TryGetCachedSummary(const String& sChannel, unsigned long long uTotal, EventSummaryResponseDto& outSummary) {
        LockCS lock(m_cacheCs);
        EventChannelCacheEntry entry;
        unsigned long long cur = GetCurrentTickMs();
        if (m_cache.TryGet(sChannel, entry) && (cur - entry.FetchTimeMs < 10000 || (entry.LastEventCount == uTotal && uTotal > 0))) {
            outSummary = entry.SummaryDto;
            return true;
        }
        return false;
    }

    void EventService::ApplySampledCounts(const String& sChannel, EventSummaryResponseDto& dto) {
        auto sample = m_spReader->ReadEvents(sChannel, 100, 0, true);
        for (int i = 0; i < sample.GetCount(); ++i) {
            auto lvl = sample[i].GetLevel();
            if (lvl == EventLevel::Critical) dto.CriticalCount++;
            else if (lvl == EventLevel::Error) dto.ErrorCount++;
            else if (lvl == EventLevel::Warning) dto.WarningCount++;
            else if (lvl == EventLevel::Verbose) dto.VerboseCount++;
        }
        unsigned long long uNonInfo = dto.CriticalCount + dto.ErrorCount + dto.WarningCount + dto.VerboseCount;
        dto.InfoCount = (dto.TotalCount >= uNonInfo) ? (dto.TotalCount - uNonInfo) : 0ULL;
    }

    void EventService::CalculateSummaryCounts(const String& sChannel, EventSummaryResponseDto& dto) {
        dto.Channel = sChannel;
        dto.TotalCount = m_spReader->GetChannelEventCount(sChannel);
        EventLevelCounts counts;
        if (m_spReader->GetChannelLevelCounts(sChannel, counts)) {
            dto.CriticalCount = counts.CriticalCount;
            dto.ErrorCount = counts.ErrorCount;
            dto.WarningCount = counts.WarningCount;
            dto.InfoCount = counts.InfoCount;
            dto.VerboseCount = counts.VerboseCount;
            return;
        }
        ApplySampledCounts(sChannel, dto);
    }

    EventSummaryResponseDto EventService::GetEventSummary(const String& sChannelName) {
        String sTarget = sChannelName.IsEmpty() ? String("Application") : sChannelName;
        EventSummaryResponseDto cached;
        unsigned long long uTotal = m_spReader->GetChannelEventCount(sTarget);
        if (TryGetCachedSummary(sTarget, uTotal, cached)) return cached;

        EventSummaryResponseDto dto;
        CalculateSummaryCounts(sTarget, dto);
        EventChannelCacheEntry entry;
        entry.ChannelName = sTarget;
        entry.LastEventCount = uTotal;
        entry.FetchTimeMs = GetCurrentTickMs();
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
        if (!m_cache.TryGet(sChannel, entry)) {
            entry.ChannelName = sChannel;
            entry.FetchTimeMs = GetCurrentTickMs();
        }
        entry.CachedPages[sPageKey] = pageDto;
        m_cache.Put(sChannel, entry);
    }

    static bool CaseInsensitiveEqual(const String& s1, const String& s2) {
        if (s1.GetLength() != s2.GetLength()) return false;
        int nLen = s1.GetLength();
        if (nLen == 0) return true;
        return String::Compare(s1, 0, s2, 0, nLen, true) == 0;
    }

    static EventLevel ParseLevelFilter(const String& sFilter) {
        if (CaseInsensitiveEqual(sFilter, "Critical")) return EventLevel::Critical;
        if (CaseInsensitiveEqual(sFilter, "Error")) return EventLevel::Error;
        if (CaseInsensitiveEqual(sFilter, "Warning")) return EventLevel::Warning;
        if (CaseInsensitiveEqual(sFilter, "Information") || CaseInsensitiveEqual(sFilter, "Informational")) return EventLevel::Informational;
        if (CaseInsensitiveEqual(sFilter, "Verbose")) return EventLevel::Verbose;
        return EventLevel::LogAlways;
    }

    static unsigned long long GetFilteredTotalCount(IEventLogReader* pReader, const String& sChannel, EventLevel level, unsigned long long uTotal) {
        if (level == EventLevel::LogAlways) return uTotal;
        EventLevelCounts counts;
        if (pReader && pReader->GetChannelLevelCounts(sChannel, counts)) {
            if (level == EventLevel::Critical) return counts.CriticalCount;
            if (level == EventLevel::Error) return counts.ErrorCount;
            if (level == EventLevel::Warning) return counts.WarningCount;
            if (level == EventLevel::Informational) return counts.InfoCount;
            if (level == EventLevel::Verbose) return counts.VerboseCount;
        }
        return uTotal;
    }

    EventLogResponseDto EventService::GetEvents(
        const String& sChannelName, size_t nPage, size_t nPageSize, const String& sLevelFilter) {
        String sTarget = sChannelName.IsEmpty() ? String("Application") : sChannelName;
        String sPageKey = String::Format("{0}_{1}_{2}", static_cast<double>(nPage), static_cast<double>(nPageSize), sLevelFilter);
        EventLogResponseDto cachedPage;
        if (TryGetCachedPage(sTarget, sPageKey, cachedPage)) return cachedPage;

        EventLevel eLevel = ParseLevelFilter(sLevelFilter);
        unsigned long long uTotalChannel = m_spReader->GetChannelEventCount(sTarget);
        unsigned long long uFilteredTotal = GetFilteredTotalCount(m_spReader.Get(), sTarget, eLevel, uTotalChannel);

        EventLogResponseDto result;
        result.Channel = sTarget;
        result.Page = nPage > 0 ? nPage : 1;
        result.PageSize = nPageSize > 0 ? nPageSize : 20;
        result.TotalCount = uFilteredTotal;
        result.TotalPages = (result.TotalCount + result.PageSize - 1) / (result.PageSize > 0 ? result.PageSize : 1);

        size_t nStartIndex = (result.Page - 1) * result.PageSize;
        auto rawEvents = m_spReader->ReadEvents(sTarget, result.PageSize, nStartIndex, true, eLevel);
        for (int i = 0; i < rawEvents.GetCount(); ++i) {
            result.Events.Add(MapEventDto(rawEvents[i], nStartIndex + i + 1));
        }
        StoreCachedPage(sTarget, sPageKey, result);
        return result;
    }
}
