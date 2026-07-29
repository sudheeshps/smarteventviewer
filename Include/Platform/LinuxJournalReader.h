#pragma once

#include "Core/IEventLogReader.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;

    class SMARTEVENTVIEWER_API LinuxJournalReader : public IEventLogReader
    {
    private:
        String m_sJournalPath{};
        bool m_bIsOpen{ false };

    public:
        LinuxJournalReader();
        ~LinuxJournalReader() override;

        StringList EnumerateEventSources() override;
        bool OpenLog(const String& sChannelName) override;
        unsigned long long GetChannelEventCount(const String& sChannelName) override;
        bool ReadNextEvent(EventRecord& outEvent) override;
        void Close() override;
    };
}
