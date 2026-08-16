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
#include "Logging/AppLoggerManager.h"

using Console = DotNetDupe::System::Console;
using Uri = DotNetDupe::System::Uri;
using namespace DotNetDupe::System::Threading;

namespace SmartEventViewer {
    // =========================================================================
    // Static Member Definitions
    // =========================================================================
    // =========================================================================

    // =========================================================================
    // Static Utility Subroutines
    // =========================================================================
    static String UrlDecodeChannel(const String& input) {
        String sPlusReplaced = input;
        sPlusReplaced.Replace("+", " ");
        return Uri::UnescapeDataString(sPlusReplaced);
    }

    static DotNetDupe::System::Collections::Generic::List<String> GetTargetChannelsList(const String& sTargetChannel) {
        DotNetDupe::System::Collections::Generic::List<String> channelsToScan;
        if (sTargetChannel == "ALL" || sTargetChannel.IsEmpty()) {
            channelsToScan.Add("Security");
            channelsToScan.Add("Microsoft-Windows-Sysmon/Operational");
            channelsToScan.Add("System");
            channelsToScan.Add("Application");
            channelsToScan.Add("Microsoft-Windows-PowerShell/Operational");
            channelsToScan.Add("Microsoft-Windows-TerminalServices-LocalSessionManager/Operational");
            channelsToScan.Add("Microsoft-Windows-TaskScheduler/Operational");
            channelsToScan.Add("Microsoft-Windows-CodeIntegrity/Operational");
            channelsToScan.Add("Microsoft-Windows-Windows Defender/Operational");
            channelsToScan.Add("Microsoft-Windows-AppLocker/EXE and DLL");
        }
        else {
            channelsToScan.Add(sTargetChannel);
        }
        return channelsToScan;
    }

    static void ScanChannelEvents(const String& sChannel, DotNetDupe::System::Collections::Generic::List<EventRecord>& eventList) {
        auto rawEvents = DotNetDupe::System::Diagnostics::EtwLogReader::ReadEvents(sChannel, 15, 0, true);
        for (int i = 0; i < rawEvents.GetCount(); ++i) {
            EventRecord evt = EventRecord::FromEtwEvent(rawEvents[i], sChannel);
            EventLevel lvl = evt.GetLevel();
            RiskLevel risk = AnomalyEngine::EvaluateRisk(evt);

            if (lvl == EventLevel::Critical || lvl == EventLevel::Error || risk == RiskLevel::Critical || risk == RiskLevel::High || risk == RiskLevel::Medium) {
                eventList.Add(evt);
            } else if (eventList.GetCount() < 10) {
                eventList.Add(evt);
            }
        }
    }

    // =========================================================================
    // Constructors & Destructor
    // =========================================================================
    EventsController::EventsController() = default;
    EventsController::~EventsController() = default;

    // =========================================================================
    // Logging Subsystem
    // =========================================================================

    // =========================================================================
    // Controller Action Endpoints
    // =========================================================================
    ChannelsResponseDto EventsController::GetChannels() {
        AppLoggerManager::Info("SERVER", "[SERVER] Executing EventsController::GetChannels() -> Enumerating event sources...");
        ChannelsResponseDto dto;
        dto.Channels = DotNetDupe::System::Diagnostics::EtwLogReader::GetEventChannels();
        AppLoggerManager::Info("SERVER", String::Format("[SERVER] Successfully enumerated channels count: {0}", dto.Channels.GetCount()));
        return dto;
    }

    EventLogResponseDto EventsController::GetEvents(const String& channelName) {
        return GetEvents(channelName, 1, 20);
    }

    static void ExtractQueryParams(const DotNetDupe::System::SmartPointer<DotNetDupe::WebAppCore::Http::HttpRequest>& req, String& sChannel, String& sLevel, size_t& page, size_t& pageSize) {
        if (req.IsNull()) return;
        auto& query = req->GetQuery();
        String val;
        if (query.TryGetValue("channel", val) || query.TryGetValue("Channel", val)) sChannel = val;
        if (query.TryGetValue("level", val) || query.TryGetValue("Level", val)) sLevel = val;
        if (query.TryGetValue("page", val) && !val.IsEmpty()) page = static_cast<size_t>(DotNetDupe::System::Convert::ToUInt64(val));
        if (query.TryGetValue("pageSize", val) && !val.IsEmpty()) pageSize = static_cast<size_t>(DotNetDupe::System::Convert::ToUInt64(val));
    }

    EventSummaryResponseDto EventsController::GetEventSummary() {
        return GetEventSummary("");
    }

    EventSummaryResponseDto EventsController::GetEventSummary(const String& channelName) {
        String sRawChannel = channelName;
        if (!m_httpContext.IsNull() && !Request().IsNull()) {
            String sQueryChannel;
            if (Request()->GetQuery().TryGetValue("channel", sQueryChannel) || Request()->GetQuery().TryGetValue("Channel", sQueryChannel)) {
                sRawChannel = sQueryChannel;
            }
        }
        if (sRawChannel.IsEmpty()) sRawChannel = "Application";
        String sTargetChannel = UrlDecodeChannel(sRawChannel);

        AppLoggerManager::Info("SERVER", String::Format("[SERVER] Executing EventsController::GetEventSummary() for channel: {0}", sTargetChannel));
        try {
            return EventLruCacheManager::GetInstance().GetSummary(sTargetChannel);
        } catch (const DotNetDupe::System::Exception& ex) {
            AppLoggerManager::Error("SERVER", String::Format("[ERROR] GetEventSummary DotNetDupe exception for '{0}': {1}", sTargetChannel, ex.What()));
        } catch (const std::exception& ex) {
            AppLoggerManager::Error("SERVER", String::Format("[ERROR] GetEventSummary std::exception for '{0}': {1}", sTargetChannel, ex.what()));
        } catch (...) {
            AppLoggerManager::Error("SERVER", String::Format("[ERROR] GetEventSummary unknown SEH / system exception for '{0}'", sTargetChannel));
        }

        EventSummaryResponseDto fallbackDto;
        fallbackDto.Channel = sTargetChannel;
        return fallbackDto;
    }

    EventLogResponseDto EventsController::GetEvents(const String& channelName, size_t page, size_t pageSize, const String& sLevelFilter) {
        String sRawChannel = channelName;
        if (sRawChannel.IsEmpty()) {
            sRawChannel = "Application";
        }
        String sTargetChannel = UrlDecodeChannel(sRawChannel);

        String sFilter = sLevelFilter;
        if (sFilter.IsEmpty()) {
            sFilter = "ALL";
        }

        AppLoggerManager::Info("SERVER", String::Format("[SERVER] Executing EventsController::GetEvents() for channel: {0} (Page: {1}, PageSize: {2}, Level: {3})", sTargetChannel, page, pageSize, sFilter));
        return EventLruCacheManager::GetInstance().GetEvents(sTargetChannel, page, pageSize, sFilter);
    }

    EventLogResponseDto EventsController::GetEvents(const String& channelName, size_t page, size_t pageSize) {
        String sRawChannel = channelName;
        String sLevelFilter = "ALL";

        if (!m_httpContext.IsNull()) {
            ExtractQueryParams(Request(), sRawChannel, sLevelFilter, page, pageSize);
        }
        return GetEvents(sRawChannel, page, pageSize, sLevelFilter);
    }
}
