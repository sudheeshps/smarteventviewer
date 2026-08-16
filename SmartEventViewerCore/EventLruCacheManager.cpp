#include "pch.h"
#include "Core/EventLruCacheManager.h"
#include "Core/EventsController.h"
#include "System/Console.h"
#include "System/Diagnostics/Stopwatch.h"
#include "System/Diagnostics/EtwLogReader.h"

using Console = DotNetDupe::System::Console;
using Stopwatch = DotNetDupe::System::Diagnostics::Stopwatch;

namespace SmartEventViewer {
    EventLruCacheManager& EventLruCacheManager::GetInstance() {
        static EventLruCacheManager instance;
        return instance;
    }

    void EventLruCacheManager::Clear() {
        LockCS lock(m_cacheCs);
        m_cache.Clear();
    }

    EventSummaryResponseDto EventLruCacheManager::GetSummary(const String& sTargetChannel) {
        Stopwatch sw = Stopwatch::StartNew();

        unsigned long long uOsTotalCount = 0;
        try {
            uOsTotalCount = DotNetDupe::System::Diagnostics::EtwLogReader::GetChannelEventCount(sTargetChannel);
        } catch (const DotNetDupe::System::Exception&) {
            uOsTotalCount = 0;
        } catch (const std::exception&) {
            uOsTotalCount = 0;
        } catch (...) {
            uOsTotalCount = 0;
        }

        ChannelCacheEntry entry;
        try {
            LockCS lock(m_cacheCs);
            if (m_cache.TryGet(sTargetChannel, entry)) {
                if (entry.LastEventCount == uOsTotalCount && uOsTotalCount > 0) {
                    sw.Stop();
                    Console::WriteLine(String::Format("[EVENTS_CACHE] GetSummary Cache HIT for '{0}' (Count: {1}) (Elapsed: {2:F2} ms)", sTargetChannel, uOsTotalCount, static_cast<double>(sw.ElapsedMilliseconds())));
                    return entry.SummaryDto;
                }
                entry.CachedPages.Clear();
            }
        } catch (const DotNetDupe::System::Exception&) {
        } catch (const std::exception&) {
        } catch (...) {
        }

        DotNetDupe::System::Diagnostics::EtwEventLevelCounts levelCounts{};
        try {
            levelCounts = DotNetDupe::System::Diagnostics::EtwLogReader::GetChannelEventLevelCounts(sTargetChannel);
        } catch (const DotNetDupe::System::Exception&) {
        } catch (const std::exception&) {
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
        } catch (const DotNetDupe::System::Exception&) {
        } catch (const std::exception&) {
        } catch (...) {
        }

        sw.Stop();
        try {
            Console::WriteLine(String::Format("[EVENTS_CACHE] GetSummary Cache MISS for '{0}' (Fetched from ETW) (Elapsed: {1:F2} ms)", sTargetChannel, static_cast<double>(sw.ElapsedMilliseconds())));
        } catch (const DotNetDupe::System::Exception&) {
        } catch (const std::exception&) {
        } catch (...) {
        }
        return dto;
    }

    static DotNetDupe::System::Diagnostics::EtwEventLevel ParseEtwEventLevel(const String& sLevelFilterLower) {
        if (sLevelFilterLower == "critical") return DotNetDupe::System::Diagnostics::EtwEventLevel::Critical;
        if (sLevelFilterLower == "error") return DotNetDupe::System::Diagnostics::EtwEventLevel::Error;
        if (sLevelFilterLower == "warning") return DotNetDupe::System::Diagnostics::EtwEventLevel::Warning;
        if (sLevelFilterLower == "information" || sLevelFilterLower == "info") return DotNetDupe::System::Diagnostics::EtwEventLevel::Info;
        if (sLevelFilterLower == "verbose") return DotNetDupe::System::Diagnostics::EtwEventLevel::Verbose;
        return DotNetDupe::System::Diagnostics::EtwEventLevel::All;
    }

    static unsigned long long GetFilteredLevelCount(const String& sTargetChannel, const String& sLevelFilterLower, unsigned long long uDefaultCount) {
        if (sLevelFilterLower == "all") return uDefaultCount;
        try {
            auto levelCounts = DotNetDupe::System::Diagnostics::EtwLogReader::GetChannelEventLevelCounts(sTargetChannel);
            if (sLevelFilterLower == "critical") return levelCounts.uCriticalCount;
            if (sLevelFilterLower == "error") return levelCounts.uErrorCount;
            if (sLevelFilterLower == "warning") return levelCounts.uWarningCount;
            if (sLevelFilterLower == "information" || sLevelFilterLower == "info") return levelCounts.uInfoCount;
            if (sLevelFilterLower == "verbose") return levelCounts.uVerboseCount;
        } catch (...) {}
        return uDefaultCount;
    }

