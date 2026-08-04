#include "pch.h"
#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <wtsapi32.h>
#include <iphlpapi.h>
#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")
#endif

#include "EventsController.h"
#include "Core/EventRecord.h"
#include "Core/AnomalyEngine.h"
#include "System/Console.h"
#include "System/Convert.h"
#include "System/Uri.h"

using Console = DotNetDupe::System::Console;
using Uri = DotNetDupe::System::Uri;
using namespace DotNetDupe::System::Threading;

namespace SmartEventViewer
{
    // =========================================================================
    // Static Member Definitions
    // =========================================================================
    CriticalSection EventsController::s_eventsCacheCs{};
    Dictionary<String, ChannelEventCache> EventsController::s_eventsCache{};

    DotNetDupe::System::Collections::Generic::List<String> EventsController::s_serverLogs{};
    CriticalSection EventsController::s_serverLogsCs{};

    // =========================================================================
    // Static Utility Subroutines
    // =========================================================================
    static String UrlDecodeChannel(const String& input)
    {
        String sPlusReplaced = input;
        sPlusReplaced.Replace("+", " ");
        return Uri::UnescapeDataString(sPlusReplaced);
    }

    static DotNetDupe::System::Collections::Generic::List<String> GetTargetChannelsList(const String& sTargetChannel)
    {
        DotNetDupe::System::Collections::Generic::List<String> channelsToScan;
        if (sTargetChannel == String("ALL") || sTargetChannel.IsEmpty())
        {
            channelsToScan.Add(String("Security"));
            channelsToScan.Add(String("Microsoft-Windows-Sysmon/Operational"));
            channelsToScan.Add(String("System"));
            channelsToScan.Add(String("Application"));
            channelsToScan.Add(String("Microsoft-Windows-PowerShell/Operational"));
            channelsToScan.Add(String("Microsoft-Windows-TerminalServices-LocalSessionManager/Operational"));
            channelsToScan.Add(String("Microsoft-Windows-TaskScheduler/Operational"));
            channelsToScan.Add(String("Microsoft-Windows-CodeIntegrity/Operational"));
            channelsToScan.Add(String("Microsoft-Windows-Windows Defender/Operational"));
            channelsToScan.Add(String("Microsoft-Windows-AppLocker/EXE and DLL"));
        }
        else
        {
            channelsToScan.Add(sTargetChannel);
        }
        return channelsToScan;
    }

    static void ScanChannelEvents(const String& sChannel, DotNetDupe::System::Collections::Generic::List<EventRecord>& eventList)
    {
        WinEventLogReader logReader;
        if (!logReader.OpenLog(sChannel)) return;

        EventRecord evt;
        size_t countForChannel = 0;
        while (logReader.ReadNextEvent(evt) && countForChannel < 15)
        {
            EventLevel lvl = evt.GetLevel();
            RiskLevel risk = AnomalyEngine::EvaluateRisk(evt);

            if (lvl == EventLevel::Critical || lvl == EventLevel::Error || risk == RiskLevel::Critical || risk == RiskLevel::High || risk == RiskLevel::Medium)
            {
                eventList.Add(evt);
                countForChannel++;
            }
            else if (eventList.GetCount() < 10)
            {
                eventList.Add(evt);
                countForChannel++;
            }
        }
        logReader.Close();
    }

    // =========================================================================
    // Constructors & Destructor
    // =========================================================================
    EventsController::EventsController() = default;
    EventsController::~EventsController() = default;

    // =========================================================================
    // Logging Subsystem
    // =========================================================================
    void EventsController::Log(const String& sMessage)
    {
        Console::WriteLine(sMessage);
        Lock<CriticalSection> lock(s_serverLogsCs);
        if (s_serverLogs.GetCount() > 500)
        {
            s_serverLogs.RemoveAt(0);
        }
        s_serverLogs.Add(sMessage);
    }

