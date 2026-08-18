#include "EventsController.h"
#include "Core/EventService.h"
#include "System/Uri.h"
#include "System/Convert.h"
#include "Logging/AppLoggerManager.h"

using Uri = DotNetDupe::System::Uri;

namespace SmartEventViewer {
    static String UrlDecodeChannel(const String& input) {
        String sPlusReplaced = input;
        sPlusReplaced.Replace("+", " ");
        return Uri::UnescapeDataString(sPlusReplaced);
    }

    EventsController::EventsController()
        : m_spEventService(EventService::GetDefault()) {
    }

    EventsController::EventsController(const DotNetDupe::System::SmartPointer<IEventService>& spService)
        : m_spEventService(spService.IsNull() ? EventService::GetDefault() : spService) {
    }

    ChannelsResponseDto EventsController::GetChannels() {
        AppLoggerManager::Info("SERVER", "[SERVER] EventsController::GetChannels() invoked");
        return m_spEventService->GetChannels();
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
        String sTarget = UrlDecodeChannel(sRawChannel);
        return m_spEventService->GetEventSummary(sTarget);
    }

    static void ExtractQueryParams(const DotNetDupe::System::SmartPointer<DotNetDupe::WebAppCore::Http::HttpRequest>& req,
                                   String& sChannel, String& sLevel, size_t& page, size_t& pageSize) {
        if (req.IsNull()) return;
        auto& query = req->GetQuery();
        String val;
        if (query.TryGetValue("channel", val) || query.TryGetValue("Channel", val)) sChannel = val;
        if (query.TryGetValue("level", val) || query.TryGetValue("Level", val)) sLevel = val;
        if (query.TryGetValue("page", val) && !val.IsEmpty()) page = static_cast<size_t>(DotNetDupe::System::Convert::ToUInt64(val));
        if (query.TryGetValue("pageSize", val) && !val.IsEmpty()) pageSize = static_cast<size_t>(DotNetDupe::System::Convert::ToUInt64(val));
    }

    EventLogResponseDto EventsController::GetEvents(const String& channelName, size_t page, size_t pageSize, const String& sLevelFilter) {
        String sTarget = UrlDecodeChannel(channelName.IsEmpty() ? String("Application") : channelName);
        String sFilter = sLevelFilter.IsEmpty() ? String("ALL") : sLevelFilter;
        return m_spEventService->GetEvents(sTarget, page, pageSize, sFilter);
    }

    EventLogResponseDto EventsController::GetEvents(const String& channelName, size_t page, size_t pageSize) {
        String sRawChannel = channelName;
        String sLevelFilter = "ALL";
        if (!m_httpContext.IsNull() && !Request().IsNull()) {
            ExtractQueryParams(Request(), sRawChannel, sLevelFilter, page, pageSize);
        }
        return GetEvents(sRawChannel, page, pageSize, sLevelFilter);
    }

    EventLogResponseDto EventsController::GetEvents(const String& channelName) {
        return GetEvents(channelName, 1, 20);
    }
}
