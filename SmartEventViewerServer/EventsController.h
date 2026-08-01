#pragma once

#include "WebAppCore/Controllers/ControllerBase.h"
#include "WebAppCore/Controllers/ControllerRouteBuilder.h"
#include "Platform/WinEventLogReader.h"
#include "Core/EventDtos.h"

using namespace DotNetDupe::System;
using namespace DotNetDupe::WebAppCore::Controllers;

namespace SmartEventViewer
{
    // ---------------------------------------------------------
    // EventsController class declaration inheriting from ControllerBase
    // ---------------------------------------------------------
    class EventsController : public ControllerBase
    {
    private:
        WinEventLogReader m_logReader{};

    public:
        EventsController() = default;
        ~EventsController() override = default;

        // Returns strongly typed ChannelsResponseDto payload
        ChannelsResponseDto GetChannels();

        // Returns strongly typed EventLogResponseDto payload with pagination
        EventLogResponseDto GetEvents(const String& channelName, size_t page, size_t pageSize);
        EventLogResponseDto GetEvents(const String& channelName);
    };
}
