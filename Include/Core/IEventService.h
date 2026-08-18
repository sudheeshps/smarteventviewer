#pragma once

#include "ViewerCommon.h"
#include "System/Object.h"
#include "System/String.h"
#include "Dto/EventDtos.h"

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;

    class IEventService : public virtual DotNetDupe::System::Object {
    public:
        virtual ~IEventService() = default;

        virtual ChannelsResponseDto GetChannels() = 0;
        virtual EventSummaryResponseDto GetEventSummary(const String& sChannelName) = 0;
        virtual EventLogResponseDto GetEvents(
            const String& sChannelName,
            size_t nPage,
            size_t nPageSize,
            const String& sLevelFilter = "ALL") = 0;
        virtual MultiChannelAnomaliesDto GetCrossChannelAnomalies(size_t maxPerChannel = 10) = 0;
        virtual void ClearCache() = 0;
    };
}
