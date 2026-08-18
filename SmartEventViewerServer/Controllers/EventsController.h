#pragma once

#include "WebAppCore/Controllers/ControllerBase.h"
#include "System/SmartPointer.h"
#include "Dto/EventDtos.h"
#include "Core/IEventService.h"

namespace SmartEventViewer {
    class EventsController : public DotNetDupe::WebAppCore::Controllers::ControllerBase {
    private:
        DotNetDupe::System::SmartPointer<IEventService> m_spEventService{ nullptr };

    public:
        EventsController();
        explicit EventsController(const DotNetDupe::System::SmartPointer<IEventService>& spService);
        ~EventsController() override = default;

        ChannelsResponseDto GetChannels();
        EventSummaryResponseDto GetEventSummary();
        EventSummaryResponseDto GetEventSummary(const String& channelName);

        EventLogResponseDto GetEvents(const String& channelName, size_t page, size_t pageSize, const String& sLevelFilter);
        EventLogResponseDto GetEvents(const String& channelName, size_t page, size_t pageSize);
        EventLogResponseDto GetEvents(const String& channelName);

        MultiChannelAnomaliesDto GetAnomalies();
        MultiChannelAnomaliesDto GetAnomalies(size_t limit);
    };
}
