#pragma once

#include "Common.h"
#include "Core/EventRecord.h"
#include "DotNetDupe/String.h"
#include "DotNetDupe/List.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;
    using StringList = DotNetDupe::System::Collections::Generic::List<String>;

    class SMARTEVENTVIEWER_API IEventLogReader
    {
    public:
        virtual ~IEventLogReader() = default;
        virtual StringList EnumerateEventSources() = 0;
        virtual bool OpenLog(const String& sChannelName) = 0;
        virtual unsigned long long GetChannelEventCount(const String& sChannelName) = 0;
        virtual bool ReadNextEvent(EventRecord& outEvent) = 0;
        virtual void Close() = 0;
    };
}
