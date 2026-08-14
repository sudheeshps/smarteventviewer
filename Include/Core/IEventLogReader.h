#pragma once

#include "ViewerCommon.h"
#include "Core/EventRecord.h"
#include "System/String.h"
#include "System/Collections/Generic/List.h"

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;
    using StringList = DotNetDupe::System::Collections::Generic::List<String>;

    class IEventLogReader {
    public:
        virtual ~IEventLogReader() = default;
        SMARTEVENTVIEWER_API virtual bool GetEventSources(StringList& outSources) = 0;
        SMARTEVENTVIEWER_API virtual bool OpenLog(const String& sChannelName) = 0;
        SMARTEVENTVIEWER_API virtual unsigned long long GetChannelEventCount(const String& sChannelName) = 0;
        SMARTEVENTVIEWER_API virtual bool ReadNextEvent(EventRecord& outEvent) = 0;
        SMARTEVENTVIEWER_API virtual void Close() = 0;
    };
}