    DotNetDupe::System::Collections::Generic::List<String> EventsController::GetServerLogList()
    {
        Lock<CriticalSection> lock(s_serverLogsCs);
        DotNetDupe::System::Collections::Generic::List<String> logs;
        for (int i = 0; i < s_serverLogs.GetCount(); ++i)
        {
            logs.Add(s_serverLogs[i]);
        }
        return logs;
    }

    // =========================================================================
    // Controller Action Endpoints
    // =========================================================================
    ChannelsResponseDto EventsController::GetChannels()
    {
        Log(String("[SERVER] Executing EventsController::GetChannels() -> Enumerating event sources..."));
        ChannelsResponseDto dto;
        m_logReader.GetEventSources(dto.Channels);
        Log(String::Format("[SERVER] Successfully enumerated channels count: {0}", dto.Channels.GetCount()));
        return dto;
    }

    EventLogResponseDto EventsController::GetEvents(const String& channelName)
    {
        return GetEvents(channelName, 1, 20);
    }

    EventLogResponseDto EventsController::GetEvents(const String& channelName, size_t page, size_t pageSize)
    {
        String sRawChannel = channelName.IsEmpty() ? String("Application") : channelName;
        String sTargetChannel = UrlDecodeChannel(sRawChannel);
        if (page < 1) page = 1;
        if (pageSize < 1) pageSize = 20;

        Log(String::Format("[SERVER] Executing EventsController::GetEvents() for channel: {0} (Page: {1}, PageSize: {2})", sTargetChannel, page, pageSize));

        unsigned long long uTotalCount = 0;
        String sCacheKey = String::Format("{0}_p{1}_ps{2}", sTargetChannel, page, pageSize);

        {
            Lock<CriticalSection> lock(s_eventsCacheCs);
            uTotalCount = m_logReader.GetChannelEventCount(sTargetChannel);

            ChannelEventCache cachedEntry;
            if (s_eventsCache.TryGetValue(sCacheKey, cachedEntry))
            {
                if (cachedEntry.LastEventCount == uTotalCount && uTotalCount > 0)
                {
                    Log(String::Format("[SERVER] [CACHE_HIT] Event count for '{0}' unchanged ({1} events). Returning cached response.", sTargetChannel, uTotalCount));
                    return cachedEntry.CachedResponse;
                }
            }
        }

        Log(String::Format("[SERVER] [CACHE_MISS] Event count changed or new channel query for '{0}' ({1} total events). Reading channel logs...", sTargetChannel, uTotalCount));

        auto levelCounts = DotNetDupe::System::Diagnostics::EtwLogReader::GetChannelEventLevelCounts(sTargetChannel);
        size_t totalPages = (uTotalCount == 0) ? 0 : static_cast<size_t>((uTotalCount + pageSize - 1) / pageSize);

        EventLogResponseDto responseDto;
        responseDto.Channel = sTargetChannel;
        responseDto.TotalCount = uTotalCount;
        responseDto.CriticalCount = levelCounts.uCriticalCount;
        responseDto.ErrorCount = levelCounts.uErrorCount;
        responseDto.WarningCount = levelCounts.uWarningCount;
        responseDto.InfoCount = levelCounts.uInfoCount;
        responseDto.VerboseCount = levelCounts.uVerboseCount;
        responseDto.Page = page;
        responseDto.PageSize = pageSize;
        responseDto.TotalPages = totalPages;

        size_t startIndex = (page - 1) * pageSize;

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
            Log(String::Format("[SERVER] Streamed DTO response: {0} events for channel '{1}'", responseDto.Events.GetCount(), sTargetChannel));
        }
        else
        {
            Log(String::Format("[SERVER] [WARNING] Failed to open channel log: {0}", sTargetChannel));
        }

        // Cache the newly fetched response
        {
            Lock<CriticalSection> lock(s_eventsCacheCs);
            ChannelEventCache newCacheEntry;
            newCacheEntry.LastEventCount = uTotalCount;
            newCacheEntry.CachedResponse = responseDto;
            s_eventsCache[sCacheKey] = newCacheEntry;
        }

        return responseDto;
    }
}
