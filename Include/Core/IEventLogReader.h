#pragma once

#include "ViewerCommon.h"
#include "System/Object.h"
#include "System/String.h"
#include "System/Collections/Generic/List.h"
#include "Core/EventRecord.h"

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;
    using StringList = DotNetDupe::System::Collections::Generic::List<String>;

    class IEventLogReader : public virtual DotNetDupe::System::Object {
    public:
        virtual ~IEventLogReader() = default;

        virtual StringList GetEventChannels() = 0;
        virtual unsigned long long GetChannelEventCount(const String& sChannelName) = 0;
        virtual DotNetDupe::System::Collections::Generic::List<EventRecord> ReadEvents(
            const String& sChannelName,
            size_t nMaxCount,
            size_t nStartIndex = 0,
            bool bReverseOrder = true) = 0;

        virtual bool GetEventSources(StringList& outSources) {
            outSources = GetEventChannels();
            return outSources.GetCount() > 0;
        }

        virtual bool OpenLog(const String& sChannelName) {
            (void)sChannelName;
            return true;
        }

        virtual bool ReadNextEvent(EventRecord& outEvent) {
            (void)outEvent;
            return false;
        }

        virtual void Close() {}
    };
}
