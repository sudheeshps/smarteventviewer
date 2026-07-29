#pragma once

#include "Core/IEventLogReader.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;

    class SMARTEVENTVIEWER_API WinEventLogReader : public IEventLogReader
    {
    private:
        void* m_hSubscription{ nullptr };
        String m_sChannel{};

    public:
        WinEventLogReader();
        ~WinEventLogReader() override;

        StringList EnumerateEventSources() override;
        bool OpenLog(const String& sChannelName) override;
        bool ReadNextEvent(EventRecord& outEvent) override;
        void Close() override;
    };
}
