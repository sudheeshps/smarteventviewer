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

#include "Core/EventsController.h"
#include "Core/EventLruCacheManager.h"
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

    EventSummaryResponseDto EventsController::GetEventSummary()
    {
        String sRawChannel;
        if (!m_httpContext.IsNull() && !Request().IsNull()) {
            Request()->GetQuery().TryGetValue("channel", sRawChannel);
        }
        if (sRawChannel.IsEmpty()) sRawChannel = "Application";
        return GetEventSummary(sRawChannel);
    }

    EventSummaryResponseDto EventsController::GetEventSummary(const String& channelName)
    {
        String sRawChannel = channelName;
        if (sRawChannel.IsEmpty() && !m_httpContext.IsNull() && !Request().IsNull()) {
            Request()->GetQuery().TryGetValue("channel", sRawChannel);
        }
        if (sRawChannel.IsEmpty()) sRawChannel = "Application";
        String sTargetChannel = UrlDecodeChannel(sRawChannel);

        Log(String::Format("[SERVER] Executing EventsController::GetEventSummary() for channel: {0}", sTargetChannel));
        try {
            return EventLruCacheManager::GetInstance().GetSummary(sTargetChannel);
        } catch (const std::exception& ex) {
            Log(String::Format("[ERROR] GetEventSummary std::exception for '{0}': {1}", sTargetChannel, ex.what()));
        } catch (...) {
            Log(String::Format("[ERROR] GetEventSummary unknown SEH / system exception for '{0}'", sTargetChannel));
        }

        EventSummaryResponseDto fallbackDto;
        fallbackDto.Channel = sTargetChannel;
        return fallbackDto;
    }

    EventLogResponseDto EventsController::GetEvents(const String& channelName, size_t page, size_t pageSize)
    {
        String sRawChannel = channelName.IsEmpty() ? String("Application") : channelName;
        String sTargetChannel = UrlDecodeChannel(sRawChannel);

        Log(String::Format("[SERVER] Executing EventsController::GetEvents() for channel: {0} (Page: {1}, PageSize: {2})", sTargetChannel, page, pageSize));
        return EventLruCacheManager::GetInstance().GetEvents(sTargetChannel, page, pageSize);
    }
}