    static String LevelToString(EventLevel lvl) {
        if (lvl == EventLevel::Critical) return "Critical";
        if (lvl == EventLevel::Error) return "Error";
        if (lvl == EventLevel::Warning) return "Warning";
        if (lvl == EventLevel::Verbose) return "Verbose";
        return "Information";
    }

    static String ResolveEventLevel(const EventRecord& evt, const DotNetDupe::System::Diagnostics::EtwEventLevel& etwLevel, int iRawLevel) {
        if (evt.GetLevel() != EventLevel::Informational || iRawLevel != 0) return LevelToString(evt.GetLevel());
        if (etwLevel == DotNetDupe::System::Diagnostics::EtwEventLevel::Critical) return "Critical";
        if (etwLevel == DotNetDupe::System::Diagnostics::EtwEventLevel::Error) return "Error";
        if (etwLevel == DotNetDupe::System::Diagnostics::EtwEventLevel::Warning) return "Warning";
        if (etwLevel == DotNetDupe::System::Diagnostics::EtwEventLevel::Verbose) return "Verbose";
        if (etwLevel == DotNetDupe::System::Diagnostics::EtwEventLevel::Info) return "Information";
        return LevelToString(evt.GetLevel());
    }

    static void PopulateEventsList(const String& sChannel, size_t page, size_t pageSize, const String& sLevelFilterLower, EventLogResponseDto& responseDto) {
        size_t startIndex = (page - 1) * pageSize;
        auto etwLevel = ParseEtwEventLevel(sLevelFilterLower);
        auto rawEvents = DotNetDupe::System::Diagnostics::EtwLogReader::ReadEvents(sChannel, static_cast<int>(pageSize), static_cast<int>(startIndex), true, etwLevel);
        for (int i = 0; i < rawEvents.GetCount(); ++i) {
            EventRecord evt = EventRecord::FromEtwEvent(rawEvents[i], sChannel);
            EventDto dto;
            dto.Index = startIndex + i + 1;
            dto.Id = evt.GetEventId();
            dto.Level = ResolveEventLevel(evt, etwLevel, rawEvents[i].iLevel);
            dto.Risk = (evt.GetRiskLevel() == RiskLevel::Critical ? "Critical" : (evt.GetRiskLevel() == RiskLevel::High ? "High" : (evt.GetRiskLevel() == RiskLevel::Medium ? "Medium" : "Low")));
            dto.Provider = evt.GetProviderName();
            dto.Time = evt.GetTimeCreated();
            dto.Message = evt.GetEventMessage();
            dto.RawXml = evt.GetRawXml();
            responseDto.Events.Add(dto);
        }
    }

    EventLogResponseDto EventLruCacheManager::GetEvents(const String& sTargetChannel, size_t page, size_t pageSize, const String& sLevelFilter) {
        if (page < 1) page = 1;
        if (pageSize < 1) pageSize = 20;

        String sPageKey = String::Format("{0}_{1}_{2}", page, pageSize, sLevelFilter);
        unsigned long long uOsTotalCount = 0;
        try { uOsTotalCount = DotNetDupe::System::Diagnostics::EtwLogReader::GetChannelEventCount(sTargetChannel); } catch (...) {}

        ChannelCacheEntry entry;
        {
            LockCS lock(m_cacheCs);
            if (m_cache.TryGet(sTargetChannel, entry) && entry.LastEventCount == uOsTotalCount && uOsTotalCount > 0) {
                EventLogResponseDto cachedPage;
                if (entry.CachedPages.TryGetValue(sPageKey, cachedPage)) return cachedPage;
            } else { entry.CachedPages.Clear(); entry.ChannelName = sTargetChannel; }
        }

        String sLevelFilterLower = sLevelFilter;
        sLevelFilterLower.ToLower();
        unsigned long long uFilteredCount = GetFilteredLevelCount(sTargetChannel, sLevelFilterLower, uOsTotalCount);

        EventLogResponseDto responseDto;
        responseDto.Channel = sTargetChannel;
        responseDto.TotalCount = uFilteredCount;
        responseDto.Page = page;
        responseDto.PageSize = pageSize;
        responseDto.TotalPages = (uFilteredCount == 0) ? 0 : static_cast<size_t>((uFilteredCount + pageSize - 1) / pageSize);

        PopulateEventsList(sTargetChannel, page, pageSize, sLevelFilterLower, responseDto);
        entry.LastEventCount = uOsTotalCount;
        entry.CachedPages[sPageKey] = responseDto;
        { LockCS lock(m_cacheCs); m_cache.Put(sTargetChannel, entry); }
        return responseDto;
    }
}
