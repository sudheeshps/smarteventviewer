#pragma once

#include "WebAppCore/Controllers/ControllerBase.h"
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

        // Returns strongly typed List directly (DotNetDupe automatic JSON serialization)
        StringList GetChannels();

        // Returns strongly typed EventLogResponseDto payload
        EventLogResponseDto GetEvents(const String& channelName);
    };
}
