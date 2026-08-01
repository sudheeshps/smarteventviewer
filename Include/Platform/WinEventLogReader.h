#pragma once

#include "Core/IEventLogReader.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;

    class WinEventLogReader : public IEventLogReader
    {
    private:
        void* m_hSubscription{ nullptr };
        String m_sChannel{};

    public:
        SMARTEVENTVIEWER_API WinEventLogReader();
        SMARTEVENTVIEWER_API ~WinEventLogReader() override;

        SMARTEVENTVIEWER_API bool GetEventSources(StringList& outSources) override;
        SMARTEVENTVIEWER_API bool OpenLog(const String& sChannelName) override;
        SMARTEVENTVIEWER_API unsigned long long GetChannelEventCount(const String& sChannelName) override;
        SMARTEVENTVIEWER_API bool ReadNextEvent(EventRecord& outEvent) override;
        SMARTEVENTVIEWER_API void Close() override;
    };
}
